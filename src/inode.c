#include "inode.h"

#include <stddef.h>

#include "buffer.h"
#include "fs.h"
#include "log.h"
#include "string.h"
#include "printk.h"

struct inode itable[NINODE];

static uint32_t bmap(uint32_t data_block, struct inode *ip);

void inodes_init(void) {
    for (struct inode *i = itable; i < &itable[NINODE]; i++) {
        memset_aligned((uint32_t *)i, 0, sizeof(struct inode)/sizeof(uint32_t));
    }
}

struct inode *ialloc(uint32_t type) {
    if (type == 0) return NULL;

    // For every inode block
    for (int i = 0; i < NINODES/INODES_PER_BLOCK; i++) {
        // Read block
        struct bbuf *bp = bread(sb.inodestart+i);
        struct dinode *dinodes = (struct dinode *)bp->data;

        // For each inode inside block
        for (int j = 0; j < INODES_PER_BLOCK; j++) {
            uint32_t inum = i*INODES_PER_BLOCK + j;
            struct dinode *d = dinodes + j;
            if (d->type == 0) {
                // Inode is unused
                // Clear
                memset_aligned((uint32_t *)d, 0, sizeof(struct dinode)/sizeof(uint32_t));
                // Mark used
                d->type = type;
                // Write to disk
                log_write(bp);
                brelse(bp);
                return iget(inum);
            }
        }

        // Release block
        brelse(bp);
    }

    panic("ialloc: no inodes on disk");
}

void ilock(struct inode *inode) {
    if (inode == NULL || inode->references < 1) panic("ilock: invalid inode");

    // Get inode
    acquire_sleep(&inode->lock);

    // Read from disk
    if (inode->valid == 0) {
        // read block
        uint32_t bno = IBLOCK(inode->inum, sb);
        struct bbuf *bp = bread(bno);
        struct dinode *dinodes = (struct dinode *)bp->data;
        struct dinode *dp = &dinodes[inode->inum % INODES_PER_BLOCK];
        // copy dp into inode.d
        memcpy(dp, &inode->d, sizeof(struct dinode));
        brelse(bp);
        inode->valid = 1;
    }
}

void iunlock(struct inode *inode) {
    if (inode == NULL || !holding_sleep(&inode->lock)) {
        panic("iunlock: bad inode");
    }

    release_sleep(&inode->lock);
}

void iput(struct inode *inode) {

    acquire();

    int last_usage = (inode->references == 1) && (inode->d.nlink == 0) && (inode->valid == 0);

    if (last_usage) {
        // Delete inode
        acquire_sleep(&inode->lock);
        release();

        itrunc(inode); // delete file
        inode->valid = 0;

        release_sleep(&inode->lock);
        acquire();
    }

    inode->references--; // refs = 0 means slot in table is free to use
    release();

    if (last_usage) {
        // Mark as free on disk
        ifree(inode->inum);
    }
}

void itrunc(struct inode *inode) {
    // For every data block in inode, bfree();
    if (inode == NULL) panic("itrunc: bad inode");

    // Read direct blocks
    for (int i = 0; i < NDIRECT; i++) {
        uint32_t bno = inode->d.data[i];
        if (bno == 0) continue;
        bfree(bno);
        inode->d.data[i] = 0;
    }

    // Read indirect blocks
    if (inode->d.data[NDIRECT]) {
        // Read block
        struct bbuf *bp = bread(inode->d.data[NDIRECT]);
        uint32_t *bs = (uint32_t *)bp->data;
        for (int i = 0; i < NINDIRECT; i++) {
            uint32_t bno = bs[i];
            if (bno == 0) continue;
            bfree(bno);
            inode->d.data[i] = 0;
        }
        brelse(bp);
        bfree(inode->d.data[NDIRECT]);
        inode->d.data[NDIRECT] = 0;
    }

    inode->d.size = 0;
    iupdate(inode);
}

void iupdate(struct inode *inode) {
    if (inode == NULL) panic("iupdate: bad inode");

    uint32_t bno = sb.inodestart + inode->inum / INODES_PER_BLOCK;
    struct bbuf *bp = bread(bno);

    struct dinode *dinode = (struct dinode *)bp->data + inode->inum % INODES_PER_BLOCK;
    memcpy(&inode->d, dinode, sizeof(struct dinode));
    log_write(bp);

    brelse(bp);
}

void ifree(uint32_t ino) {
    uint32_t bno = sb.inodestart + ino / INODES_PER_BLOCK;
    struct bbuf *bp = bread(bno);
    struct dinode *dinode = (struct dinode *)bp->data + ino % INODES_PER_BLOCK;
    dinode->type = 0;
    log_write(bp);
    brelse(bp);
}

struct inode *idup(struct inode *ip) {
    acquire();
    ip->references++;
    release();
    return ip;
}

uint32_t min(uint32_t a, uint32_t b) {
    if (a <= b) return a;
    return b;
}

uint32_t readi(struct inode *ip, void *addr, const size_t offset, size_t n) {
    if (offset >= ip->d.size) {
        return 0;
    }
    if (offset + n > ip->d.size) {
        n = ip->d.size - offset;
    }

    size_t left = n;
    size_t count = 0;
    size_t off = offset;

    while (count < n) {
        // Get block
        uint32_t bno = bmap(off / BLOCKSIZE, ip);
        struct bbuf *bp = bread(bno);

        // How much do we copy?
        uint32_t block_left = BLOCKSIZE - (off % BLOCKSIZE);
        uint32_t copy = min(block_left, left);

        // Copy data
        memcpy(&bp->data[off % BLOCKSIZE], addr, copy);

        // Increment
        count += copy;
        off += copy;
        left -= copy;

        brelse(bp);
    }

    return count;
}

uint32_t writei(struct inode *ip, void *addr, size_t offset, size_t n) {
    if (offset > ip->d.size || offset + n > (MAX_FILE * BLOCKSIZE)) {
        return -1;
    }

    uint32_t left = n;
    uint32_t count = 0;
    size_t off = offset;

    while (count < n) {
        uint32_t bno = bmap(off / BLOCKSIZE, ip);
        struct bbuf *bp = bread(bno);

        uint32_t block_left = BLOCKSIZE - (off % BLOCKSIZE);
        uint32_t copy = min(block_left, left);

        memcpy(addr, &bp->data[off % BLOCKSIZE], copy);
        log_write(bp);

        left -= copy;
        count += copy;
        off += copy;

        brelse(bp);
    }

    if (off > ip->d.size) ip->d.size = off;
    iupdate(ip);

    return count;
}

// Gets the inode for this number, allocating one if needed
// Does not write useful information. ilock() must be called after
struct inode *iget(uint32_t inum) {
    struct inode *empty = NULL;

    acquire();

    // Look through table
    for (struct inode *i = itable; i < &itable[NINODE]; i++) {
        if (i->references > 0 && i->inum == inum) {
            i->references++;
            release();
            return i;
        }
        if (i->references == 0 && empty == NULL) {
            empty = i;
        }
    }

    if (empty != NULL) {
        empty->inum = inum;
        empty->references = 1;
        release();
        return empty;
    }

    panic("iget: no inodes in table");
}

// Returns the block number of the given data block in the file
uint32_t bmap(uint32_t data_block, struct inode *ip) {
    uint32_t *data = &ip->d.data[0];
    // Direct block
    if (data_block < NDIRECT) {
        if (data[data_block] == 0) {
            // Allocate block
            uint32_t bno = balloc();
            data[data_block] = bno;
        }
        return data[data_block];
    }

    // Indirect
    data_block -= NDIRECT;
    if (data_block < NINDIRECT) {
        // Read indirect block
        if (data[NDIRECT] == 0) {
            uint32_t indirect = balloc();
            data[NDIRECT] = indirect;
        }
        uint32_t bno;
        struct bbuf *bp = bread(data[NDIRECT]);
        uint32_t *blocks = (uint32_t *)bp->data;
        if (blocks[data_block] == 0) {
            bno = balloc();
            blocks[data_block] = bno;
            log_write(bp);
        } else {
            bno = blocks[data_block];
        }
        brelse(bp);
        return bno;
    }

    panic("bmap: out of range");
}