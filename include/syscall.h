/*
 * Syscall handler
 *
 * To add a new syscall:
 * 1. define its identifier here, with the format SYS_[name]. Modify SYSDOMAIN if needed
 * 2. in syscall.c, write the associated function, with the name sys_[name]
 *      - when writing the function, use argraw(n) to extract the n'th argument
 * 3. in syscall.c, add the new function to the syscall table
 * 4. add the user-level wrapper function to user.h and its implementation in usys.s
 *
 * Implemented in syscall.c
 * - pclava 13 oct 2026
 */

#pragma once

// Process syscalls
#define SYS_sbrk        1
#define SYS_exit        2
#define SYS_termctl     3
#define SYS_fork        4
#define SYS_wait        5
#define SYS_sleep       6
// File syscalls
#define SYS_read        20
#define SYS_write       21
#define SYS_exec        22
#define SYSDOMAIN       22 // largest syscall and number of syscalls

// Execute the syscall based on the contents of the trapframe's $v0 and argument registers
// Called by usertrap() in trap.c
void syscall();