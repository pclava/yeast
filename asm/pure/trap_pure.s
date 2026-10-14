# ===
#   Low-level functions for trap handling
#   Implementation of certain functions in trap.h
#
#   - pclava 13 oct 2026
# ===


    .ktext

.define TRAPFRAME 0x90000004

panc: j panc

# void prepare_return_pure()
.globl prepare_return_pure
prepare_return_pure:
    mfc0    $v0, $12                # Read status
    li      $t0, 0xFFFFFFEF
    and     $v0, $v0, $t0           # Set bit 4 to 0 (user mode)
    ori     $v0, $v0, 2             # Set bit 1 to 1 (exc_level)
    mtc0    $v0, $12
    ei      $0                      # Enable interrupts TODO: might be a problem here?
    la      $v0, TRAPFRAME          # Get pointer to trapframe
    lw      $v0, 0($v0)
    lw      $v0, 116($v0)           # Read PC
    mtc0    $v0, $14                # Set EPC
    jr      $ra


# void userret()
.globl userret
userret:
    # Get pointer to trapframe
    la      $a0, TRAPFRAME
    lw      $a0, 0($a0)
    # Load trapframe
    lw      $at, 0($a0)
    lw      $v0, 4($a0)
    lw      $v1, 8($a0)
    lw      $a1, 16($a0)
    lw      $a2, 20($a0)
    lw      $a3, 24($a0)
    lw      $t0, 28($a0)
    lw      $t1, 32($a0)
    lw      $t2, 36($a0)
    lw      $t3, 40($a0)
    lw      $t4, 44($a0)
    lw      $t5, 48($a0)
    lw      $t6, 52($a0)
    lw      $t7, 56($a0)
    lw      $s0, 60($a0)
    lw      $s1, 64($a0)
    lw      $s2, 68($a0)
    lw      $s3, 72($a0)
    lw      $s4, 76($a0)
    lw      $s5, 80($a0)
    lw      $s6, 84($a0)
    lw      $s7, 88($a0)
    lw      $t8, 92($a0)
    lw      $t9, 96($a0)
    lw      $gp, 100($a0)
    lw      $sp, 104($a0)
    lw      $fp, 108($a0)
    lw      $ra, 112($a0)
    lw      $a0, 12($a0)
    eret