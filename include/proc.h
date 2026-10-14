/*
 * Functions and structs for everything process-related
 *
 * Implemented in proc.c
 * - pclava 13 oct 2026
 */

#pragma once
#include <stdint.h>


// Kernel thread
struct context {
    uint32_t ra;    // 0
    uint32_t sp;    // 4
    uint32_t fp;    // 8
    uint32_t gp;    // 12
    uint32_t s0;    // 16
    uint32_t s1;    // 20
    uint32_t s2;    // 24
    uint32_t s3;    // 28
    uint32_t s4;    // 32
    uint32_t s5;    // 36
    uint32_t s6;    // 40
    uint32_t s7;    // 44
};

// Process trapframe (saved registers upon process switch)
struct trapframe {
    uint32_t at;    // 0
    uint32_t v0;    // 4
    uint32_t v1;    // 8
    uint32_t a0;    // 12
    uint32_t a1;    // 16
    uint32_t a2;    // 20
    uint32_t a3;    // 24
    uint32_t t0;    // 28
    uint32_t t1;    // 32
    uint32_t t2;    // 36
    uint32_t t3;    // 40
    uint32_t t4;    // 44
    uint32_t t5;    // 48
    uint32_t t6;    // 52
    uint32_t t7;    // 56
    uint32_t s0;    // 60
    uint32_t s1;    // 64
    uint32_t s2;    // 68
    uint32_t s3;    // 72
    uint32_t s4;    // 76
    uint32_t s5;    // 80
    uint32_t s6;    // 84
    uint32_t s7;    // 88
    uint32_t t8;    // 92
    uint32_t t9;    // 96
    uint32_t gp;    // 100
    uint32_t sp;    // 104
    uint32_t fp;    // 108
    uint32_t ra;    // 112
    uint32_t pc;    // 116
    uint32_t kernel_sp; // 120
};

// Process state
enum procstate {
    RUNNING,
    SLEEPING,
    RUNNABLE,
    UNUSED,
};

// Process information
struct proc {
    unsigned char PID;              // Process number and index in proctable
    struct context context;         // Process context
    struct trapframe *trapframe;    // Pointer to trapframe (somewhere in user memory but not mapped in any page talge)
    volatile enum procstate state;  // Current state, volatile important for compiler to know about concurrency
    void *chan;                     // If sleeping, channel the process is sleeping on
    void *pbrk;                     // Program break (virtual address)
    unsigned char parent;           // Parent process ID
    unsigned int pages;             // Pages currently in use
};

#define NPROC 64                        // Number of processes in the process table
extern struct proc proctable[NPROC];    // Table containing every process

// Proctable initialization
void procinit(void);

// Called by kernel initialization during boot. Infinite loop:
// Finds a runnable process and schedules it
void scheduler(void);

/*
 * Switch to scheduler context and save current context
 * This is a coroutine of scheduler():
 * sched() usually ends up in scheduler() and scheduler() usually ends up in sched()
 */
void sched(struct proc *p);

// Called once:
struct proc * pfind(void);              // Finds a slot for a new process. Returns pointer to process slot, or NULL on failure. Sets the PID itself!
void exec(int arg);                     // Loads an executable file from disk
void forkret(void);                     // A process's first scheduling ends up here. On process initialization, context $ra should be set here
void first_proc(void);                  // Creates the first process
void sleep(void *chan);                 // Puts a process to sleep on a given channel. Must be awoken with wakeup()
void wakeup(const void *chan);          // Wakes up any process sleeping on the given channel
uint32_t fork(void);                    // Creates new process, copying parent

// Called repeatedly:
struct proc * getproc(void);            // Returns pointer to currently running process
void switch_to(struct proc *p);         // Called when a process gets scheduled. Sets to running and switches contexts
void yield(void);                       // Called during timer interrupt. Pauses process and switches to scheduler thread