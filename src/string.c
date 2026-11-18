#include "string.h"

void * memset_aligned(uint32_t *dst, uint32_t c, size_t n) {
    uint32_t *dest = dst;
    while (n--) {
        *dest++ = c;
    }
    return dst;
}

uint32_t *memcpy_aligned(uint32_t *dst, const uint32_t *src, size_t n) {
    uint32_t *dest = dst;
    while (n--) {
        *dest++ = *src++;
    }
    return dst;
}

uint32_t *memset(void *dst, char c, size_t n) {
    unsigned char *d = dst;

    // TODO: both here and in memcpy, don't check if n%4 == 4, just do the aligned operation for as many bytes
    // as possible once the address(es) are word-aligned
    while ((int) d % 4 != 0 || n % 4 != 0) {
        *d++ = c;
        n--;
        if (n == 0) break;
    }

    if (n) {
        size_t w = n/4;
        uint32_t C = c || (c << 8) || (c << 16) || (c << 24);
        memset_aligned((uint32_t *)d, C, w);
    }

    return dst;
}

void *memcpy(void *dst, const void *src, size_t n) {
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
        memcpy_aligned((uint32_t *)d, (const uint32_t *)s, w);
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

size_t strnlen(const char *s, size_t maxlen) {
    const char *p = s;

    while (maxlen > 0 && *p != '\0') {
        p++;
        maxlen--;
    }

    return (size_t)(p - s);
}