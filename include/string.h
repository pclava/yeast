// needs no introduction

#pragma once
#include <stdint.h>
#include <stddef.h>

uint32_t *memset(void *dst, char c, size_t n);
void *memcpy(const void *src, void *dst, size_t n);
void * memset_aligned(uint32_t *dst, uint32_t c, size_t n);
uint32_t *memcpy_aligned(const uint32_t *src, uint32_t *dst, size_t n);
int strncmp(const char *s1, const char *s2, size_t n);
size_t strlen(const char *str);