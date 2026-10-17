/*
 * Virtual memory, TLB, and page table manager
 *
 * There's a lot going on here. When the TLB misses, it raises an exception
 * This is caught by the handler, and it ends up in one of the three appropriate functions
 *  (mod, load_miss, store_miss)
 * These pull the page table entry and load it into the TLB
 * If there is no valid page table entry (ie, the page has not been allocated yet),
 * kalloc() is called to find a free frame and assign it to this page number and process ID.
 *
 * The rest of the functions are to support these exception handlers.
 * Note that page numbers are kept UNSHIFTED. That is, to get the actual number, the code must shift it right by PAGE_OFFSET first
 *
 * A memory address is formatted as:
 *
 *  31  30                   12  11                    0
 * [  ][ Page number (19 bits) ][ Page offset (12 bits) ]
 * - Bits 0-11  : page offset (not changed during translation)
 * - Bits 12-30 : page number
 * - Bit 31     : not used (all user addresses have this bit low)
 *
 * Implemented in vm.c and vm_pure.s
 * - pclava 13 oct 2026
 */


#pragma once
#include <stdbool.h>
#include <stdint.h>

#include "proc.h"

/* === PAGE TABLE === */

#define PAGE_OFFSET 12  // Size in bits of page offset (4096 bytes = 12 bits)
#define TLB_SIZE 128    // Number of entries in TLB
#define PAGE_SIZE 4096  // Size in bytes of page

// Structure of page table entry. This gets encoded/decoded into/from a 32 bit value with pte_encode/decode
struct PTE {
    uint32_t PFN;
    bool dirty;
    bool valid;
};

#define PTE_V 0b10
#define PTE_D 0b100
#define FLAGS_WIDTH 6
#define PAGE_COUNT 0x80000

// One process's page table
struct page_table {
    uint32_t pages[PAGE_COUNT]; // There are 2^19 (0x80000) virtual pages
};

// List of all page tables
extern struct page_table *page_tables;

// The mapping for page N of process M can be found at page_tables[M][N]

// Structure of a TLB entry
// Note that the TLB contains the mapping of two contiguous virtual pages
// so the VPN here is always an even number
struct TLBEntry {
    uint32_t VPN2;
    uint32_t ASID;      // Associated process
    struct PTE page0;   // Mapping of lower (even) page
    struct PTE page1;   // Mapping of upper (odd) page
};

// Initializes the page table for the process (sets everything to zero)
void pt_init(unsigned char pid);

// Converts PTE encoding into readable struct
void pte_decode(struct PTE *dst, uint32_t pte);

// Converts PTE struct into encoding
uint32_t pte_encode(const struct PTE *pte);

// Get PTE from page table, given process ID and VPN
uint32_t pte_lookup(unsigned char proc, uint32_t VPN);

// Writes a PTE to the page table at the given VPN
void pte_write(const struct PTE *pte, unsigned char proc, uint32_t VPN);

// Allocates a page and writes it to the TLB
// TODO: check for kalloc failure
void alloc_page(struct PTE *pte, unsigned char asid, uint32_t vpn);

// Copies the contents of virtual memory from one process to the other
// TODO: check for failure
// TODO: implement copy-on-write
void vm_copy(struct proc *old, struct proc *new);

// Removes all mappings in the process's page table
// and frees physical memory
void free_pagetable(unsigned char proc);

/* === TLB === */

// Occurs when a store occurs on a valid but not dirty page
void tlb_mod();

// Occurs when a load occurs at a page not present in TLB
void tlb_load_miss();

// Occurs when a store occurs at a page not present in TLB
void tlb_store_miss();

// Reads the TLB entry at the index into the pointer
void tlb_read_entry(uint32_t index, struct TLBEntry *tlb_entry);

// Writes the TLB entry at the pointer into the index
void tlb_write_entry(uint32_t index, const struct TLBEntry *tlb_entry);

/* === PURE === */

// Reads from TLB entry at the index in the $index register, into EntryHi and EntryLo
// Must have set the index with set_index()
void tlb_read();

// Writes to TLB entry at the index in the $index register, from EntryHi and EntryLo
// Must have set the index with set_index()
void tlb_write();