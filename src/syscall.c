#include "syscall.h"
#include "cpu.h"
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

static uint32_t sys_printint(void) {
    const int num = (int) argraw(0);
    printk("%d", num);
    return SYS_printint;
}

static uint32_t sys_printstr(void) {
    const char *str = (char *) argraw(0);
    printk("%s", str);
    return SYS_printstr;
}

static uint32_t sys_readstr(void) {
    uint8_t *str = (uint8_t *) argraw(0);
    const int len = (int) argraw(1);
    const int read = term_read(str, len-1);
    str[read] = '\0';
    return read;
}

static uint32_t sys_sbrk(void) {
    size_t n = (size_t) argraw(0);

    struct proc *p = getproc();

    if (n % 4 != 0) n += 4 - (n % 4); // Round up to nearest multiple
    if ((size_t) p->pbrk + n >= HEAPLIM) {
        // Can't allocate
        panic("out of heap space");
    }

    p->pbrk = p->pbrk + n;
    return (uint32_t) p->pbrk;
}

static uint32_t sys_printchr(void) {
    const char c = (char) argraw(0);
    printk("%c", c);
    return SYS_printchr;
}

static uint32_t sys_readfile(void) {
    const int fd = (int) argraw(0);
    void *buffer = (void *)argraw(1);
    const int n = (int) argraw(2);

    if (fd != 0) {
        panic("bad file descriptor"); // TODO: write error to stdout
    }

    // FD is stdin
    return term_read(buffer, n);
}

static uint32_t sys_writefile(void) {
    const int fd = (int) argraw(0);
    void *buffer = (void *)argraw(1);
    const int n = (int) argraw(2);

    if (fd == 1 || fd == 2) {
        term_write(buffer, n);
        return n;
    }

    panic("bad file descriptor");
}

static uint32_t sys_termctl(void) {
    enum termop op = (enum termop) argraw(0);
    return term_op(op);
}

static uint32_t sys_exec(void) {
    // Replace contents of current process with a new one
    uint32_t code = 0x08100000; // infinite loop

    uint32_t *start = (uint32_t *)UTEXT0;
    *(uint32_t *)STACK0 = argraw(0);
    *start = code;

    // Setup trapframe
    struct proc *p = getproc();
    p->trapframe->pc = UTEXT0;
    p->trapframe->sp = STACK0-4; // 1 argument has been placed already
    p->pbrk = (void *)HEAP0;

    return 0;
}

static uint32_t sys_fork(void) {
    return fork();
}

static uint32_t (*syscalls[])(void) = {
    [SYS_printint]      = sys_printint,
    [SYS_printstr]      = sys_printstr,
    [SYS_readstr]       = sys_readstr,
    [SYS_sbrk]          = sys_sbrk,
    [SYS_printchr]      = sys_printchr,
    [SYS_readfile]      = sys_readfile,
    [SYS_writefile]     = sys_writefile,
    [SYS_termctl]       = sys_termctl,
    [SYS_exec]          = sys_exec,
    [SYS_fork]          = sys_fork,
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
            printk("Too big\n\r");
        }
        if (!syscalls[num]) {
            printk("Not implemented\n\r");
        }
        trapframe->v0 = -1;
    }
}