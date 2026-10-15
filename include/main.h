/*
 * Kernel entry point after boot.s is executed
 *
 * Implemented in main.c
 * - pclava 13 oct 2026
 */

#pragma once
#include <stdint.h>
#include "proc.h"
#include "vga.h"

#define KERNEL_NAME "yeast"
#define KERNEL_MSG "(" ANSI_COLOR_YELLOW KERNEL_NAME ANSI_COLOR_RESET ") "

void kernel_terminate(int32_t code);
void start(void);