#include "file.h"

#include "log.h"
#include "printk.h"

static void * memset_aligned(uint32_t *dst, uint32_t c, size_t n) {
    uint32_t *dest = dst;
    while (n--) {
        *dest++ = c;
    }
    return dst;
}
static uint32_t *memset(void *dst, char c, size_t n) {
    size_t w = n/4;
    size_t left = n%4;
    uint32_t C = c || (c << 8) || (c << 16) || (c << 24);
    memset_aligned(dst, C, left);
    unsigned char *d = dst+(w * sizeof(uint32_t));
    while (left--) {
        *d++ = c;
    }
    return dst;
}

typedef struct file * fileptr;

struct file ftable[NFILE];

void fileinit(void) {
    memset(ftable, 0, sizeof(ftable));
}

fileptr filealloc(void) {
    acquire();
    for (fileptr fp = ftable; fp < &ftable[NFILE]; fp++) {
        if (fp->references == 0) {
            fp->references++;
            return fp;
        }
    }
    release();
    return NULL;
}

fileptr filedup(fileptr fp) {
    acquire();
    if (fp->references <= 0) panic("filedup");
    fp->references++;
    release();
    return fp;
}

void fileclose(fileptr fp) {
    acquire();

    if (fp->references <= 0) panic("fileclose");

    fp->references--;
    if (fp->references > 0) {
        release();
        return;
    }

    struct file f = *fp;
    fp->type = FD_NONE;
    fp->references = 0;

    release();

    if (f.type == FD_INODE) {
        begin_op();
        iput(f.ip);
        end_op();
    } else {
        panic("fileclose: not implemented");
    }
}

int fileread(fileptr fp, void *addr, size_t size) {
    if (!fp->readable) return -1;

    if (fp->type == FD_INODE) {
        ilock(fp->ip);
        uint32_t ret = readi(fp->ip, addr, fp->offset,size);
        if (ret > 0) fp->offset += ret;
        iunlock(fp->ip);
        return (int) ret;
    }

    panic("fileread: not implemented");
}

int filewrite(fileptr fp, void *addr, size_t size) {
    if (!fp->writable) return -1;

    if (fp->type == FD_INODE) {
        ilock(fp->ip);
        uint32_t ret = writei(fp->ip, addr, fp->offset,size);
        if (ret > 0) fp->offset += ret;
        iunlock(fp->ip);
        return (int) ret;
    }

    panic("filewrite: not implemented");
}
