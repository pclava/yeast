#include "buffer.h"

#include <stddef.h>

#include "memlayout.h"
#include "printk.h"

enum DiskOp {
    READ = 1,
    WRITE,
};

struct {
    struct bbuf bufs[NBUFS];
    struct bbuf *head;
} bcache;

static void * memset (void *dest, register int val, register size_t len)
{
    register unsigned char *ptr = (unsigned char*)dest;
    while (len-- > 0)
        *ptr++ = val;
    return dest;
}

// NOTE: the emulator currently does disk operations atomically
// to be more realistic, we would want disk operations to be slow
// and we would have the thread wait until the disk issues an interrupt
// indicating that it's finished
// I will hopefully implement this someday
void disk_read(struct bbuf *b) {
    acquire();

    unsigned int addr = (unsigned int) b->data;
    unsigned int sector = b->bno;
    unsigned int op = READ;

    // Set arguments
    *(unsigned int *)DSK_SECT = sector;
    *(unsigned int *)DSK_ADDR = addr;

    // Perform operation (happens immediately on write)
    *(unsigned int *)DSK_CTRL = op;
    asm volatile ("break");

    release();
}

void disk_write(struct bbuf *b) {
    acquire();

    unsigned int addr = (unsigned int) &b->data;
    unsigned int sector = b->bno;
    unsigned int op = WRITE;

    // Set arguments
    *(unsigned int *)DSK_SECT = sector;
    *(unsigned int *)DSK_ADDR = addr;

    // Perform operation (happens immediately on write)
    *(unsigned int *)DSK_CTRL = op;

    release();
}

void disk_test(void) {
    // get a buffer
    struct bbuf *b = bread(31);
    b->data[BLOCKSIZE-1] = '\0';

    // write to it, read from it
    // b->data[0] = '!';
    printk("%s\n\r", b->data);

    // write buffer
    bwrite(b);

    struct bbuf *b2 = bread(32);
    b2->data[BLOCKSIZE-1] = '\0';
    printk("%s\n\r", b2->data);

    // release
    brelse(b);
    brelse(b2);

    // get buffer back
    bread(31);
    brelse(b);
    bread(32);
    brelse(b2);
}

void bcache_init(void) {
    bcache.head = bcache.bufs;
    bcache.head->prev = bcache.head;
    bcache.head->next = bcache.head;

    // Initialize linked list
    for (int i = 1; i < NBUFS; i++) {
        struct bbuf *b = &bcache.bufs[i];
        b->valid = 0;
        b->references = 0;
        b->sleeplock.locked = 0;
        b->sleeplock.pid = 0;

        struct bbuf *tl = bcache.head->prev;
        tl->next = b;
        bcache.head->prev = b;
        b->next = bcache.head;
        b->prev = tl;
    }
}

// otherwise known as bget (block get)
// Gets a block from the cache
// If the block is not cached, finds a slot and assigns it
// The caller still has to load the contents from disk
static struct bbuf *baguette(uint32_t bno) {
    acquire();

    struct bbuf *b;

    // Check cache for existing buffer
    b = bcache.head;
    do {
        // The linked list is sorted by most recently used
        // So by starting from the head forwards we are more likely
        // to get the block quickly
        if (b->bno == bno) {
            // we don't check if valid, there's no need
            // if it's here near the head of the list
            // it may have been recently brelse()'d
            b->references++;
            release();
            acquire_sleep(&b->sleeplock);
            return b;
        }
        b = b->next;
    } while (b != bcache.head);

    // Check cache for first unused buffer
    b = bcache.head->prev;
    do {
        // We recycle the least recently used buffer by going backwards
        if (b->references == 0) {
            // Assign buffer. bread() will fetch the data from disk
            b->valid = 0;
            b->references = 1;
            b->bno = bno;
            release();
            acquire_sleep(&b->sleeplock);
            return b;
        }
        b = b->prev;
    } while (b != bcache.head->prev);

    panic("no free buffers");
}

// omg bread, like yeast? omg
// reads and acquires a block from the disk
struct bbuf *bread(uint32_t bno) {
    struct bbuf *b = baguette(bno);
    if (!b->valid) {
        // Read from disk
        disk_read(b);
        b->valid = 1;
    }
    return b;
}

// writes a block to the disk, but does not release it
void bwrite(struct bbuf *b) {
    if (!holding_sleep(&b->sleeplock)) panic("bwrite: block not held");
    disk_write(b);
}

void brelse(struct bbuf *b) {
    if (!holding_sleep(&b->sleeplock)) panic("brelse: block not held");
    release_sleep(&b->sleeplock);

    acquire();
    b->references--; // no longer using block
    if (b->references == 0) { // if no one is waiting on this block
        // move to head of linked list

        // remove from current position
        b->prev->next = b->next; // y.next = v
        b->next->prev = b->prev; // v.prev = y

        // place after tail
        bcache.head->prev->next = b; // w.next = z
        b->prev = bcache.head->prev; // z.prev = w

        // place before head
        bcache.head->prev = b;
        b->next = bcache.head;

        // set head
        bcache.head = b;
    }
    release();
}