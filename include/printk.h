
/*
 * Code for printing to terminal from kernel mode
 * Taken from https://github.com/mit-pdos/xv6-riscv/blob/06aad25c735fd3159bdfae5680be4eab7b1668b2/kernel/printk.c
 *
 * Implemented in printk.c
 * - pclava 13 oct 2026
 */

#pragma once

// Print functions used by kernel code
int             printk(char*, ...) __attribute__ ((format (printf, 1, 2)));
void            panic(char*) __attribute__((noreturn));