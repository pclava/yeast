#include "string.h"

void * memset_aligned(uint32_t *dst, uint32_t c, size_t n) {
    uint32_t *dest = dst;
    while (n--) {
        *dest++ = c;
    }
    return dst;
}

uint32_t *memcpy_aligned(const uint32_t *src, uint32_t *dst, size_t n) {
    uint32_t *dest = dst;
    while (n--) {
        *dest++ = *src++;
    }
    return dst;
}

uint32_t *memset(void *dst, char c, size_t n) {
    size_t w = n/4;
    size_t left = n%4;
    uint32_t C = c || (c << 8) || (c << 16) || (c << 24);
    memset_aligned(dst, C, left);
    unsigned char *d = dst+(w * sizeof(uint32_t));
    while (left--) {
        *d++ = c;
    }
    return dst;
}

void *memcpy(const void *src, void *dst, size_t n) {
    const unsigned char *s = src;
    unsigned char *d = dst;

    // copy until both addresses are word-aligned
    while (((int) s % 4 != 0) || ((int) d % 4 != 0) || (n % 4 != 0)) {
        *d++ = *s++;
        n--;
        if (n == 0) break;
    }

    // Only once once all word-aligned
    if (n) {
        // copy rest
        size_t w = n/4;
        memcpy_aligned((const uint32_t *)s, (uint32_t *)d, w);
    }

    return dst;
}

int strncmp(const char *s1, const char *s2, size_t n) {
    if (n == 0)
        return (0);
    do {
        if (*s1 != *s2++)
            return (*(unsigned char *)s1 - *(unsigned char *)--s2);
        if (*s1++ == 0)
            break;
    } while (--n != 0);
    return (0);
}

size_t strlen(const char *str) {
    const char *s;
    for (s = str; *s; ++s)
        ;
    return (s - str);
}