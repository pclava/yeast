/*
 * Logging layer (skeleton)
 * Not yet implemented
 * Will provide logging buffer for crash recovery
 * The log makes file writes atomic:
 *      if a crash happens during a write,
 *      either none of the write or all of the write will go through,
 *      avoiding partial writes
 *
 * TODO: implement!
 * Implemented in log.c
 * - pclava 5 nov 2026
 */

#pragma once
#include "fs.h"
#include "buffer.h"

void log_init(struct superblock* sb);   // initializes logging layer
void log_write(struct bbuf* b);             // writes to the log. all file writes must do this!
void begin_op(void);                        // called at the start of fs system call: begins logging transaction
void end_op(void);                          // called at the end of fs system call: ends logging transaction. if no more syscalls are writing, commits the log