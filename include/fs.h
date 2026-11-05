/*
 * Core file system code
 *
 * Implemented in fs.c
 * - pclava 5 nov 2026
 */

#pragma once
#include <stdint.h>

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

#define NDIRECT 12  // Number of direct data blocks kept in inode
#define NINDIRECT (BLOCKSIZE / sizeof(uint32_t)) // Number of blocks blocks kept in the inode's indirect block
#define MAX_FILE (NDIRECT+NINDIRECT)

enum file_type {
    F_DIRECTORY = 0,
    F_FILE = 1,
};

// On-disk inode structure
struct dinode {
    uint32_t type;
    uint32_t nlink;
    uint32_t size;                  // Size of file in bytes
    uint32_t data[NDIRECT + 1];     // Direct data blocks. Last one points to the single indirect block
};

#define INODES_PER_BLOCK (BLOCKSIZE / sizeof(struct dinode))
#define BITS_PER_BLOCK (BLOCKSIZE * 8)
#define IBLOCK(i, superblock) (superblock.inodestart + ((i) / INODES_PER_BLOCK))

#define DIRSIZE 15

// Directory entry
struct direntry {
    uint32_t inode;
    char name[DIRSIZE+1];
};

void fs_init(void);