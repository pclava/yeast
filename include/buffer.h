/*
 * Code for buffer cache layer in file system,
 * the lowest in the layered abstraction.
 * A buffer represents a cached disk block.
 * This file also contains functions for interacting
 * directly with the disk
 *
 * Implemented in buffer.c
 * - pclava 3 nov 2026
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

void disk_test();

// Buffer operations
struct bbuf *bread(uint32_t bno);       // Reads block from disk, acquires sleeplock on buffer
void bwrite(struct bbuf *b);            // Writes buffer to disk
void brelse(struct bbuf *b);            // Releases sleeplock and loses reference to buffer