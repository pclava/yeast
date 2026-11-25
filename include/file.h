/*
 * File Layer
 * This is the highest kernel-level abstraction in the file system
 * These functions handle the global file table.
 * Per-process file tables are handled at the syscall-layer
 *
 * Providing a single wrapper for files, directories, pipes, and devices (the last two are not implemented)
 * The functions exposed by this layer are:
 * filealloc        gets a reference to a file structure in the table
 * fileclose        drops the reference got by filealloc
 * fileread         is a wrapper for the associated read function (i.e, readi for inodes)
 * filewrite        cf. fileread
 * filedup          gets another reference to an existing file
 *
 * Note it's still up to the programmer to find the inode and set it
 *
 * - pclava 18 nov 2026
 */

#pragma once
#include <stdint.h>
#include "inode.h"

#define NFILE 64
#define NOPEN 16

#define O_CREAT     0b00000001
#define O_RDONLY    0b00000010
#define O_WRONLY    0b00000100
#define O_RDWR      0b00001000
#define O_TRUNC     0b00010000

enum fd_type {
    FD_NONE,
    FD_PIPE,
    FD_DEVICE,
    FD_INODE,
};

struct file {
    enum fd_type type;
    uint32_t references;
    char readable;
    char writable;

    uint32_t offset; // file pointer (where in inode we are reading/writing)
    struct inode *ip;
};

struct stat {
    uint32_t size;
    uint32_t ino;
    uint32_t nlink;
    uint32_t type;
};

extern struct file ftable[NFILE];

void fileinit(void);

// Finds an unused file in the GLOBAL file table and gets a reference
// Should call fileclose to drop reference
// Returns NULL on failure
struct file *filealloc(void);

// Gets another reference to a file. Should call fileclose afterwards
struct file *filedup(struct file *fp);

// Drops reference to file
// If references are 0, releases underlying data structure (inode, pipe, device)
void fileclose(struct file *fp);

// Wrapper for underlying read function. Checks permissions
// For inodes, uses offset
int fileread(struct file *fp, void *addr, size_t size);

// Wrapper for underlying write function. Checks permissions
// For inodes, uses offset
int filewrite(struct file *fp, void *addr, size_t size);

// Writes file metadata into pointer
int filestat(struct file *fp, struct stat *st);