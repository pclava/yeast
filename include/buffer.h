/*
 * Implementation of the buffer cache
 * This is the lowest layer of the file system,
 * taking disk sectors and loading them into
 * a cache of blocks in memory.
 *
 * The disk is divided into 512-byte *sectors* which are read/written atomically
 * A *block* is the in-memory representation of one or more sectors; a yeast block
 * is exactly one disk sector.
 *
 * A buffer has a lock to prevent concurrent reads/writes
 * The buffer's raw data is in the 512-byte data array
 *
 * The key functions exposed by this layer are
 * bread        reads a disk sector into a memory buffer, and acquires the lock for that buffer
 * bwrite       writes the buffer to the disk
 * brelse       release the buffer
 * Any call to bread must be accompanied by a call to bread!
 *
 * A typical block operation looks like
 * struct bbuf *b = bread(sector number)
 * (some data is written or read from b->data)
 * bwrite(b)
 * brelse(b)
 *
 * Implemented in buffer.c
 * - pclava 18 nov 2026
 */

#pragma once

#include "fs.h"
#include "lock.h"

#define NBUFS 32

// Block buffer
struct bbuf {
    int valid; // Contains a valid block
    int disk; // being used by disk (ie being written or read)
    uint32_t references;

    uint32_t bno; // Block number on disk

    // Linked list representation of buffer cache
    struct bbuf *next;
    struct bbuf *prev;

    struct sleeplock sleeplock;

    char data[BLOCKSIZE];
};

// Buffer cache initialization; must be called on boot
void bcache_init(void);

// Disk operations
void disk_read(struct bbuf *b);
void disk_write(struct bbuf *b);

// Buffer operations
struct bbuf *bread(uint32_t bno);       // Reads block from disk, acquires sleeplock on buffer
void bwrite(struct bbuf *b);            // Writes buffer to disk
void brelse(struct bbuf *b);            // Releases sleeplock and loses reference to buffer