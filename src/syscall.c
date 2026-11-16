#include "syscall.h"
#include "cpu.h"
#include "lock.h"
#include "main.h"
#include "printk.h"
#include "memlayout.h"
#include "tty.h"

static uint32_t argraw(const int n) {
    const struct trapframe *trapframe = get_trapframe();
    // ReSharper disable once CppDFAUnreachableCode
    switch (n) {
        case 0:
            return trapframe->a0;
        case 1:
            return trapframe->a1;
        case 2:
            return trapframe->a2;
        case 3:
            return trapframe->a3;
        default:
            panic("argraw");
            return -1;
    }
}

/* === PROCESS SYSCALLS === */

static uint32_t sys_sbrk(void) {
    size_t n = (size_t) argraw(0);

    struct proc *p = getproc();

    if (n % 4 != 0) n += 4 - (n % 4); // Round up to nearest multiple of 4
    if ((size_t) p->pbrk + n >= HEAPLIM) {
        // Can't allocate
        panic("out of heap space");
    }

    // Note: lazy allocation
    p->pbrk = p->pbrk + n;
    return (uint32_t) p->pbrk;
}

static uint32_t sys_termctl(void) {
    enum termop op = (enum termop) argraw(0);
    return term_op(op);
}

static uint32_t sys_fork(void) {
    return fork();
}

static uint32_t sys_exit(void) {
    uint32_t code = (uint32_t) argraw(0);
    exit(code);
    return -1; // shouldn't actually return but like what if? y'know?
}

static uint32_t sys_wait(void) {
    uint32_t *code = (uint32_t *) argraw(0);
    return wait(code);
}

static uint32_t sys_sleep(void) {
    unsigned int milliseconds = (unsigned int) argraw(0);
    unsigned int n = milliseconds / TICK;

    unsigned int ticks0 = cpu.ticks;
    acquire();
    while (cpu.ticks-ticks0 < n) {
        sleep_prepare(&cpu.ticks);
        release();
        sleep();
        acquire();
    }
    release();
    return 0;
}

/* === FILE SYSCALLS === */

// Find a free file in the processs's file table and make it point to the given file
// This "gives" the file to the process and returns its file descriptor
static int fdalloc(struct file *f) {
    struct proc *p = getproc();
    for (int fd = 0; fd < NOPEN; fd++) {
        if (p->ofiles[fd] == 0) {
            p->ofiles[fd] = f;
            return fd;
        }
    }
    return -1;
}

// TODO: rewrite
static uint32_t sys_read(void) {
    const int fd = (int) argraw(0);
    void *buffer = (void *)argraw(1);
    const int n = (int) argraw(2);

    if (fd != 0) {
        panic("bad file descriptor"); // TODO: write error to stdout
    }

    // FD is stdin
    return term_read(buffer, n);
}

// TODO: rewrite
static uint32_t sys_write(void) {
    const int fd = (int) argraw(0);
    void *buffer = (void *)argraw(1);
    const int n = (int) argraw(2);

    if (fd == 1 || fd == 2) {
        term_write(buffer, n);
        return n;
    }

    panic("bad file descriptor");
}

static uint32_t sys_exec(void) {
    char *path = (char *) argraw(0);
    char **argv = (char **) argraw(1);

    // Count args
    int argc = 0;
    while (argv[argc] != NULL) argc++;

    return exec(path, argc, argv);
}

static uint32_t (*syscalls[])(void) = {
    [SYS_sbrk]          = sys_sbrk,
    [SYS_exit]          = sys_exit,
    [SYS_read]          = sys_read,
    [SYS_write]         = sys_write,
    [SYS_termctl]       = sys_termctl,
    [SYS_exec]          = sys_exec,
    [SYS_fork]          = sys_fork,
    [SYS_wait]          = sys_wait,
    [SYS_sleep]         = sys_sleep,
};

void syscall() {
    struct trapframe *trapframe = get_trapframe();

    const uint32_t num = trapframe->v0;

    if (num > 0 && num <= SYSDOMAIN && syscalls[num]) {
        trapframe->v0 = syscalls[num]();
    } else {
        printk("Unknown syscall: ");
        if (num <= 0) {
            printk("Less than 0\n\r");
        }
        if (num > SYSDOMAIN) {
            printk("Too big (%u)\n\r", num);
        }
        if (!syscalls[num]) {
            printk("Not implemented\n\r");
        }
        trapframe->v0 = -1;
    }
}
