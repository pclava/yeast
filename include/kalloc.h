/*
 * Physical memory manager
 * Taken from https://github.com/mit-pdos/xv6-riscv/blob/riscv/kernel/kalloc.c
 *
 * The code keeps pointers in aliased user memory, but functions take and return real physical user addresses!
 * TODO: write my own
 * Implemented in kalloc.c
 * - pclava 13 oct 2026
 */


#pragma once

void*           kalloc(void);   // Find a free frame in physical memory
void            kfree(void *);  // Free a frame
void            kinit(void);    // Initialize frame allocator