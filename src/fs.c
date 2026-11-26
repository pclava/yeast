#include "fs.h"

#include <stddef.h>

#include "log.h"
#include "buffer.h"
#include "inode.h"
#include "proc.h"
#include "string.h"
#include "file.h"
#include "printk.h"
#include "vga.h"

struct superblock sb = {0};

static void read_superblock(struct superblock *super) {
    struct bbuf *block = bread(1);
    memcpy(super, block->data, sizeof(struct superblock));
    brelse(block);
}

static void block_zero(uint32_t bno) {
    struct bbuf *bp = bread(bno);
    memset_aligned((uint32_t *)bp->data, 0, BLOCKSIZE / sizeof(uint32_t));
    log_write(bp);
    brelse(bp);
}

uint32_t balloc(void) {
    uint32_t ceil_bmapsz = (sb.size + BITS_PER_BLOCK - 1) / BITS_PER_BLOCK;
    for (int bno = 0; bno < ceil_bmapsz; bno++) {
        // Get bitmap block
        struct bbuf *bp = bread(sb.bitmapstart+bno);

        // Read each byte in the block
        for (int bit = 0; bit < BITS_PER_BLOCK && bit + bno < sb.size; bit++) {
            int byte = bit / 8;
            char mask = (char) (1 << (bit % 8));

            if ((bp->data[byte] & mask) == 0) {
                // block is free, mark as used
                bp->data[byte] = (char) (bp->data[byte] | mask);
                log_write(bp); // write bitmap
                brelse(bp); // release block
                block_zero(bit+bno); // clear data
                return bit+bno;
            }
        }

        brelse(bp);
    }

    panic("balloc: no blocks!");
}

void bfree(uint32_t bno) {

    // Get bitmap block
    struct bbuf *bp = bread(bno / BITS_PER_BLOCK + sb.bitmapstart);

    // Get location in block
    uint32_t bit = bno % BITS_PER_BLOCK;
    uint32_t byte = bit / 8;
    char mask = (char) (1 << (bit % 8));

    if ((bp->data[byte] & mask) == 0) panic("bfree: block already free");

    // Set to 0
    bp->data[byte] = (char) (bp->data[byte] & (~mask));

    // Write and release
    log_write(bp);
    brelse(bp);
}

// called when first process is initialized
void fs_init(void) {
    bcache_init();

    // Read superblock
    read_superblock(&sb);

    // Init log
    log_init(&sb);

    // Init inodes
    inodes_init();

    // Init file table
    fileinit();
}

struct inode *dirlookup(struct inode *dirp, char *name, uint32_t *poff) {
    if (dirp == NULL || dirp->d.type != F_DIRECTORY) return NULL;

    struct direntry dirent;
    for (uint32_t off = 0; off < dirp->d.size; off += sizeof(struct direntry)) {
        readi(dirp, &dirent, off, sizeof(dirent));
        if (dirent.inode != 0) {
            if (strncmp(name, dirent.name, DIRSIZE) == 0) {
                if (poff != NULL) *poff = off;
                struct inode *inode = iget(dirent.inode);
                return inode;
            }
        }
    }

    return NULL;
}

// 0 on success; -1 on failure
int add_direntry(struct inode *dirp, char *name, uint32_t ino) {
    if (dirp == NULL || dirp->d.type != F_DIRECTORY) return -1;

    if (strlen(name) > DIRSIZE) return -1;

    // Check it doesn't exist already
    struct inode *ip;
    if ((ip = dirlookup(dirp, name, 0)) != 0) {
        iput(ip);
        return -1;
    }

    uint32_t off;
    struct direntry dirent;
    for (off = 0; off < dirp->d.size; off += sizeof(struct direntry)) {
        readi(dirp, &dirent, off, sizeof(struct direntry));
        if (dirent.inode == 0) break;
    }

    dirent.inode = ino;
    memset(dirent.name, 0, DIRSIZE+1);
    memcpy(dirent.name, name, DIRSIZE);
    if (writei(dirp, &dirent, off, sizeof(struct direntry)) != sizeof(struct direntry)) return -1;
    return 0;
}

// Gets the next name in a path, puts it in name, and returns the rest of the path
// name should be at least DIRSIZE
static char *get_name(char *path, char *name) {
    char *p = path;
    while (*p == '/') p++;
    if (*p == '\0') return NULL;
    char *s = p;
    size_t count = 0;
    while (*p != '/' && *p != '\0') {
        p++;
        count++;
    }
    if (count > DIRSIZE) {count = DIRSIZE;} // TODO: what happens here?
    memcpy(name, s, count);
    name[count] = '\0';
    while (*p == '/') p++;
    return p;
}

static struct inode *pathlookup(char *path, char *name, int parent) {
    struct inode *cur;
    struct inode *next;

    if (*path == '/') cur = iget(ROOTINUM);
    else cur = idup(getproc()->cwd);
    // printk("pathlookup: cwd %d @ 0x%x\n\r", cur->inum, cur);

    while ((path = get_name(path, name)) != NULL) {
        ilock(cur);
        if (cur->d.type != F_DIRECTORY || cur->d.nlink == 0) {
            goto fail;
        }
        if (parent && *path == '\0') {
            // If there's no more path, then we've just read the parent
            iunlock(cur);
            return cur;
        }
        if ((next = dirlookup(cur, name, NULL)) == 0) {
            goto fail;
        }
        iunlock(cur);
        iput(cur);
        cur = next;
    }

    if (parent) {
        // no parent was given
        iput(cur);
        return 0;
    }

    return cur;

    fail:
    iunlock(cur);
    iput(cur);
    return NULL;
}

struct inode *namelookup(char *path) {
    char name[DIRSIZE+1];
    return pathlookup(path, name, 0);
}

struct inode *parentlookup(char *path, char *name) {
    return pathlookup(path, name, 1);
}