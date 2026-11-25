#include "file.h"

#include "log.h"
#include "printk.h"
#include "string.h"

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

int filestat(struct file *fp, struct stat *st) {
    if (fp->type == FD_INODE) {
        memset(st, 0, sizeof(*st));
        ilock(fp->ip);
        st->size = fp->ip->d.size;
        st->ino = fp->ip->inum;
        st->type = fp->ip->d.type;
        st->nlink = fp->ip->d.nlink;
        iunlock(fp->ip);
        return 0;
    }
    return -1;
}