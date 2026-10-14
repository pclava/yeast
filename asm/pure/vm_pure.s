# ===
#   Low-level functions for virtual memory, such as interacting with the TLB
#   Implementation of certain functions in vm.h
#
#   - pclava 13 oct 2026
# ===

    .ktext

# void tlb_read()
.globl tlb_read
tlb_read:
    tlbr                # Read from TLB
    jr      $ra

# void tlb_write()
.globl tlb_write
tlb_write:
    tlbwi               # Write to TLB
    jr      $ra
