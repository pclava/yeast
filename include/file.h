/*
 * File Layer
 *
 * - pclava 14 nov 2026
 */

#pragma once

#define NFILE 64
#include <stdint.h>
#include "inode.h"

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

extern struct file ftable[NFILE];

void fileinit(void);

// Finds an unused file in the table and gets a reference
// Should call fileclose to drop reference
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
