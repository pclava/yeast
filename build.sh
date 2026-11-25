#!/bin/bash

# Build the kernel, user programs, and disk
# Expects mkfs, MIPSAssembler, and MIPSLinker to be in the same directory

src="asm"
pure="$src/pure"
ker="$src/kernel"
usr="$src/user"
userroot="userroot"

# === BUILD KERNEL ===

ks_files=(
  "$pure/boot.s"
  "$ker/main.s"
  "$ker/tty.s"
  "$ker/proc.s"
  "$ker/trap.s"
  "$pure/trap_pure.s"
  "$pure/cpu.s"
  "$ker/syscall.s"
  "$ker/printk.s"
  "$pure/keyboard.s"
  "$ker/vm.s"
  "$pure/vm_pure.s"
  "$ker/kalloc.s"
  "$ker/lock.s"
  "$ker/buffer.s"
  "$ker/log.s"
  "$ker/fs.s"
  "$ker/inode.s"
  "$ker/file.s"
  "$ker/mof.s"
  "$ker/string.s"
)

ko_files=()

for file in "${ks_files[@]}"; do
  ko_files+=("${file%.s}.o")
done

./MIPSAssembler -p pseudo.asm "${ks_files[@]}"
./MIPSLinker -c -s _entry -o kernel.out "${ko_files[@]}"


# === BUILD USER PROGRAMS ===

# Assemble lib files
ulib_s=(
  "$pure/usys.s"
  "$usr/printf.s"
  "$usr/ulib.s"
  "$usr/umalloc.s"
  "$userroot/__start.s"
)
ulib_o=()
for file in "${ulib_s[@]}"; do
  ulib_o+=("${file%.s}.o")
done
./MIPSAssembler -p pseudo.asm "${ulib_s[@]}"

# Assemble program files
usrc_s=(
  "$usr/init.s"
  "$usr/sh.s"
  "$usr/ls.s"
)
usrc_o=()
for file in "${usrc_s[@]}"; do
  usrc_o+=("${file%.s}.o")
done
./MIPSAssembler -p pseudo.asm "${usrc_s[@]}"

# Build programs
for file in "${usrc_o[@]}"; do
  filename=$(basename "${file}" .o)
  ./MIPSLinker -s __start -o "$userroot/_$filename" "${file}" "${ulib_o[@]}"
done

# Clean objects (normally would do the -c flag during linking but we link multiple times)
rm -f "$ker"/*.o
rm -f "$pure"/*.o
rm -f "$usr"/*.o
rm -f "$userroot"/*.o

# === FILE SYSTEM ===
./mkfs disk.img "$userroot"