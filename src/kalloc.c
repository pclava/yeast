// Source: https://github.com/mit-pdos/xv6-riscv/blob/riscv/kernel/kalloc.c

// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include <stdint.h>

/*
 * Physical user memory is 4 Mb from
 *      0x00000000 -> 0x003fffff (inclusive)
 * The virtual addresses in kernel space
 *      0x8f000000 -> 0x8f3fffff (inclusive)
 * are mapped linearly to physical user memory
 * the kernel can write to these addresses to manipulate physical user memory and bypass the TLB
 */

#include "kalloc.h"

#include <stddef.h>

#include "main.h"
#include "memlayout.h"
#include "printk.h"
#include "vga.h"
#include "vm.h"

// Fills in n-many words `c` into word-aligned address `dst`
// Significantly faster than simple memset
static void * memset_aligned(uint32_t *dst, uint32_t c, size_t n) {
    uint32_t *dest = dst;
    while (n--) {
        *dest++ = c;
    }
    return dst;
}

static void freerange(void *pa_start, void *pa_end)
{
    char *p;
    p = (char *)(uint32_t)pa_start;
    for (; p + PAGE_SIZE <= (char *)pa_end; p += PAGE_SIZE)
        kfree(p);
}

struct run {
    struct run *next;
};

static struct {
    struct run *freelist;
} kmem ;

void kinit() {
    freerange((void *)UMEM_START, (void *)UMEM_LIMIT);
}

// Frees the page starting at user physical address pa
// Must have been allocated with `kalloc`,
// with the exception of when called by `freerange`
// during initialization
void kfree(void *pa) {
    struct run *r;

    // should be page aligned and within user memory
    if (((uint32_t)pa % PAGE_SIZE) != 0 || (uint32_t)pa < UMEM_START || (uint32_t)pa > UMEM_LIMIT)
        panic("kfree");

    pa += UMEM_ALIASDIFF;

    // Fill with junk
    memset_aligned((uint32_t *) pa, 0x01010101, PAGE_SIZE / sizeof(uint32_t));

    // alias and cast to a run ptr
    r = (struct run *)pa;

    // Insert at the head of the list
    r->next = kmem.freelist;
    kmem.freelist = r;
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
    struct run *r;

    r = kmem.freelist;
    if (r)
        kmem.freelist = r->next;

    if (r) {
        // Fill with junk
        memset_aligned((uint32_t *)r, 0x05050505, PAGE_SIZE/4);

        // Alias for return
        r = (struct run*)((uint32_t) r - UMEM_ALIASDIFF);
    }
    return (void *)(r);
}