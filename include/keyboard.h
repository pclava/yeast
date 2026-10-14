/*
 * Keyboard interface
 *
 * Implemented in keyboard.s
 * - pclava 13 oct 2026
 */

#pragma once
#include <stdint.h>

// Converts a received scancode into ASCII
uint8_t key_from_scancode(uint8_t scancode);

// Global keyboard state
extern struct {
    uint8_t shift;
    uint8_t control;
    uint8_t caps;
} keyboard_state;