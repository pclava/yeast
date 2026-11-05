/*
 * Kernel-level concurrency
 * Since there's only one processor, locking really means disabling/enabling interrupts
 * Provides 'spinlock' analogs, which just involve disabling interrupts
 * and sleeplocks, which function as semaphores
 *
 * Implemented in lock.c
 * - pclava 31 oct 2026
 */

#pragma once
#include <stdbool.h>

/*
 * push_off and pop_off provide a safe way of enabling/disabling interrupts
 * they use a stack (kept in the cpu struct). if the stack is at 0, interrupts are enabled
 * otherwise they are disabled.
 */

void push_off(void);
void pop_off(void);

/*
 * acquire and release behave like spinlocks
 * between an acquire and a release, the code is guaranteed
 * not to be interrupted, unless an exception occurs (so
 * avoid causing those!)
 */

void acquire(void); // currently just an alias for push_off
void release(void); // currently just an alias for pop_off

/*
 * For long waits, sleeplocks allow a thread to be put to sleep
 * and other threads to run while it sleeps
 */

struct sleeplock {
    unsigned int locked;
    unsigned int pid;
};

void acquire_sleep(struct sleeplock *s); // goes to sleep on the given sleeplock
void release_sleep(struct sleeplock *s);
int holding_sleep(struct sleeplock *s);