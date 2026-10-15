/*
 * Trap handling code
 * We define a trap as one of:
 *  - Interrupt
 *  - Exception
 *  - Syscall
 * The three go through the same pipeline until usertrap() or kerneltrap()
 * On any exception, the CPU lands in _exc (boot.s), which jumps to usertrap or kerneltrap
 * These functions read the cause and handle the exception
 *
 * Note: timer interrupts are issued every half second by the emulator
 *
 * Implemented in trap.c and trap_pure.s
 * - pclava 13 oct 2026
 */

#pragma once

// Trap codes are written into the $cause register with this mapping
enum ExcCode {
    INTERRUPT = 0,                      // Interrupt
    TLB_MOD = 1,                        // Store occurs on valid entry with dirty bit set to 0. Kernel will check if writes are allowed, and is so set the dirty bit to 1
    TLBL_MISS = 2,                      // Load occurs without a valid entry
    TLBS_MISS = 3,                      // Store occurs without a valid entry
    ADDRESS_ERROR_EXCEPTION_LOAD = 4,   // Bad load
    ADDRESS_ERROR_EXCEPTION_STORE = 5,  // Bad store
    SYSCALL = 8,                        // Syscall
    BREAKPOINT_EXCEPTION = 9,           // Breakpoint (not used)
    ARITHMETIC_OVERFLOW_EXCEPTION = 12, // Arithmetic overflow
    TRAP_EXCEPTION = 13,                // Unspecified trap. Caused by specific trap instructions
};

// Interrupts are written into the $cause register, with each bit from 15 to 8 representing a different interrupt, mapped here
enum Interrupt {
    NO_INTERRUPT =             0,
    DISPLAY_REFRESH =      0b100,  // IP2
    KEY_DOWN =            0b1000,  // IP3
    KEY_UP =             0b10000,  // IP4
    TIMER_INTERRUPT =   0b100000,  // IP5 : Issued periodically by the emulator!
};

/* === trap_pure.s === */

// Returns to user mode from kernel after trap handling
// prepare_return from cpu.h MUST have been called first!
// Restores trapframe and calls eret
void userret();

// Prepares CPU for returning to user mode after trap
void prepare_return_pure();

/* === trap.c === */

void prepare_return();

// Traps in user mode land here after going through exception handler
void usertrap();

// Traps in kernel mode land here after going through exception handler
// Only returns if the trap was an interrupt
void kerneltrap();

// Called by usertrap() or kerneltrap() on interrupt. Returns 2 on timer interrupt
int devintr();