/*
 * inode layer
 *
 * implemented in inode.c
 * - pclava 7 nov 2026
 */

#pragma once
#include <stddef.h>

#include "fs.h"
#include "lock.h"

struct inode {
    uint32_t inum;
    int32_t references;
    int32_t valid; // has the inode been read from disk?
    struct sleeplock lock;
    struct dinode d;
};

void inodes_init(void);

// Allocate a new inode. This is the main interface for getting inodes
struct inode *ialloc(uint32_t type);

// Locks the inode for use. Reads it from disk if necesary
void ilock(struct inode *inode);

// Drop the reference. If this is the last reference, drop from table
// If no links, delete from disk
void iput(struct inode *inode);

// Unlock inode
void iunlock(struct inode *inode);

// Delete all data from inode
void itrunc(struct inode *inode);

// Write changes to disk
void iupdate(struct inode *inode);

// Free inode on disk
void ifree(uint32_t ino);

// Reads data from file through inode. Main wrapper for reading
uint32_t readi(struct inode *ip, void *addr, size_t offset, size_t n);

// Copies data to file through inode. Main wrapper for writing
uint32_t writei(struct inode *ip, void *addr, size_t offset, size_t n);