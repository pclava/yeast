#include "trap.h"
#include "cpu.h"
#include "tty.h"
#include "main.h"
#include "memlayout.h"
#include "printk.h"
#include "syscall.h"
#include "keyboard.h"
#include "vga.h"
#include "vm.h"

// Returns the value stored in KEY_REG
// Called by int_keydown
static unsigned char read_key_register(void) {
    return *(unsigned char *)KEY_REG;
}

static void intr_keydown(void) {
    // Read character from register
    const uint8_t scancode = read_key_register();

    // Shift
    if (scancode == 225 || scancode == 229) {
        keyboard_state.shift = 1;
        return;
    }
    // Caps lock
    if (scancode == 57) {
        keyboard_state.caps = !keyboard_state.caps;
        return;
    }
    // Control
    if (scancode == 224 || scancode == 228) {
        keyboard_state.control = 1;
        return;
    }

    uint8_t c = key_from_scancode(scancode);

    // Control characters
    if (keyboard_state.control) {
        c &= 0x1f;
    }

    // Send to terminal
    term_intr(c);
}

void intr_keyup(void) {
    const uint8_t scancode = read_key_register();
    if (scancode == 225 || scancode == 229) {
        keyboard_state.shift = 0;
    }
    if (scancode == 224 || scancode == 228) {
        keyboard_state.control = 0;
    }
}

void prepare_return(void) {
    struct proc *p = getproc();
    p->trapframe->kernel_sp = KSTACK0-((p->PID+1)*PAGE_SIZE);
    prepare_return_pure();
}

void usertrap(void) {
    // This code occurs in the process's kernel context
    // The process's kernel context ONLY exists inside this function
    // The kernel stack will be emptied after every return to user space

    // Get cause
    const enum ExcCode cause = get_cause();

    // Get trapframe
    struct trapframe *trapframe = get_trapframe();

    int kind = 0;

    // Check cause
    if (cause == SYSCALL) {
        // Syscall
        trapframe->pc += 4;
        int_on();
        syscall();
    }
    else if (cause == INTERRUPT) {
        // Interrupt
        kind = devintr();
        if (kind == 0) {
            panic("devintr");
        }
    }
    else {
        switch (cause) {
            case TLB_MOD:
                tlb_mod();
                break;
            case TLBS_MISS:
                tlb_store_miss();
                break;
            case TLBL_MISS:
                tlb_load_miss();
                break;
            default:
                trapframe->pc += 4;
                panic("Exception"); // should just kill the process but atm there's only 1 process
        }
    }

    // Was a timer interrupt
    if (kind == 2) {
        // yield
        yield();
        // once control is handed back to the user, we end up here!
    }

    prepare_return();
    userret();

}

void kerneltrap(void) {
    // Check cause
    const enum ExcCode cause = get_cause();

    // Check cause
    if (cause == INTERRUPT) {
        // Handle interrupt
        devintr();
    } else {
        switch (cause) {
            case TLB_MOD:
                tlb_mod();
                break;
            case TLBS_MISS:
                tlb_store_miss();
                break;
            case TLBL_MISS:
                tlb_load_miss();
                break;
            default:
                printk(ANSI_COLOR_RED "cause: %d | vaddr: %p\n\r" ANSI_COLOR_RESET, cause, (void *)get_vaddr());
                panic("Non-interrupt trap occurred in kernel space");
        }
    }
}

int devintr(void) {
    const enum Interrupt cause = get_interrupt();

    switch (cause) {
        case TIMER_INTERRUPT:
            return 2; // usertrap will handle
        case KEY_DOWN:
            intr_keydown();
            return 1;
            break;
        case KEY_UP:
            intr_keyup();
            return 1;
        case NO_INTERRUPT:
        case DISPLAY_REFRESH:
        default:
            printk(KERNEL_MSG "Unrecognized interrupt %u\n\r", cause);
    }
    return 0;
}
