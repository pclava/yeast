/*
 * Inode layer
 * The inode layer is where the magic happens for a file system,
 * turning numbered disk sectors into abstract files.
 *
 * In the code, a *dinode* is the disk representation of an inode
 * and an *inode* is the in-memory representation. An inode consists
 * of a dinode and some metadata, namely the inum. A dinode does not
 * hold its own number. Neither a dinode nor an inode holds the file's name
 *
 * The key concept for using inodes are
 * iget/iput
 * ilock/iunlock
 *
 * A call to iget must be paired with a call to iput. Beware that some functions
 * call iget internally but still must be paired with a call to put.
 * Likewise, a call to ilock must be paired with a call to iunlock.
 *
 * iget gets a reference to an in-memory inode in the kernel's table of inodes.
 * A reference reserves a spot on the table for a specific inumber. Since a thread
 * may want to hold a reference to an inode without needing its data immediately,
 * iget does not read the inode from the disk.
 *
 * To do that, the thread must call ilock, which locks the inode and reads it from the disk.
 * Importantly, this does not read the inode's *data* (as in the data in the file), only the inode itself.
 * One can think of ilock() as filling in the dinode part of an inode and iget() filling in the in-memory part.
 *
 * After a thread is done directly operating on an inode, it should call iunlock. After it's
 * done with the inode completely, it should call iput.
 *
 * inodes provide the abstraction of files with readi and writei.
 *
 * Any changes to a dinode's data must be followed with a call to iupdate.
 *
 * A typical inode operation should look like
 * struct inode *ip = iget(inum)
 * ilock(ip)
 * (some operation like readi, writei, iupdate())
 * iunlock(ip)
 * iput(ip)
 *
 * implemented in inode.c
 * - pclava 18 nov 2026
 */

#pragma once
#include <stddef.h>

#include "fs.h"

void inodes_init(void);

// Create a new inode. This is the main interface for creating inodes
// Calls iget()!!!
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

// Write changes to disk, must be called every time an inode's metadata is edited
void iupdate(struct inode *inode);

// Free inode on disk
void ifree(uint32_t ino);

// Adds a reference to an inode
struct inode *idup(struct inode *ip);

// Reads data from file through inode. Main wrapper for reading
uint32_t readi(struct inode *ip, void *addr, size_t offset, size_t n);

// Copies data to file through inode. Main wrapper for writing
uint32_t writei(struct inode *ip, void *addr, size_t offset, size_t n);