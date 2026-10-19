#include "vm.h"

#include <stddef.h>
#include "memlayout.h"
#include "cpu.h"
#include "kalloc.h"
#include "main.h"
#include "printk.h"
#include "vga.h"

static inline uint32_t find_index(const uint32_t vpn) {
    // return ((vpn >> 12) / 2) % TLB_SIZE;
    return ((((vpn >> 12) / 2) + (3 * get_asid())) * 2654435769u) % TLB_SIZE;
}

void pt_init(unsigned char asid) {
    struct page_table *pt = &page_tables[asid];
    for (uint32_t *i = pt->pages; i < &pt->pages[PAGE_COUNT]; i++) {
        *i = 0;
    }
}

struct page_table *page_tables = (struct page_table *)PAGETABLE0;

void pte_decode(struct PTE *dst, const uint32_t pte) {
    dst->PFN = (pte >> FLAGS_WIDTH) << PAGE_OFFSET;
    dst->dirty = (pte & PTE_D) != 0;
    dst->valid = (pte & PTE_V) != 0;
}

uint32_t pte_encode(const struct PTE *pte) {
    uint32_t ret = 0;
    ret |= ((pte->PFN >> PAGE_OFFSET) << 6);
    ret |= (pte->dirty << 2);
    ret |= (pte->valid << 1);
    return ret;
}

uint32_t pte_lookup(const unsigned char proc, const uint32_t VPN) {
    const struct page_table *pt = &page_tables[proc];
    return pt->pages[VPN >> PAGE_OFFSET];
}

void pte_write(const struct PTE *pte, const unsigned char proc, const uint32_t VPN) {
    struct page_table *pt = &page_tables[proc];
    pt->pages[VPN >> PAGE_OFFSET] = pte_encode(pte);
}

void alloc_page(struct PTE *pte, const unsigned char asid, const uint32_t vpn) {
    // Find physical frame
    void *addr = kalloc();

    // Fill in page
    pte->PFN = (uint32_t)addr;
    pte->valid = true;
    pte->dirty = false;

    // Write to page table
    pte_write(pte, asid, vpn);

    // Increment process page count
    struct proc *p = getproc();
    p->pages++;

    // printk(KERNEL_MSG "allocated VPN 0x%x of process %d @ PFN 0x%x\n\r", vpn, asid, pte->PFN);
}

// Copies a given number of words between two word-aligned addresses
static uint32_t * memcpy_aligned(const uint32_t *src, uint32_t *dst, size_t n) {
    while (n--) {
        *dst++ = *src++;
    }
    return dst;
}

// Copies the frame at src (PFN) to dst (PFN)
static void copy_frame(uint32_t src, uint32_t dst) {
    // Read from aliased user memory
    src += UMEM_ALIASDIFF;
    dst += UMEM_ALIASDIFF;

    memcpy_aligned((uint32_t *)src, (uint32_t *)dst, PAGE_SIZE/sizeof(uint32_t));
}

void vm_copy(struct proc *old, struct proc *new) {
    const unsigned char asid_old = old->PID;
    const unsigned char asid_new = new->PID;

    const struct page_table *pt_old = &page_tables[asid_old];
    const struct page_table *pt_new = &page_tables[asid_new];

    unsigned int to_copy = old->pages;  // Number of pages used by old process that still need to be copied

    // Copy text segment
    const uint32_t *p = &pt_old->pages[UTEXT0 >> PAGE_OFFSET]; // start at first page of text segment
    struct PTE pte_old;
    pte_decode(&pte_old, *p);

    int i = 0;
    while (pte_old.valid && to_copy) {
        // Allocate new page
        struct PTE pte_new;
        alloc_page(&pte_new, asid_new, UTEXT0+(PAGE_SIZE*i));

        // Copy page contents
        copy_frame(pte_old.PFN, pte_new.PFN);

        // Iterate
        to_copy--;
        i++;
        pte_decode(&pte_old, *++p);
    }

    // Copy data segment
    p = &pt_old->pages[UDATA0 >> PAGE_OFFSET];
    pte_decode(&pte_old, *p);

    i = 0;
    while (pte_old.valid && to_copy) {
        // Allocate new page
        struct PTE pte_new;
        alloc_page(&pte_new, asid_new, UDATA0+(PAGE_SIZE*i));

        // Copy page contents
        copy_frame(pte_old.PFN, pte_new.PFN);

        // Iterate
        to_copy--;
        i++;
        pte_decode(&pte_old, *++p);
    }

    // Copy stack
    p = &pt_old->pages[STACK_PG0 >> PAGE_OFFSET];
    pte_decode(&pte_old, *p);

    i = 0;
    while (pte_old.valid && to_copy) {
        // Allocate new page
        struct PTE pte_new;
        alloc_page(&pte_new, asid_new, STACK_PG0-(PAGE_SIZE*i));

        // Copy page contents
        copy_frame(pte_old.PFN, pte_new.PFN);

        // Iterate
        to_copy--;
        i++;
        pte_decode(&pte_old, *--p); // stack grows down!
    }

    if (to_copy != 0) {
        panic("vm_copy");
    }

}

void free_pagetable(unsigned char proc) {
    struct page_table *pt = &page_tables[proc];

    // Loop through entry in the page table
    for (uint32_t va = UMEM_START; va < UMEM_LIMIT; va += PAGE_SIZE ) {
        uint32_t *pte = &pt->pages[va >> PAGE_OFFSET];

        if (*pte == 0) continue;

        if ((*pte & PTE_V) == 0) continue;

        // Page is valid, free it
        struct PTE entry;
        pte_decode(&entry, *pte);
        kfree((void *)entry.PFN);

        // Clear entry
        *pte = 0;
    }


}

void tlb_read_entry(const uint32_t index, struct TLBEntry *tlb_entry) {
    set_index(index);
    tlb_read();
    tlb_entry->VPN2 = read_vpn();
    tlb_entry->ASID = get_asid();
    pte_decode(&tlb_entry->page0, read_entrylo0());
    pte_decode(&tlb_entry->page1, read_entrylo1());
}

void tlb_write_entry(const uint32_t index, const struct TLBEntry *tlb_entry) {
    set_index(index);
    set_vpn(tlb_entry->VPN2);
    set_entrylo0(pte_encode(&tlb_entry->page0));
    set_entrylo1(pte_encode(&tlb_entry->page1));
    tlb_write();
}

void tlb_mod() {
    // We're supposed to check if the address is writable but who cares
    // For now, only update the dirty bit in the page table and TLB

    const uint32_t vaddr = get_vaddr();
    const uint32_t vpn = vaddr & (~0xFFF);
    const unsigned char asid = get_asid();

    // Get page table entry
    const uint32_t x = pte_lookup(asid, vpn);
    struct PTE pte;
    pte_decode(&pte, x);

    // Make dirty
    pte.dirty = true;

    // Write to page table
    pte_write(&pte, asid, vpn);

    // Now we update the TLB

    // Get index
    const uint32_t index = find_index(vpn);

    // Get existing entry
    struct TLBEntry entry;
    tlb_read_entry(index, &entry);

    // Update and write
    if ((vpn >> 12) % 2 == 0)
        entry.page0 = pte;
    else
        entry.page1 = pte;
    tlb_write_entry(index, &entry);

}

void tlb_load_miss() {
    // Write page table entry to TLB
    const uint32_t vaddr = get_vaddr();
    const uint32_t vpn = vaddr & (~0xFFF);
    const unsigned char asid = get_asid();

    struct PTE pte;
    pte_decode(&pte, pte_lookup(asid, vpn));
    if (!pte.valid) {
        printk(ANSI_COLOR_RED "vaddr: 0x%x\n\r", vaddr);
        panic("tried to read from unmapped memory");
    }

    // Write new entry
    const uint32_t index = find_index(vpn);

    struct TLBEntry entry;
    entry.VPN2 = vpn;
    entry.ASID = asid;
    if ((vpn >> 12) % 2 == 0) {
        entry.page0 = pte;
        pte_decode(&entry.page1, pte_lookup(asid, vpn+0x1000));
    }
    else {
        entry.page1 = pte;
        pte_decode(&entry.page1, pte_lookup(asid, vpn-0x1000));
    }
    tlb_write_entry(index, &entry);
}

void tlb_store_miss() {
    const uint32_t vaddr = get_vaddr();
    const uint32_t vpn = vaddr & (~0xFFF);
    const unsigned char asid = get_asid();

    uint32_t x = pte_lookup(asid, vpn);
    struct PTE pte;
    pte_decode(&pte, x);
    if (!pte.valid) {
        alloc_page(&pte, asid, vpn);
    }

    // Write new entry to TLB
    const uint32_t index = find_index(vpn);
    struct TLBEntry entry;
    entry.VPN2 = vpn;
    entry.ASID = asid;
    if ((vpn >> 12) % 2 == 0) {
        // printk(KERNEL_MSG "Even page\n\r");
        entry.page0 = pte;
        pte_decode(&entry.page1, pte_lookup(asid, vpn+0x1000));
    }
    else {
        // printk(KERNEL_MSG "Odd page\n\r");
        entry.page1 = pte;
        pte_decode(&entry.page0, pte_lookup(asid, vpn-0x1000));
    }

    tlb_write_entry(index, &entry);

}