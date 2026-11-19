# Unfortunate caveat: a lot of work is done to shape the compiler's output
# into something readable by the assembler
# and this is thus not guaranteed to work on any other compiler :(

ifndef CC_OVERRIDE_CHECK
	CC = /opt/homebrew/opt/llvm/bin/clang
endif
TARGET = mipsel-linux-gnu

# Idk what half these flags do but getting them right was a delicate process of fine tuning
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

KER_SRCS = $(wildcard $(KER_SRC_DIR)/*.c)
USR_SRCS = $(wildcard $(USR_SRC_DIR)/*.c)

KER_ASM_FILES = $(patsubst $(KER_SRC_DIR)/%.c, $(KER_ASM_DIR)/%.s, $(KER_SRCS))
USR_ASM_FILES = $(patsubst $(USR_SRC_DIR)/%.c, $(USR_ASM_DIR)/%.s, $(USR_SRCS))

all: $(KER_ASM_FILES) $(USR_ASM_FILES) mkfs
	./build.sh

mkfs: mkfs.o
	$(CC) -Wall -Wextra -I. mkfs.o -o mkfs

mkfs.o: mkfs.c
	$(CC) -Wall -Wextra -I. -c mkfs.c -o mkfs.o

# Note: the sed command is a little outrageous, I know.
# The compiler I'm using was not meant for my assembler
# and the compiler's output needs to be shaped to fit it
# Note: converts all .text and .data to .ktext and .kdata
$(KER_ASM_DIR)/%.s: $(KER_SRC_DIR)/%.c
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

	rm $@.tmp

$(USR_ASM_DIR)/%.s: $(USR_SRC_DIR)/%.c
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

	rm $@.tmp

clean:
	rm -f $(KER_ASM_DIR)/*.s
	rm -f $(USR_ASM_DIR)/*.s
	rm -f mkfs.o
	rm -f kernel.out
