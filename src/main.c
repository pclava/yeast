#include "main.h"
#include "tty.h"
#include "printk.h"
#include "proc.h"
#include "cpu.h"
#include "kalloc.h"
#include "vga.h"

struct cpu cpu = {};

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
    // SYSTEM INITIALIZATION
    term_init();
    printk(KERNEL_MSG "Kernel booting\n\r");
    kinit();
    printk(KERNEL_MSG "Memory initialized\n\r");
    procinit();
    printk(KERNEL_MSG "Process table initialized\n\r");

    int_on(); // Enable interrupts, but they won't come through until userret because EXL is on

    // Initialize first process
    first_proc();

    printk("(" ANSI_COLOR_YELLOW "yeast" ANSI_COLOR_RESET ") Entering normal operation\n\r");

    // Enable terminal press echo
    term_echo(1);

    // Jump to scheduler
    scheduler();
}

void start(void)
{
    kernel_main();
}

