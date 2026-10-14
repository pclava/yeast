/*
 * Memory layout definitions
 *
 * - pclava 13 oct 2026
 */

#pragma once

/* === Segments === */
#define KTEXT0          0x80000000  // Base virtual addresses
#define KDATA0          0x90000000
#define UTEXT0          0x00400000
#define UDATA0          0x10010000
#define STACK0          0x7ffffffc
#define STACK_PG0       0x7ffff000  // First page of the stack
#define KSTACK0         0x9ffffffc  // Each context has a 1 page stack at KSTACK0 - PG_SIZE*CONTEXT_ID
#define HEAP0           0x10080000
#define HEAPLIM         0x10400000  // Allow for 3.5 mb of heap, should be enough?

/* === Physical User Memory === */
#define UMEM_START      0x00001000
#define UMEM_LIMIT      0x003fffff
#define UMEM_ALIASDIFF  0x8f000000
#define UMEM_ALIAS      0x8f001000  // Addresses in this space get translated by UMEM_ALIASDIFF:
#define UMEM_ALIASLIMIT 0x8f3fffff  // this allows the kernel to write to physical memory while bypassing the TLB

/* === Kernel Data @ 0x90000000 === */
#define EXC_VEC         0x90000000
#define TRAPFRAME       0x90000004
#define PAGETABLE0      0x91000000  // A process's page table starts at PAGETABLE0 + PID*PT+SIZE
#define PT_SIZE         0x00200000

/* === MMIO @ 0xFFFF0000 === */
#define VGA_MEMORY      0xFFFF0000
#define KEY_REG         0xFFFFFA00  // Single byte! Scancodes get sent here on key interrupt

// temp
#define FILE_REG        0xFFFFFA04  // Word
#define FILE_BUFFER     0x90010000  // Space to put page read from memory