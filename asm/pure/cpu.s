# ===
#   Low-level CPU functions
#   namely for fetching coprocessor 0 registers
#   This is the implementation of cpu.h
#
#   - pclava 13 oct 2026
# ===


.ktext

.define TRAPFRAME       0x90000004
.define UMEM_ALIASDIFF  0x8f000000

# void swtch(struct context *old, struct context *new);
.globl swtch
swtch:
    # Save old context callee-saved registers
    sw      $ra, 0($a0)
    sw      $sp, 4($a0)
    sw      $fp, 8($a0)
    sw      $gp, 12($a0)
    sw      $s0, 16($a0)
    sw      $s1, 20($a0)
    sw      $s2, 24($a0)
    sw      $s3, 28($a0)
    sw      $s4, 32($a0)
    sw      $s5, 36($a0)
    sw      $s6, 40($a0)
    sw      $s7, 44($a0)

    # Load new context callee-saved registers
    lw      $ra, 0($a1)
    lw      $sp, 4($a1)
    lw      $fp, 8($a1)
    lw      $gp, 12($a1)
    lw      $s0, 16($a1)
    lw      $s1, 20($a1)
    lw      $s2, 24($a1)
    lw      $s3, 28($a1)
    lw      $s4, 32($a1)
    lw      $s5, 36($a1)
    lw      $s6, 40($a1)
    lw      $s7, 44($a1)

    # Return to where new context was supposed to return after swtch()
    jr      $ra

# void register_trapframe(struct trapframe *trapframe)
.globl register_trapframe
register_trapframe:
    la      $v0, TRAPFRAME
    sw      $a0, 0($v0)
    jr      $ra

# struct trapframe *get_trapframe()
.globl get_trapframe
get_trapframe:
    la      $v0, TRAPFRAME
    lw      $v0, 0($v0)
    jr      $ra

# void int_on()
.globl int_on
int_on:
    ei      $0
    jr      $ra

# void int_off()
.globl int_off
int_off:
    di      $0
    jr      $ra

# void set_user_mode()
.globl set_user_mode
set_user_mode:
    mfc0    $v0, $12                # Read status
    andi    $v0, $v0, 0xFFFFFFEF    # Set bit 4 to 0
    mtc0    $v0, $12                # Save status
    jr      $ra

# void set_kernel_mode()
.globl set_kernel_mode
set_kernel_mode:
    mfc0    $v0, $12
    ori     $v0, $v0, 0x10
    mtc0    $v0, $12
    jr      $ra

# int get_mode()
.globl get_mode
get_mode:
    mfc0    $v0, $12                # Read status
    andi    $v0, $v0, 0x00000010    # Isolate bit 4
    srl     $v0, $v0, 4             # Move to bit 0
    jr      $ra

# int get_interrupt()
.globl get_interrupt
get_interrupt:
    mfc0    $v0, $13            # Cause register
    srl     $v0, $v0, 8         # Move IP bits down
    andi    $v0, $v0, 0xFF      # Mask bits
    jr      $ra

# void set_epc(uint32_t addr)
.globl set_epc
set_epc:
    mtc0    $a0, $14        # Move a0 into EPC
    jr      $ra

# uint32_t get_epc()
.globl get_epc
get_epc:
    mfc0    $v0, $14
    jr      $ra

# unsigned char get_cause()
.globl get_cause
get_cause:
    mfc0    $v0, $13            # Cause register
    andi    $v0, $v0, 0b1111100 # Extract ExcCode field
    srl     $v0, $v0, 2
    jr      $ra

# unsigned char get_asid()
.globl get_asid
get_asid:
    mfc0    $v0, $10
    andi    $v0, $v0, 0xFF
    jr      $ra

# void set_asid(unsigned char asid)
.globl set_asid
set_asid:
    andi    $a0, $a0, 0xFF
    mfc0    $t0, $10
    andi    $t0, $t0, 0xFFFFFF00
    or      $a0, $t0, $a0
    mtc0    $a0, $10
    jr      $ra

# void set_index(uint32_t index)
.globl set_index
set_index:
    mtc0    $a0, $0
    jr      $ra

# uint32_t read_vpn()
.globl read_vpn
read_vpn:
    mfc0    $v0, $10
    li      $t0, 0xFFFFFF00
    and     $v0, $v0, $t0
    srl     $v0, $v0, 1
    jr      $ra

# void set_vpn(uint32_t)
.globl set_vpn
set_vpn:
    mfc0    $t0, $10        # Get EntryHi
    li      $t1, 0xFFFFFF00
    and     $a0, $a0, $t1   # Clear out ASID bits of argument
    andi    $t0, $t0, 0xFF  # Clear VPN from EntryHi
    sll     $a0, $a0, 1     # Move VPN up one bit
    or      $a0, $a0, $t0   # Combine EntryHi and argument
    mtc0    $a0, $10        # Save in EntryHi
    jr      $ra

# uint32_t read_entrylo0()
.globl read_entrylo0
read_entrylo0:
    mfc0    $v0, $2
    jr      $ra

# uint32_t read_entrylo1();
.globl read_entrylo1
read_entrylo1:
    mfc0    $v0, $3
    jr      $ra

# void set_entrylo0(uint32_t pte)
.globl set_entrylo0
set_entrylo0:
    mtc0    $a0, $2
    jr      $ra

# void set_entrylo1(uint32_t pte)
.globl set_entrylo1
set_entrylo1:
    mtc0    $a0, $3
    jr      $ra

# uint32_t get_vaddr()
.globl get_vaddr
get_vaddr:
    mfc0    $v0, $8
    jr      $ra
