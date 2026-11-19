# ===
#   User-level wrapper functions for syscalls
#   Implementation of the syscall functions in user.h
#
#   - pclava 13 oct 2026
# ===

    .text

# char *sbrk(int n)
    .globl  sbrk
sbrk:
    li      $v0, _SYSSBRK
    syscall
    jr      $ra

# void exit(unsigned int code)
    .globl  exit
exit:
    li      $v0, _SYSEXIT
    syscall
    jr      $ra

# void termctl(enum termop op)
    .globl  termctl
termctl:
    li      $v0, _SYSTERMCTL
    syscall
    jr      $ra

# int fork(void)
    .globl  fork
fork:
    li      $v0, _SYSFORK
    syscall
    #break
    jr      $ra

# unsigned int wait(unsigned int *code)
    .globl  wait
wait:
    li      $v0, _SYSWAIT
    syscall
    jr      $ra

# void sleep(unsigned int ms)
    .globl  sleep
sleep:
    li      $v0, _SYSSLEEP
    syscall
    jr      $ra

# int write(int fd, const void *buffer, int n)
    .globl  write
write:
    li      $v0, _SYSWRITE
    syscall
    jr      $ra

# int read(int fd, void *buffer, int n)
    .globl  read
read:
    li      $v0, _SYSREAD
    syscall
    jr      $ra

# int exec(const char *path, char *argv[])
    .globl  exec
exec:
    li      $v0, _SYSEXEC
    syscall
    jr      $ra

# int link(const char *old, const char *new)
    .globl  link
link:
    li      $v0, _SYSLINK
    syscall
    jr      $ra

# int open(const char *path, int mode)
    .globl  open
open:
    li      $v0, _SYSOPEN
    syscall
    jr      $ra

# int mkdir(const char *path)
    .globl  mkdir
mkdir:
    li      $v0, _SYSMKDIR
    syscall
    jr      $ra

# int dup(int oldfd)
    .globl  dup
dup:
    li      $v0, _SYSDUP
    syscall
    jr      $ra

# int chdir(const char *path)
    .globl  chdir
chdir:
    li      $v0, _SYSCHDIR
    syscall
    jr      $ra

# int close(int fd)
    .globl  close
close:
    li      $v0, _SYSCLOSE
    syscall
    jr      $ra

# int unlink(const char *path)
    .globl  unlink
unlink:
    li      $v0, _SYSUNLINK
    syscall
    jr      $ra

# char *getcwd(char *buf)
    .globl getcwd
getcwd:
    li      $v0, _SYSGETCWD
    syscall
    jr      $ra