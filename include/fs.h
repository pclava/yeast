/*
 * Core file system code
 * This is an amalgamation of a few layers
 *
 * Implemented in fs.c
 * - pclava 18 nov 2026
 */

#pragma once
#include <stdint.h>

#include "lock.h"

/* === DISK LAYOUT === */

#define BLOCKSIZE 512

#define FSMAGIC 0x594D4653

struct superblock {
    uint32_t magic;         // Magic number
    uint32_t size;          // Size of file system in blocks
    uint32_t ndata;         // Number of data blocks
    uint32_t ninodes;       // Number of inode
    uint32_t nlog;          // Number of log blocks
    uint32_t logstart;      // First log block
    uint32_t inodestart;    // First inode block
    uint32_t bitmapstart;     // First data block
};

extern struct superblock sb;

#define NDIRECT 12  // Number of direct data blocks kept in inode
#define NINDIRECT (BLOCKSIZE / sizeof(uint32_t)) // Number of blocks blocks kept in the inode's indirect block
#define MAX_FILE (NDIRECT+NINDIRECT)

void fs_init(void);             // Initialize file system (should be called in forkret right before exec'ing init)
uint32_t balloc(void);          // Mark a sector as used in the bitmap
void bfree(uint32_t bno);       // Mark a sector as free in the bitmap

/* === INODES === */

enum file_type {
    F_DIRECTORY = 1,
    F_FILE = 2,
};

// On-disk inode structure
struct dinode {
    uint32_t type;
    uint32_t nlink;
    uint32_t size;                  // Size of file in bytes
    uint32_t data[NDIRECT + 1];     // Direct data blocks. Last one points to the single indirect block
};

// In-memory inode structure
struct inode {
    uint32_t inum;
    int32_t references;
    int32_t valid; // has the inode been read from disk?
    struct sleeplock lock;
    struct dinode d;
};

#define INODES_PER_BLOCK (BLOCKSIZE / sizeof(struct dinode))
#define BITS_PER_BLOCK (BLOCKSIZE * 8)
#define IBLOCK(i, superblock) (superblock.inodestart + ((i) / INODES_PER_BLOCK))
#define NINODES 200 // inodes on disk
#define NINODE 50 // inodes in memory
#define ROOTINUM 1

/* === DIRECTORY === */

#define DIRSIZE 15

// Directory entry
struct direntry {
    uint32_t inode; // 0 = free
    char name[DIRSIZE+1];
};

// Looks up the name in the directory. If it finds it, calls iget() and returns the inode, placing the offset inside the directory in poff
// It does not call ilock()! This call is equivalent to a call to iget() in that respect.
// If it fails, it returns NULL. If poff is NULL, it is not touched.
struct inode *dirlookup(struct inode *dirp, char *name, uint32_t *poff);

int add_direntry(struct inode *dirp, char *name, uint32_t ino); // adds an entry to the given directory

/* === PATHNAME === */

// Both return an unlocked inode that has been called with iget()
struct inode *namelookup(char *path);
struct inode *parentlookup(char *path, char *name);