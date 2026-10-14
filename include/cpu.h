/*
 * Miscellaneous set of low-level CPU-related functions,
 * including interacting with C0 registers
 *
 * Implemented in cpu.s
 * - pclava 13 oct 2026
 */

#pragma once
#include <stdint.h>
#include "proc.h"

/*
 * Switches between contexts. Calling swtch saves $ra.
 * so we always return to the last swtch.
 * In this sense, swtch "pauses" a context.
 */
void swtch(struct context *old, struct context *new);

// Saves the given trapframe in TRAPFRAME
void register_trapframe(struct trapframe *trapframe);

// Returns pointer to currently registered trapframe
struct trapframe *get_trapframe();

// Enables interrupts
void int_on();

// Disables interrupts
void int_off();

// Set CPU mode to USER or KERNEL
void set_user_mode();
void set_kernel_mode();

// Returns current mode (0 = USER, 1 = KERNEL)
int get_mode();

// Returns interrupt code
int get_interrupt();

// Sets the address in the EPC to return to upon exception return (eret)
void set_epc(uint32_t addr);

// Gets the address in EPC
uint32_t get_epc();

// Returns the exception code from the cause register (see trap.h)
unsigned char get_cause();

// Returns currently registered process ID
unsigned char get_asid();

// Registers the current process ID
void set_asid(unsigned char asid);

// Sets the index in the TLB for TLB writes and reads
void set_index(uint32_t index);

// Reads/sets the VPN2 in EntryHi
uint32_t read_vpn();
void set_vpn(uint32_t);

// Reads/sets the page table entry in EntryLo0/EntryLo1
uint32_t read_entrylo0();
uint32_t read_entrylo1();
void set_entrylo0(uint32_t pte);
void set_entrylo1(uint32_t pte);

// Gets the virtual address associated with a memory exception
uint32_t get_vaddr();
