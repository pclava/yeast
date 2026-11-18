#pragma once

// === TTY ===
enum termop {
    TTY_CLEAR,
    TTY_FLUSH,
};

// === SYSCALLS (usys.s) ===
int fork(void);
int write(int fd, const void *buffer, int n);
int read(int fd, void *buffer, int n);
char *sbrk(int n);
void termctl(enum termop op);
void exit(unsigned int code);
unsigned int wait(unsigned int *code);
void sleep(unsigned int ms);
int exec(const char *path, char *argv[]);
int link(const char *old, const char *new);
int open(const char *path, int mode);
int mkdir(const char *path);
int dup(int oldfd);
int chdir(const char *path);
int close(int fd);
int unlink(const char *path);
char *getcwd(char *buf);

// === ULIB (ulib.c) ===
char *strcpy(char *, const char *);
void *memmove(void *, const void *, int);
char *strchr(const char *, char c);
int strcmp(const char *, const char *);
char *gets(char *, int max);
unsigned int strlen(const char *);
void *memset(void *, int, unsigned int);
int atoi(const char *);
int memcmp(const void *, const void *, unsigned int);
void *memcpy(void *, const void *, unsigned int);

// === PRINTF (printf.c) ===
void fprintf(int fd, const char *fmt, ...);
void printf(const char *fmt, ...);

// === UMALLOC (umalloc.c) ===
void *malloc(unsigned int);
void free(void *);

// === kernel stuff ===
#define KERNEL_NAME "yeast"
#define KERNEL_VERS "0.3.0"

// === ANSI and VGA ===
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
#define VGA_HEIGHT 25
#define VGA_WIDTH 80
#define VGA_BUFFER_SIZE (VGA_WIDTH * VGA_HEIGHT)