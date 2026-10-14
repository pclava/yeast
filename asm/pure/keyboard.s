# ===
#   Code for handling the keyboard and mapping scancodes to keycodes
#   Implementation of keyboard.h
#
#   - pclava 13 oct 2026
# ===


    .kdata

# SDL_Scancode to ASCII mapping
KEYMAP:
    .byte 0 0 0 0                               # 0 - 3
    .byte "a" "b" "c" "d" "e" "f" "g" "h"       # 4 - 11        LATIN LETTERS 4-29
    .byte "i" "j" "k" "l" "m" "n" "o" "p"       # 12 - 19
    .byte "q" "r" "s" "t" "u" "v" "w" "x"       # 20 - 27
    .byte "y" "z" "1" "2" "3" "4" "5" "6"       # 28 - 35       DIGITS 30-39
    .byte "7" "8" "9" "0" "\n" 0x1b "\b" "\t"   # 36 - 43       return, escape, backspace, tab, space
    .byte " " "-" "=" "[" "]" "\\" "\\" ";"     # 44 - 51       SYMBOLS
    .byte "'" "`" "," "." "/" 0 0 0             # 52 - 59
    .byte 0 0 0 0 0 0 0 0                       # 60 - 67
    .byte 0 0 0 0 0 0 0 0                       # 68 - 75
    .byte "\b" 0 0 0 0 0 0 0                    # 76 - 83 (Arrow keys are 79-92: RLDU)
    .byte "/" "*" "-" "+" 0x8 "1" "2" "3"       # 84 - 91
    .byte "4" "5" "6" "7" "8" "9" "0" "."       # 92 - 99
    .byte "\\" 0 0 "=" 0 0 0 0                  # 100 - 107
    .byte 0 0 0 0 0 0 0 0                       # 108 - 115
    .byte 0 0 0 0 0 0 0 0                       # 116 - 123
    .byte 0 0 0 0 0 0 0 0                       # 124 - 131
    .byte 0 0 0 0 0 0 0 0                       # 132 - 139
    .byte 0 0 0 0 0 0 0 0                       # 140 - 147
    .byte 0 0 0 0 0 0 0 0                       # 148 - 155
    .byte 0 0 0 0 0 0 0 0                       # 156 - 163
    .byte 0 0 0 0 0 0 0 0                       # 164 - 171
    .byte 0 0 0 0 0 0 0 0                       # 172 - 179
    .byte 0 0 0 0 0 0 0 0                       # 180 - 187
    .byte 0 0 0 0 0 0 0 0                       # 188 - 195
    .byte 0 0 0 0 0 0 0 0                       # 196 - 203
    .byte 0 0 0 0 0 0 0 0                       # 204 - 211
    .byte 0 0 0 0 0 0 0 0                       # 212 - 219
    .byte 0 0 0 0 0 0 0 0                       # 220 - 227
    .byte 0 0 0 0 0 0 0 0                       # 228 - 235
    .byte 0 0 0 0 0 0 0 0                       # 236 - 243
    .byte 0 0 0 0 0 0 0 0                       # 244 - 251
    .byte 0 0 0 0                               # 252 - 255

.globl keyboard_state
keyboard_state:
    .space  16

    .ktext
#uint8_t key_from_scancode(uint8_t scancode)
.globl key_from_scancode
key_from_scancode:
    la      $v0, KEYMAP                     # Get address of keymap
    addu    $v0, $v0, $a0                   # Use scancode as index in keymap
    lbu     $v0, 0($v0)                     # Read from keymap
    jr      $ra