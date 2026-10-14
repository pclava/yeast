/*
 * Terminal interface code
 *
 * The terminal is based on VGA mode 7
 * It is 80x25 with characters 9x14 pixels
 * More information in vga.h
 *
 * The terminal keeps an input buffer of 128 characters.
 * When it fills up or receives a newline, it sends the buffer to any waiting term_read()
 * If none are waiting, the buffer is held full and no keys can be received unless the terminal
 * is flushed using term_op(TTY_FLUSH)
 *
 * Implemented in tty.c
 * - pclava 13 oct 2026
 */


#pragma once
#include <stddef.h>
#include <stdint.h>

// Operations to interact with the terminal from user space using sys_termctl()
enum termop {
    TTY_CLEAR,  // Clears terminal, sets cursor to (0, 0)
    TTY_FLUSH,  // Flushes input buffer
};

int term_read(uint8_t *dst, int n);                                 // Reads from input buffer. This puts a process to sleep until it a line is sent!
void term_intr(uint8_t c);                                          // Called by intr_keydown() in trap.c: handles keypresses
void term_setcolor(uint8_t color);                                  // Sets text color (see vga.h for ANSI color codes)
void term_init(void);                                               // Called by main(): initializes terminal and state variables
void term_putentryat(char c, uint8_t color, size_t x, size_t y);    // Writes to terminal screen
void term_putchar(char c);                                          // Puts character at current cursor with current color
void term_write(const char* data, size_t size);                     // Prints from a buffer
void term_writestring(const char *data);                            // Prints from a null-terminated string
void term_clear(uint8_t color);                                     // Fills the terminal with spaces with the given vga_entry_color. Use 0 for black-on-black
void term_echo(uint8_t v);                                          // Takes boolean argument: disables or enables terminal echoing (when a key is pressed)
int term_op(enum termop op);                                        // Called by sys_termctl(): executes terminal operation