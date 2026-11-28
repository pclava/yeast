#include "inode.h"

#include <stddef.h>

#include "buffer.h"
#include "fs.h"
#include "log.h"
#include "memlayout.h"
#include "string.h"
#include "printk.h"
#include "vga.h"

struct inode itable[NINODE];

static uint32_t bmap(uint32_t data_block, struct inode *ip);

void inodes_init(void) {
    for (struct inode *i = itable; i < &itable[NINODE]; i++) {
        memset_aligned((uint32_t *)i, 0, sizeof(struct inode)/sizeof(uint32_t));
    }
}

// Get a pointer to the dinode given a bbuf ptr
#define DINODE(bp, inum) ( &((struct dinode *)bp->data)[(inum % INODES_PER_BLOCK)] )

// Read the block containing the inode
#define IBREAD(inum) ( bread(IBLOCK(inum, sb)) )
#define IBRELSE(bp) ( brelse(bp) ) // for idiomatic purposes

struct inode *ialloc(uint32_t type) {
    if (type == F_NONE) return NULL;

    for (int ino = 1; ino < NINODES; ino++) {
        struct bbuf *bp = IBREAD(ino);
        struct dinode *dp = DINODE(bp, ino);
        if (dp->type == F_NONE) {
            // Disk inode is free
            memset(dp, 0, sizeof(struct dinode));
            dp->type = type;
            log_write(bp);
            brelse(bp);
            return iget(ino); // get the in-memory data structure
        }
        IBRELSE(bp);
    }

    panic("ialloc: no free indoes on disk");
}

void ilock(struct inode *ip) {
    if (ip == NULL || ip->references < 0) panic("ilock: bad inode");

    acquire_sleep(&ip->lock);

    // read from disk if needed
    if (ip->valid == 0) {
        struct bbuf *bp = IBREAD(ip->inum);
        struct dinode *dp = DINODE(bp, ip->inum);
        memcpy(&ip->d, dp, sizeof(struct dinode));
        IBRELSE(bp);
        ip->valid = 1;
    }
}

void iunlock(struct inode *inode) {
    if (inode == NULL || !holding_sleep(&inode->lock)) {
        panic("iunlock: bad inode");
    }

    release_sleep(&inode->lock);
}

// Gets the inode for this number, allocating one if needed
// Does not write useful information. ilock() must be called after
struct inode *iget(uint32_t inum) {
    acquire();

    // Look for the inode in the table
    struct inode *free = NULL;
    for (struct inode *ip = itable; ip < &itable[NINODE]; ip++) {
        if (ip->references > 0 && ip->inum == inum) {
            ip->references++;
            release();
            return ip;
        }
        if (ip->references == 0) free = ip;
    }

    // Allocate
    if (free) {
        // assign inode
        free->inum = inum;
        free->references = 1;
        free->valid = 0; // tell caller to read from disk
        release();
        return free;
    }

    panic("iget: no free inodes in memory");
}

void iput(struct inode *ip) {
    acquire();
    int last_reference = (ip->references == 1) && (ip->d.nlink == 0) && (ip->valid);
    uint32_t inum = ip->inum;
    // delete inode's data
    if (last_reference) {
        itrunc(ip);
        ip->valid = 0;
    }

    // clear from table
    ip->references--;
    release();

    // outside of a lock, delete the inode (we do this outside to reduce the time we spent in the lock)
    if (last_reference) {
        ifree(inum);
    }
}

void itrunc(struct inode *inode) {
    if (inode == NULL) panic("itrunc: bad inode");
    // For every data block in inode, bfree();

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

    struct bbuf *bp = IBREAD(inode->inum);
    struct dinode *dp = DINODE(bp, inode->inum);
    memcpy(dp, &inode->d, sizeof(struct dinode));
    log_write(bp);

    IBRELSE(bp);
}

void ifree(uint32_t ino) {
    struct bbuf *bp = IBREAD(ino);
    struct dinode *dp = DINODE(bp, ino);
    dp->type = 0;
    log_write(bp);
    IBRELSE(bp);
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
    char *dst = (char *)addr;

    while (count < n) {
        // Get block
        uint32_t bno = bmap(off / BLOCKSIZE, ip);
        if (bno == 0) break;
        struct bbuf *bp = bread(bno);

        // How much do we copy?
        uint32_t block_left = BLOCKSIZE - (off % BLOCKSIZE);
        uint32_t copy = min(block_left, left);

        // Copy data
        memcpy(dst, &bp->data[off % BLOCKSIZE], copy);

        brelse(bp);

        // Increment
        count += copy;
        off += copy;
        left -= copy;
        dst += copy;
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
    char *src = (char *)addr;

    while (count < n) {
        uint32_t bno = bmap(off / BLOCKSIZE, ip);
        struct bbuf *bp = bread(bno);

        uint32_t block_left = BLOCKSIZE - (off % BLOCKSIZE);
        uint32_t copy = min(block_left, left);

        memcpy(&bp->data[off % BLOCKSIZE], src, copy);
        log_write(bp);

        left -= copy;
        count += copy;
        off += copy;
        src += copy;

        brelse(bp);
    }

    if (off > ip->d.size) ip->d.size = off;
    iupdate(ip);

    return count;
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