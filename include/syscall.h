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
#define SYS_printint    1
#define SYS_printstr    4
#define SYS_readstr     8
#define SYS_sbrk        9
#define SYS_exit        10
#define SYS_printchr    11
#define SYS_readfile    14
#define SYS_writefile   15
#define SYS_termctl     28
#define SYS_exec        29
#define SYS_fork        30
#define SYS_wait        31
#define SYSDOMAIN       31 // largest syscall and number of syscalls

// Execute the syscall based on the contents of the trapframe's $v0 and argument registers
// Called by usertrap() in trap.c
void syscall();