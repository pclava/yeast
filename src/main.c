#include <stdint.h>
#include "main.h"
#include "tty.h"
#include "printk.h"
#include "proc.h"
#include "cpu.h"
#include "kalloc.h"
#include "lock.h"
#include "vga.h"

#include "inode.h"

volatile struct cpu cpu = {0};

void kernel_terminate(int32_t code) {
    asm volatile (
        "di\t$t0\n\t"
        "li\t$t0, 0x7FFFFFFF\n\t"
        "mfc0\t$t1, $12\n\t"
        "and\t$t1,$t1,$t0\n\t"
        "move\t$v0, %1\n\t"
        "mtc0\t$t1, $12"
        : "=r" (code)
        : "r" (code)
    );
}

static void kernel_main(void)
{
    push_off();

    // SYSTEM INITIALIZATION
    term_init();
    printk(KERNEL_MSG "Kernel booting\n\r");
    kinit();
    printk(KERNEL_MSG "Physical memory initialized\n\r");
    procinit();
    printk(KERNEL_MSG "Process table initialized\n\r");
    first_proc();
    printk(KERNEL_MSG "First process initialized\n\r");

    pop_off();

    // Enable terminal key echo
    term_echo(1);

    // Jump to scheduler
    scheduler();
}

void start(void)
{
    kernel_main();
}

