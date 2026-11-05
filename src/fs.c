#include "fs.h"

#include <stddef.h>

#include "log.h"
#include "buffer.h"
#include "printk.h"

struct superblock sb;

static void * memset_aligned(uint32_t *dst, uint32_t c, size_t n) {
    uint32_t *dest = dst;
    while (n--) {
        *dest++ = c;
    }
    return dst;
}

static void * memcpy(void *vdst, const void *vsrc, int n)
{
    char *dst;
    const char *src;

    dst = vdst;
    src = vsrc;
    if (src > dst) {
        while (n-- > 0)
            *dst++ = *src++;
    } else {
        dst += n;
        src += n;
        while (n-- > 0)
            *--dst = *--src;
    }
    return vdst;
}

static void read_superblock(struct superblock *super) {
    struct bbuf *block = bread(1);
    memcpy(super, block, sizeof(struct superblock));
    brelse(block);
}

static void block_zero(uint32_t bno) {
    struct bbuf *bp = bread(bno);
    memset_aligned((uint32_t *)bp->data, 0, BLOCKSIZE / sizeof(uint32_t));
    log_write(bp);
    brelse(bp);
}

static uint32_t balloc(void) {
    uint32_t ceil_bmapsz = (sb.size + BITS_PER_BLOCK - 1) / BITS_PER_BLOCK;
    for (int bno = 0; bno < ceil_bmapsz; bno++) {
        // Get bitmap block
        struct bbuf *bp = bread(sb.bitmapstart+bno);

        // Read each byte in the block
        for (int bit = 0; bit < BITS_PER_BLOCK && bit + bno < sb.size; bit++) {
            int byte = bit / 8;
            char mask = (char) (1 << (bit % 8));

            if ((bp->data[byte] & mask) == 0) {
                // block is free, mark as used
                bp->data[byte] = (char) (bp->data[byte] | mask);
                log_write(bp); // write bitmap
                brelse(bp); // release block
                block_zero(bit+bno); // clear data
            }
        }

        brelse(bp);
    }

    panic("balloc: no blocks!");
}

static void bfree(uint32_t bno) {

    // Get bitmap block
    struct bbuf *bp = bread(bno / BITS_PER_BLOCK + sb.bitmapstart);

    // Get location in block
    uint32_t bit = bno % BITS_PER_BLOCK;
    uint32_t byte = bit / 8;
    char mask = (char) (1 << (bit % 8));

    if ((bp->data[byte] & mask) == 0) panic("bfree: block already free");

    // Set to 0
    bp->data[byte] = (char) (bp->data[byte] & (~mask));

    // Write and release
    log_write(bp);
    brelse(bp);
}

// called when first process is initialized
void fs_init(void) {
    bcache_init();

    // Read superblock
    read_superblock(&sb);

    // Init log
    log_init(&sb);
}