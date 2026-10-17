#pragma once

// === TTY ===
enum termop {
    TTY_CLEAR,
    TTY_FLUSH,
};

// === SYSCALLS (usys.s) ===
int fork(void);
void exec(int arg);    // TODO: temporary!
int write(int fd, const void *buffer, int n);
int read(int fd, void *buffer, int n);
char *sbrk(int n);
void termctl(enum termop op);
void exit(unsigned int code);
unsigned int wait(unsigned int *code);

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