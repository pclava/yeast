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

#include "main.h"
#include "memlayout.h"
#include "printk.h"
#include "vga.h"
#include "vm.h"

static void *
memset(void *dst, int c, unsigned int n)
{
    char *cdst = (char *)dst;
    int i;
    for (i = 0; i < n; i++) {
        cdst[i] = c;
    }
    return dst;
}

static void
freerange(void *pa_start, void *pa_end)
{
    char *p;
    p = (char *)(uint32_t)pa_start;
    for (; p + PAGE_SIZE <= (char *)pa_end; p += PAGE_SIZE)
        kfree(p);
}

struct run {
    struct run *next;
};

struct __attribute__((aligned(4))) {
    struct run *freelist;
} kmem ;

void
kinit()
{
    freerange((void *)UMEM_START, (void *)UMEM_LIMIT);
}

// Free the page of physical memory pointed at by pa,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
    struct run *r;

    if (((uint32_t)pa % PAGE_SIZE) != 0 || (uint32_t)pa < UMEM_START || (uint32_t)pa > UMEM_LIMIT)
        panic("kfree");

    // Fill with junk to catch dangling refs. This is absurdly slow, so I'm not sure if it's necessary
    // memset(pa, 1, PAGE_SIZE);

    pa += UMEM_ALIASDIFF;
    r = (struct run *)pa;

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
        // memset((char *)r, 5, PAGE_SIZE); // fill with junk. slow
        r = (struct run*)((uint32_t) r - UMEM_ALIASDIFF);
    }
    return (void *)(r);
}