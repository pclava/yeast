/*
 * Implementation of VGA text mode 7 with ANSI colors
 * See tty.h for more
 *
 * - pclava 13 oct 2026
 */


#pragma once
#include <stdint.h>

#define VGA_HEIGHT 25
#define VGA_WIDTH 80
#define VGA_BUFFER_SIZE (VGA_WIDTH * VGA_HEIGHT)

#define ANSI_COLOR_BLACK            "\x1b[30m"
#define ANSI_COLOR_RED              "\x1b[31m"
#define ANSI_COLOR_GREEN            "\x1b[32m"
#define ANSI_COLOR_YELLOW           "\x1b[33m"
#define ANSI_COLOR_BLUE             "\x1b[34m"
#define ANSI_COLOR_MAGENTA          "\x1b[35m"
#define ANSI_COLOR_CYAN             "\x1b[36m"
#define ANSI_COLOR_WHITE            "\x1b[37m"
#define ANSI_COLOR_BRIGHT_BLACK     "\x1b[90m"
#define ANSI_COLOR_BRIGHT_RED       "\x1b[91m"
#define ANSI_COLOR_BRIGHT_GREEN     "\x1b[92m"
#define ANSI_COLOR_BRIGHT_YELLOW    "\x1b[93m"
#define ANSI_COLOR_BRIGHT_BLUE      "\x1b[94m"
#define ANSI_COLOR_BRIGHT_MAGENTA   "\x1b[95m"
#define ANSI_COLOR_BRIGHT_CYAN      "\x1b[96m"
#define ANSI_COLOR_BRIGHT_WHITE     "\x1b[97m"
#define ANSI_BG_BLACK               "\x1b[40m"
#define ANSI_BG_RED                 "\x1b[41m"
#define ANSI_BG_GREEN               "\x1b[42m"
#define ANSI_BG_YELLOW              "\x1b[43m"
#define ANSI_BG_BLUE                "\x1b[44m"
#define ANSI_BG_MAGENTA             "\x1b[45m"
#define ANSI_BG_CYAN                "\x1b[46m"
#define ANSI_BG_WHITE               "\x1b[47m"
#define ANSI_BG_BRIGHT_BLACK        "\x1b[100m"
#define ANSI_BG_BRIGHT_RED          "\x1b[101m"
#define ANSI_BG_BRIGHT_GREEN        "\x1b[102m"
#define ANSI_BG_BRIGHT_YELLOW       "\x1b[103m"
#define ANSI_BG_BRIGHT_BLUE         "\x1b[104m"
#define ANSI_BG_BRIGHT_MAGENTA      "\x1b[105m"
#define ANSI_BG_BRIGHT_CYAN         "\x1b[106m"
#define ANSI_BG_BRIGHT_WHITE        "\x1b[107m"
#define ANSI_COLOR_RESET            "\x1b[0m"

// ANSI colors get converted to VGA colors in tty.c
enum vga_color {
    VGA_COLOR_BLACK = 0,
    VGA_COLOR_BLUE = 1,
    VGA_COLOR_GREEN = 2,
    VGA_COLOR_CYAN = 3,
    VGA_COLOR_RED = 4,
    VGA_COLOR_MAGENTA = 5,
    VGA_COLOR_BROWN = 6,
    VGA_COLOR_LIGHT_GREY = 7,
    VGA_COLOR_DARK_GREY = 8,
    VGA_COLOR_LIGHT_BLUE = 9,
    VGA_COLOR_LIGHT_GREEN = 10,
    VGA_COLOR_LIGHT_CYAN = 11,
    VGA_COLOR_LIGHT_RED = 12,
    VGA_COLOR_LIGHT_MAGENTA = 13,
    VGA_COLOR_YELLOW = 14,
    VGA_COLOR_WHITE = 15,
};

// Returns formatted color byte with bg in the upper bits and fg in the lower bits
static inline uint8_t vga_entry_color(enum vga_color fg, enum vga_color bg)
    {return fg | bg << 4;}

// Returns 2-byte vga entry with color in the upper byte and ascii code in the lower byte
static inline uint16_t vga_entry(unsigned char uc, uint8_t color)
    {return (uint16_t) uc | (uint16_t) color << 8;}

// Extracts the foreground from a formatted color byte
static inline enum vga_color vga_fg(uint8_t entry_color)
    {return (enum vga_color) (entry_color & 0xF);}

// Extracts the background from a formatted color byte
static inline enum vga_color vga_bg(uint8_t entry_color)
    {return (enum vga_color) (entry_color >> 4);}