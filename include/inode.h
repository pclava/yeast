/*
 * inode layer
 *
 * implemented in inode.c
 * - pclava 7 nov 2026
 */

#pragma once
#include <stddef.h>

#include "fs.h"

void inodes_init(void);

// Create a new inode. This is the main interface for creating inodes
struct inode *ialloc(uint32_t type);

// Locks the inode for use. Reads it from disk if necesary
// Must be paired with iunlock
void ilock(struct inode *inode);

// Unlock inode
// Must be paired with ilock
void iunlock(struct inode *inode);

// Gets an inode in-memory data structure without locking or reading from disk
// Must be paired with iput
struct inode *iget(uint32_t inum);

// Drop the reference. If this is the last reference, drop from table
// If no links, delete from disk
// Must be paired with iget
void iput(struct inode *inode);

// Delete all data from inode
void itrunc(struct inode *inode);

// Write changes to disk
void iupdate(struct inode *inode);

// Free inode on disk
void ifree(uint32_t ino);

// Adds a reference to an inode
struct inode *idup(struct inode *ip);

// Reads data from file through inode. Main wrapper for reading
uint32_t readi(struct inode *ip, void *addr, size_t offset, size_t n);

// Copies data to file through inode. Main wrapper for writing
uint32_t writei(struct inode *ip, void *addr, size_t offset, size_t n);