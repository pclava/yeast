#include "fs.h"

#include <stddef.h>

#include "log.h"
#include "buffer.h"
#include "inode.h"
#include "proc.h"
#include "file.h"
#include "printk.h"

struct superblock sb = {0};

static void * memset_aligned(uint32_t *dst, uint32_t c, size_t n) {
    uint32_t *dest = dst;
    while (n--) {
        *dest++ = c;
    }
    return dst;
}

static uint32_t *memcpy_aligned(const uint32_t *src, uint32_t *dst, size_t n) {
    uint32_t *dest = dst;
    while (n--) {
        *dest++ = *src++;
    }
    return dst;
}

static uint32_t *memset(void *dst, char c, size_t n) {
    size_t w = n/4;
    size_t left = n%4;
    uint32_t C = c || (c << 8) || (c << 16) || (c << 24);
    memset_aligned(dst, C, left);
    unsigned char *d = dst+(w * sizeof(uint32_t));
    while (left--) {
        *d++ = c;
    }
    return dst;
}

static void *memcpy(const void *src, void *dst, size_t n) {
    size_t w = n/4;
    size_t left = n%4;
    memcpy_aligned((const uint32_t *)src, (uint32_t *)dst, w);
    const unsigned char *s = (const unsigned char *)src+(w * sizeof(uint32_t));
    unsigned char *d = dst+(w * sizeof(uint32_t));
    while (left--) {
        *d++ = *s++;
    }
    return dst;
}

static int strncmp(const char *s1, const char *s2, size_t n)
{
    if (n == 0)
        return (0);
    do {
        if (*s1 != *s2++)
            return (*(unsigned char *)s1 - *(unsigned char *)--s2);
        if (*s1++ == 0)
            break;
    } while (--n != 0);
    return (0);
}

static size_t strlen(const char *str) {
    const char *s;
    for (s = str; *s; ++s)
        ;
    return (s - str);
}

static void read_superblock(struct superblock *super) {
    struct bbuf *block = bread(1);
    memcpy(block->data, super, sizeof(struct superblock));
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
int dirlink(struct inode *dirp, char *name, uint32_t ino) {
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
    memcpy(name, dirent.name, DIRSIZE);
    if (writei(dirp, &dirent, off, sizeof(struct direntry)) != sizeof(struct direntry)) return -1;
    return 0;
}

// Gets the next name in a path, puts it in name, and returns the rest of the path
// name should be at least DIRSIZE
static char *get_name(char *path, char *name) {
    char *p = path;
    while (*p == '/') p++;
    if (*p == '\0') return NULL;
    size_t count = 0;
    while (*p != '/' && *p != '\0') {
        p++;
        count++;
    }
    if (count > DIRSIZE) {count = DIRSIZE;} // TODO: what happens here?
    memcpy(path, name, count);
    name[count] = '\0';
    while (*p == '/') p++;
    return p;
}

struct inode *pathlookup(char *path, char *name) {
    struct inode *cur;
    struct inode *next;

    if (*path == '/') cur = iget(ROOTINUM);
    else cur = idup(getproc()->cwd);

    while ((path = get_name(path, name)) != NULL) {
        ilock(cur);
        if (cur->d.type != F_DIRECTORY || cur->d.nlink == 0) goto fail;
        if ((next = dirlookup(cur, name, NULL)) == 0) goto fail;
        iunlock(cur);
        iput(cur);
        cur = next;
    }

    return cur;

    fail:
    iunlock(cur);
    iput(cur);
    return NULL;
}