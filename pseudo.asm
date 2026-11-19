# Macros inserted at the start of every file by the preprocessor

.define _TEXT 0x00400000
.define _DATA 0x10010000
.define _STACK 0x7fffffff
.define _HEAP 0x10080000
.define _EXCVEC 0x80000180
.define _KTEXT 0x80000000
.define _KDATA 0x90000000
.define _DISPLAYADDR 0xffff0000
.define _MMIO 0xfffffa00

.define _SYSSBRK        1
.define _SYSEXIT        2
.define _SYSTERMCTL     3
.define _SYSFORK        4
.define _SYSWAIT        5
.define _SYSSLEEP       6
.define _SYSREAD        20
.define _SYSWRITE       21
.define _SYSEXEC        22
.define _SYSLINK        23
.define _SYSOPEN        24
.define _SYSMKDIR       25
.define _SYSDUP         26
.define _SYSCHDIR       27
.define _SYSCLOSE       28
.define _SYSUNLINK      29
.define _SYSGETCWD      30

# For debugging, prints the value of a register
.macro peek %r %tmp
    addiu   %tmp, $v0, 0
    addiu   $v0, %r, 0
    break
    addiu   $v0, %tmp, 0
.end_macro

# Flip the sign of a register
.macro negu %rd, %rs
    subu    %rd, $0, %rs
.end_macro

# Negate register
.macro not %rd, %rs
    nor %rd, %rs, $0
.end_macro

# Push to stack
.macro push %r
    addiu $sp, $sp, -4
    sw %r, 0($sp)
.end_macro

# Pop from stack
.macro pop %r
    lw %r, 0($sp)
    addiu $sp, $sp, 4
.end_macro

# Branch less than
.macro blt %r1 %r2 %lbl
    slt $at, %r1, %r2
    bne $at, $0, %lbl
.end_macro

# Branch greater or equal
.macro bge %r1 %r2 %lbl
    slt $at, %r1, %r2
    beq $at, $0, %lbl
.end_macro

# Branch greater or equal immediate
.macro bgei %r %imm %lbl
    slti $at, %r, %imm
    beq  $at, $0, %lbl
.end_macro

# Branch greater
.macro bgt %r1 %r2 %lbl
    slt $at, %r2, %r1
    bne $at, $0, %lbl
.end_macro

# Branch less or equal
.macro ble %r1 %r2 %lbl
    slt $at, %r2, %r1
    beq $at, $0, %lbl
.end_macro

# Move
.macro move %r1 %r2
    addu %r1, $0, %r2
.end_macro

# Short-distance unconditional branch
.macro b %lbl
    beq $0, $0, %lbl
.end_macro

# Branch on zero
.macro beqz %r %lbl
    beq %r, $0, %lbl
.end_macro

# Branch on not zero
.macro bnez %r %lbl
    bne %r, $0, %lbl
.end_macro

# Load address
.macro la %r %lbl
    lui $at, %hi(%lbl)
    ori %r, $at, %lo(%lbl)
.end_macro

# Load immediate
.macro li %r %imm
    lui $at, %hi(%imm)
    ori %r, $at, %lo(%imm)
.end_macro

# Terminate program with exit code 0
.macro done
    li $v0, _SYSEXIT
    syscall
.end_macro

# Terminate program with an exit code
.macro progexit %imm
    li $v0, _SYSEXIT2
    li $a0, %imm
    syscall
.end_macro

# Absolute value
.macro abs %dst %src
    sra $at, %src, 31    # -1 if negative, 0 if positive
    xor %dst, %src, $at  # if negative, flip all the bits
    sub %dst, %dst, $at  # subtract -1 or 0
.end_macro