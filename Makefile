# Use Homebrew's upstream Clang
CC = /opt/homebrew/opt/llvm/bin/clang
TARGET = mipsel-linux-gnu

# Stripped down flags: disables PIC, abicalls, CFI stack unwinding, debug, and address signatures
CFLAGS = -S -target $(TARGET) -march=mips32r3 -O2 -g0 \
         -fno-PIC -mno-abicalls \
         -mno-gpopt -G0 \
         -fno-jump-tables -fno-builtin -fno-builtin-memcpy -fno-common \
         -fno-asynchronous-unwind-tables -fno-addrsig \
         -mllvm -disable-mips-delay-filler -I./include

KER_SRC_DIR = src
KER_ASM_DIR = asm/kernel
USR_SRC_DIR = userroot
USR_ASM_DIR = asm/user

# Find all .c files inside the src/ directory
KER_SRCS = $(wildcard $(KER_SRC_DIR)/*.c)
USR_SRCS = $(wildcard $(USR_SRC_DIR)/*.c)

# Map src/*.c paths to asm/*.s paths
KER_ASM_FILES = $(patsubst $(KER_SRC_DIR)/%.c, $(KER_ASM_DIR)/%.s, $(KER_SRCS))
USR_ASM_FILES = $(patsubst $(USR_SRC_DIR)/%.c, $(USR_ASM_DIR)/%.s, $(USR_SRCS))

all: $(KER_ASM_FILES) $(USR_ASM_FILES) mkfs
	./build.sh

mkfs: mkfs.o
	$(CC) -Wall -Wextra -I. mkfs.o -o mkfs

mkfs.o: mkfs.c
	$(CC) -Wall -Wextra -I. -c mkfs.c -o mkfs.o

$(KER_ASM_DIR)/%.s: $(KER_SRC_DIR)/%.c
# 1. Compile C to raw MIPS assembly
# !!! NOTE !!! Converts all .text and .data to .ktext and .kdata
	$(CC) $(CFLAGS) $< -o $@.tmp

	sed -E \
		-e 's/asciz/asciiz/g' \
		-e '/func_end/d' \
		-e '/[[:space:]]+nop([[:space:]]+|$$)/d' \
		-e 's/\.bss/\.kdata/g' \
		-e 's/^[[:space:]]*\.section[[:space:]]+\.rodata.*/	.kdata/g' \
		-e 's/\.data/\.kdata/g' \
		-e 's/\.text/\.ktext/g' \
		-e 's/\.4byte/\.word/g' \
		-e '/^[[:space:]]*\.(section|nan|module|file|p2align)/d' \
		-e '/^[[:space:]]*\.(type|size|ident|frame|mask|fmask|local|insn|comm|align|weak)/d' \
		-e '/^[[:space:]]*\.set[[:space:]]+/d' \
		-e '/^[[:space:]]*\.end[[:space:]]+/d' \
		-e '/^[[:space:]]*\.ent[[:space:]]+/d' \
		$@.tmp > $@

# 3. Clean up the temporary file
	rm $@.tmp

$(USR_ASM_DIR)/%.s: $(USR_SRC_DIR)/%.c
# 1. Compile C to raw MIPS assembly
	$(CC) $(CFLAGS) -I./include $< -o $@.tmp

	sed -E \
		-e 's/asciz/asciiz/g' \
		-e '/func_end/d' \
		-e '/[[:space:]]+nop([[:space:]]+|$$)/d' \
		-e 's/^[[:space:]]*\.section[[:space:]]+\.rodata.*/	.data/g' \
		-e 's/\.kdata/\.data/g' \
		-e 's/\.ktext/\.text/g' \
		-e 's/\.4byte/\.word/g' \
		-e '/^[[:space:]]*\.(section|nan|module|file|p2align)/d' \
		-e '/^[[:space:]]*\.(type|size|ident|frame|mask|fmask|local|insn|align|weak)/d' \
		-e '/^[[:space:]]*\.set[[:space:]]+/d' \
		-e '/^[[:space:]]*\.end[[:space:]]+/d' \
		-e '/^[[:space:]]*\.ent[[:space:]]+/d' \
		$@.tmp > $@

# 3. Clean up the temporary file
	rm $@.tmp

clean:
	rm -f $(KER_ASM_DIR)/*.s
	rm -f $(USR_ASM_DIR)/*.s
	rm -f mkfs.o
