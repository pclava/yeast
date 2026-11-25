# ===
# Added to user programs to load user arguments and call main
# ===

.globl __start
__start:
    move    $fp, $sp

    # main(argc, argv)
    lw      $a0, 0($sp)
    addiu   $a1, $sp, 4
    jal     main

    # Terminate gracefully
    move    $a0, $v0
    jal     exit

__bottom:
    j       __bottom