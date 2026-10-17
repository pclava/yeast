# ===
#   User-level wrapper functions for syscalls
#   Implementation of the syscall functions in user.h
#
#   - pclava 13 oct 2026
# ===

    .text

# int fork(void)
    .globl  fork
fork:
    li      $v0, _SYSFORK
    syscall
    #break
    jr      $ra

# void exec(void)
    .globl  exec
exec:
    li      $v0, _SYSEXEC
    syscall
    jr      $ra

# int write(int fd, const void *buffer, int n)
    .globl  write
write:
    li      $v0, _SYSWRITEFILE
    syscall
    jr      $ra

# int read(int fd, void *buffer, int n)
    .globl  read
read:
    li      $v0, _SYSREADFILE
    syscall
    jr      $ra

# char *sbrk(int n)
    .globl  sbrk
sbrk:
    li      $v0, _SYSSBRK
    syscall
    jr      $ra

# void termctl(enum termop op)
    .globl  termctl
termctl:
    li      $v0, _SYSTERMCTL
    syscall
    jr      $ra

# void exit(unsigned int code)
    .globl  exit
exit:
    li      $v0, _SYSEXIT
    syscall
    jr      $ra

# unsigned int wait(unsigned int *code)
    .globl  wait
wait:
    li      $v0, _SYSWAIT
    syscall
    jr      $ra