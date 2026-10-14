# ===
#   This is the first code run upon boot
#   The exception handler is also kept here
#   It is extremely important that the first word in .kdata be the exception vector
#   This file MUST be linked first
#
#   - pclava 13 oct 2026
# ===

.kdata
.word   _exc        # Exception vector
.word   0           # Active process trapframe

.define EXCVEC 0x90000000
.define TRAPFRAME 0x90000004

.ktext

# CPU begins execution here on boot. This must be the first part of .ktext
.globl _entry
_entry:
    # Do some stuff

    # Initialize kernel stack pointer
    li  $sp, 0x9ffffffc

    # Jump to start routine
    jal start

spin:
    j   spin                        # If start() ever returns, idle indefinitely


# === GLOBAL EXCEPTION HANDLER ===

# All interrupts, syscalls, and exceptions land here
# Upon any trap, the emulator disables interrupts for us. They are already disabled here, and should be reenabled.
.globl _exc
_exc:
    addu    $k1, $at, $0

    # Get CPU mode in $k0
    mfc0    $k0, $12                # Read status
    andi    $k0, $k0, 0x10          # Isolate bit 4
    srl     $k0, $k0, 4             # Move to bit 0

    # Check where trap came from
    beq     $k0, $0, _userexc
    j       _kerexc

# If exception happened in user space
_userexc:
    # Get pointer to trapframe
    la      $k0, TRAPFRAME
    lw      $k0, 0($k0)

    # Save trapframe
    addu    $at, $k1, $0
    sw      $at, 0($k0)
    sw      $v0, 4($k0)
    sw      $v1, 8($k0)
    sw      $a0, 12($k0)
    sw      $a1, 16($k0)
    sw      $a2, 20($k0)
    sw      $a3, 24($k0)
    sw      $t0, 28($k0)
    sw      $t1, 32($k0)
    sw      $t2, 36($k0)
    sw      $t3, 40($k0)
    sw      $t4, 44($k0)
    sw      $t5, 48($k0)
    sw      $t6, 52($k0)
    sw      $t7, 56($k0)
    sw      $s0, 60($k0)
    sw      $s1, 64($k0)
    sw      $s2, 68($k0)
    sw      $s3, 72($k0)
    sw      $s4, 76($k0)
    sw      $s5, 80($k0)
    sw      $s6, 84($k0)
    sw      $s7, 88($k0)
    sw      $t8, 92($k0)
    sw      $t9, 96($k0)
    sw      $gp, 100($k0)
    sw      $sp, 104($k0)
    sw      $fp, 108($k0)
    sw      $ra, 112($k0)

    # At this point, all registers are fair game

    # Save PC in trapframe, because we will be overwriting EPC
    jal     get_epc
    sw      $v0, 116($k0)

    # Set EPC to usertrap
    la      $a0, usertrap
    jal     set_epc

    # Switch to kernel mode
    jal     set_kernel_mode

    # Load kernel stack pointer
    lw      $sp, 120($k0)

    # Jump to usertrap, lowering EXL at the same time
    eret

_kerexc:
    # Push registers to stack
    addiu   $sp, $sp, -124
    sw      $at, 0($sp);
    sw      $v0, 4($sp);
    sw      $v1, 8($sp);
    sw      $a0, 12($sp);
    sw      $a1, 16($sp);
    sw      $a2, 20($sp);
    sw      $a3, 24($sp);
    sw      $t0, 28($sp);
    sw      $t1, 32($sp);
    sw      $t2, 36($sp);
    sw      $t3, 40($sp);
    sw      $t4, 44($sp);
    sw      $t5, 48($sp);
    sw      $t6, 52($sp);
    sw      $t7, 56($sp);
    sw      $s0, 60($sp);
    sw      $s1, 64($sp);
    sw      $s2, 68($sp);
    sw      $s3, 72($sp);
    sw      $s4, 76($sp);
    sw      $s5, 80($sp);
    sw      $s6, 84($sp);
    sw      $s7, 88($sp);
    sw      $t8, 92($sp);
    sw      $t9, 96($sp);
    sw      $gp, 100($sp);
    #sw      $sp, 104($sp);
    sw      $fp, 108($sp);
    sw      $ra, 112($sp);
    sw      $k0, 116($sp)
    sw      $k1, 120($sp)

    # Call kerneltrap
    jal     kerneltrap

    # Load registers
    lw      $at, 0($sp);
    lw      $v0, 4($sp);
    lw      $v1, 8($sp);
    lw      $a0, 12($sp);
    lw      $a1, 16($sp);
    lw      $a2, 20($sp);
    lw      $a3, 24($sp);
    lw      $t0, 28($sp);
    lw      $t1, 32($sp);
    lw      $t2, 36($sp);
    lw      $t3, 40($sp);
    lw      $t4, 44($sp);
    lw      $t5, 48($sp);
    lw      $t6, 52($sp);
    lw      $t7, 56($sp);
    lw      $s0, 60($sp);
    lw      $s1, 64($sp);
    lw      $s2, 68($sp);
    lw      $s3, 72($sp);
    lw      $s4, 76($sp);
    lw      $s5, 80($sp);
    lw      $s6, 84($sp);
    lw      $s7, 88($sp);
    lw      $t8, 92($sp);
    lw      $t9, 96($sp);
    lw      $gp, 100($sp);
    #lw      $sp, 104($sp);
    lw      $fp, 108($sp);
    lw      $ra, 112($sp);
    lw      $k0, 116($sp)
    lw      $k1, 120($sp)
    addiu   $sp, $sp, 124

    # Enable interrupts
    # Note: kerneltrap will only return safely if
    # the trap *was* an interrupt,
    # meaning interrupts had to have been enabled
    # and it is safe to reenable

    # TODO: this is no longer true because of TLB exceptions
    ei      $0

    # Note: emulator runs interrupts *after* instruction execution
    # and *after* PC has been incremented, so no need to touch EPC

    # Return to where kernel was interrupted
    eret