#include "vga.h"
#include "tty.h"
#include "main.h"
#include "keyboard.h"
#include "memlayout.h"
#include "printk.h"
#include "proc.h"

// Default terminal color (white on black)
#define TERM_COLOR_DEF vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK)

enum STATE {
    NORMAL,
    ESCAPE,
    COMMAND
};

// Terminal state variables
size_t term_row;    // Cursor y
size_t term_col;    // Cursor x
uint16_t *term_buffer = (uint16_t *)VGA_MEMORY; // Buffer location
uint8_t term_color; // Cursor color
enum STATE term_state;  // Modifiers
uint32_t command;   // Active command
uint8_t echo;   // Echo enabled

// Starts at 30 (fg) or 40 (bg)
enum vga_color ansi_to_vga[] = {
    VGA_COLOR_BLACK,
    VGA_COLOR_RED,
    VGA_COLOR_GREEN,
    VGA_COLOR_BROWN,
    VGA_COLOR_BLUE,
    VGA_COLOR_MAGENTA,
    VGA_COLOR_CYAN,
    VGA_COLOR_LIGHT_GREY,
};

// Starts at 90 (fg) or 100 (bg)
enum vga_color ansi_to_vga_bright[] = {
    VGA_COLOR_DARK_GREY,
    VGA_COLOR_LIGHT_RED,
    VGA_COLOR_LIGHT_GREEN,
    VGA_COLOR_YELLOW,
    VGA_COLOR_LIGHT_BLUE,
    VGA_COLOR_LIGHT_MAGENTA,
    VGA_COLOR_LIGHT_CYAN,
    VGA_COLOR_WHITE,
};

#define C(x) ((x) & 0x1f) // get control key from uppercase letter

static void * memmove(void *vdst, const void *vsrc, int n) {
    char *dst;
    const char *src;

    dst = vdst;
    src = vsrc;
    if (src > dst) {
        while (n-- > 0)
            *dst++ = *src++;
    } else {
        dst += n;
        src += n;
        while (n-- > 0)
            *--dst = *--src;
    }
    return vdst;
}

// Circular input buffer
// We use 32 bit indices in the hopes that the user does not type 4 billion keys
struct {
    #define INPUT_BUF_SIZE 128
    char buf[INPUT_BUF_SIZE];
    uint32_t r; // read
    uint32_t w; // write
    uint32_t e; // edit
} cons;

int term_read(uint8_t *dst, int32_t n) {
    const uint32_t target = n;

    while (n > 0) {
        // Wait until write pointer has advanced (int handler has written something)
        while (cons.r == cons.w) {
            sleep(&cons);
        }

        const char c = cons.buf[cons.r++ % INPUT_BUF_SIZE]; // Read char and increment read ptr

        *dst++ = c;

        dst++;
        --n;

        if (c == '\n') {
            // Finished
            break;
        }
    }
    return (int32_t) target-n;
}

static char shift(uint8_t c) {
    if (c >= 'a' && c <= 'z') {
        return (char) (c - 32);
    }

    static const char shift_lookup[128] = {
        ['1'] = '!', ['2'] = '@', ['3'] = '#', ['4'] = '$', ['5'] = '%',
        ['6'] = '^', ['7'] = '&', ['8'] = '*', ['9'] = '(', ['0'] = ')',
        ['-'] = '_', ['='] = '+', ['['] = '{', [']'] = '}', [';'] = ':',
        ['\''] = '"', [','] = '<', ['.'] = '>', ['/'] = '?', ['`'] = '~',
        ['\\'] = '|'
    };

    if (shift_lookup[c] != 0) {
        return shift_lookup[c];
    }

    return (char) c;
}

// Called on keydown interrupt. `c` can be anything mapped in the keymap in keyboard.s
void term_intr(uint8_t c) {
    // Apply modifiers
    if (keyboard_state.shift) {
        c = shift(c);
    }
    if (keyboard_state.caps || keyboard_state.shift) {
        if (c >= 0x61 && c <= 0x7a) {
            c -= 0x20;
        }
    }

    switch (c) {
        case C('P'):    // debug stuff
            procdump();
            break;
        case C('U'):    // Kills the line
            while (cons.e != cons.w &&
                cons.buf[(cons.e - 1) % INPUT_BUF_SIZE] != '\n') {
                cons.e--;
                term_putchar('\b');
            }
            break;
        case C('['):    // Escape
            kernel_terminate(0);
            break;
        case C('H'):    // Backspace
        case '\x7f':    // Delete
            if (cons.e != cons.w) {
                cons.e--;
                term_putchar('\b');
            }
            break;
        default:
            // if character is not zero and there is space in buffer
            if (c != 0 && cons.e - cons.r < INPUT_BUF_SIZE) {
                // Echo to user
                if (echo) term_putchar((char) c);

                // Store in buffer
                cons.buf[cons.e++ % INPUT_BUF_SIZE] = (char) c;

                if (c == '\n' && echo) term_putchar('\r'); // Also send carriage return

                // If newline or no more buffer space, "write" the entry
                if (c == '\n' || c == C('D') || cons.e - cons.r == INPUT_BUF_SIZE) {
                    cons.w = cons.e;
                    wakeup(&cons);
                }
            }
            break;
    }

}

static size_t strlen(const char* str)
{
    size_t len = 0;
    while (str[len])
        len++;
    return len;
}

void term_setcolor(const uint8_t color)
{
    term_color = color;
}

void term_init(void)
{
    term_row = 0;
    term_col = 0;
    term_color = TERM_COLOR_DEF;

    command = 0;
    echo = 0;

    cons.r = 0;
    cons.w = 0;
    cons.e = 0;
    for (int i = 0; i < INPUT_BUF_SIZE; i++) {
        cons.buf[i] = -1;
    }

    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            const size_t index = y * VGA_WIDTH + x;
            term_buffer[index] = vga_entry(' ', 0);
        }
    }
}

void term_putentryat(const char c, const uint8_t color, const size_t x, const size_t y)
{
    const size_t index = y * VGA_WIDTH + x;
    term_buffer[index] = vga_entry(c, color);
}

// Processes the command sent via escape sequence
static void read_command() {
    if (command == 0) {
        return term_setcolor(TERM_COLOR_DEF);
    }

    enum vga_color fg = vga_fg(term_color);
    enum vga_color bg = vga_bg(term_color);

    // Background
    if (command >= 40 && command <= 47) {
        bg = ansi_to_vga[command-40];
    }
    // Light background
    else if (command >= 100 && command <= 107) {
        bg = ansi_to_vga_bright[command-100];
    }
    // Foreground
    else if (command >= 30 && command <= 37) {
        fg = ansi_to_vga[command-30];
    }
    // Light foreground
    else if (command >= 90 && command <= 97) {
        fg = ansi_to_vga_bright[command-90];
    }

    term_setcolor(vga_entry_color(fg, bg));
    command = 0;
}

// Called when cursor reaches bottom of terminal
static void scroll() {
    memmove(term_buffer, term_buffer + VGA_WIDTH, (VGA_BUFFER_SIZE - VGA_WIDTH) * sizeof(uint16_t));
    term_row -= 1;
    uint8_t old_colors = term_color;
    term_color = TERM_COLOR_DEF;
    for (int i = 0; i < VGA_WIDTH; i++) {
        term_putentryat(' ', term_color, i, term_row);
    }
    term_color = old_colors;
}

void term_putchar(const char c)
{
    // these shouldn't be done here probably, because then you can type it if you allow the user to type escape
    if (term_state == ESCAPE) {
        if (c == '[') {
            term_state = COMMAND;
            return;
        }
        term_state = NORMAL; // Fall down
    }

    else if (term_state == COMMAND) {
        if (c == 'm') {
            read_command();
            term_state = NORMAL;
            return;
        }

        if (c >= '0' && c <= '9') {
            command *= 10;
            command += c - '0';
            return;
        }

        term_state = NORMAL;
        // Fall into main switch statement below
    }

    switch (c) {
        case '\n':
            term_row++;
            if (term_row >= VGA_HEIGHT) scroll();
            break;
        case '\r':
            term_col = 0;
            break;
        case 0x1b:
            term_state = ESCAPE;
            break;
        case '\b':
            // not sure what the behavior should be here.
            // right now it stops you at the left end
            if (term_col == 0) {
                term_col = 0;
            } else {
                term_col--;
            }
            term_putentryat(' ', term_color, term_col, term_row);
            break;
        default:
            if (c < 20) return; // skip non-printable
            term_putentryat(c, term_color, term_col, term_row);
            if (++term_col == VGA_WIDTH) {
                term_col = 0;
                if (++term_row == VGA_HEIGHT) scroll();
            }
    }
}

void term_write(const char* data, const size_t size)
{
    for (size_t i = 0; i < size; i++)
        term_putchar(data[i]);
}

void term_writestring(const char *data)
{
    term_write(data, strlen(data));
}

void term_clear(const uint8_t color) {
    for (int y = 0; y < VGA_HEIGHT; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            term_putentryat(' ', color, x, y);
        }
    }
    term_row = 0;
    term_col = 0;
}

void term_echo(uint8_t v) {
    echo = v;
}

static void term_flush(void) {
    cons.w = cons.r = cons.e;
}

// Terminal operation. Performed through termctl() syscall
int term_op(enum termop op) {
    switch (op) {
        case TTY_CLEAR:
            term_clear(0);
            return 1;
        case TTY_FLUSH:
            term_flush();
            return 1;
        default:
            return 0;
    }
}