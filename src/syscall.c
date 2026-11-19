#include "syscall.h"
#include "cpu.h"
#include "lock.h"
#include "log.h"
#include "main.h"
#include "printk.h"
#include "memlayout.h"
#include "tty.h"
#include "string.h"

static uint32_t argraw(const int n) {
    const struct trapframe *trapframe = get_trapframe();
    // ReSharper disable once CppDFAUnreachableCode
    switch (n) {
        case 0:
            return trapframe->a0;
        case 1:
            return trapframe->a1;
        case 2:
            return trapframe->a2;
        case 3:
            return trapframe->a3;
        default:
            panic("argraw");
            return -1;
    }
}

/* === PROCESS SYSCALLS === */

static uint32_t sys_sbrk(void) {
    size_t n = (size_t) argraw(0);

    struct proc *p = getproc();

    if (n % 4 != 0) n += 4 - (n % 4); // Round up to nearest multiple of 4
    if ((size_t) p->pbrk + n >= HEAPLIM) {
        // Can't allocate
        panic("out of heap space");
    }

    // Note: lazy allocation
    p->pbrk = p->pbrk + n;
    return (uint32_t) p->pbrk;
}

static uint32_t sys_termctl(void) {
    enum termop op = (enum termop) argraw(0);
    return term_op(op);
}

static uint32_t sys_fork(void) {
    return fork();
}

static uint32_t sys_exit(void) {
    uint32_t code = (uint32_t) argraw(0);
    exit(code);
    return -1; // shouldn't actually return but like what if? y'know?
}

static uint32_t sys_wait(void) {
    uint32_t *code = (uint32_t *) argraw(0);
    return wait(code);
}

static uint32_t sys_sleep(void) {
    unsigned int milliseconds = (unsigned int) argraw(0);
    unsigned int n = milliseconds / TICK;

    unsigned int ticks0 = cpu.ticks;
    acquire();
    while (cpu.ticks-ticks0 < n) {
        sleep_prepare(&cpu.ticks);
        release();
        sleep();
        acquire();
    }
    release();
    return 0;
}

/* === FILE SYSCALLS === */

// Find a free file in the processs's file table and make it point to the given file
// This "gives" the file to the process and returns its file descriptor
static int fdalloc(struct file *f) {
    struct proc *p = getproc();
    for (int fd = 0; fd < NOPEN; fd++) {
        if (p->ofiles[fd] == 0) {
            p->ofiles[fd] = f;
            return fd;
        }
    }
    return -1;
}

static struct file *getfile(int fd) {
    struct proc *p = getproc();
    return p->ofiles[fd];
}

static void clearfd(int fd) {
    struct proc *p = getproc();
    p->ofiles[fd] = NULL;
}

static uint32_t sys_read(void) {
    const int fd = (int) argraw(0);
    void *buffer = (void *)argraw(1);
    const int n = (int) argraw(2);

    // TODO: devices n allat
    if (fd == 0 || fd == 1 || fd == 2) {
        return term_read(buffer, n);
    }

    struct file *file = getfile(fd);
    if (file == NULL) return -1;

    return fileread(file, buffer, n);
}

static uint32_t sys_write(void) {
    const int fd = (int) argraw(0);
    void *buffer = (void *)argraw(1);
    const int n = (int) argraw(2);

    if (fd == 0 || fd == 1 || fd == 2) {
        term_write(buffer, n);
        return n;
    }

    struct file *file = getfile(fd);
    if (file == NULL) return -1;

    return filewrite(file, buffer, n);
}

static uint32_t sys_exec(void) {
    char *path = (char *) argraw(0);
    char **argv = (char **) argraw(1);

    // Count args
    int argc = 0;
    while (argv[argc] != NULL) argc++;

    return kexec(path, argc, argv);
}

static uint32_t sys_link(void) {
    char *oldpath = (char *) argraw(0);
    char *newpath = (char *) argraw(1);

    begin_op();

    // Get inode of old file
    struct inode *target = namelookup(oldpath);
    if (target == NULL || target->d.type != F_FILE) {
        return -1;
    }
    ilock(target);

    // Add a new directory entry pointing to it

    // Increment nlink
    target->d.nlink++;
    iupdate(target);
    iunlock(target);

    // Get parent of new
    char name[DIRSIZE+1];
    struct inode *parent = parentlookup(newpath, name);
    if (parent == NULL) {
        goto fail;
    }
    ilock(parent);

    if (add_direntry(parent, newpath, target->inum) == 0) {
        iunlock(parent);
        iput(parent);
        goto fail;
    }

    iput(target);
    iunlock(parent);
    iput(parent);
    end_op();
    return 0;

    fail:
    target->d.nlink--;
    iupdate(target);
    iunlock(target);
    iput(target);
    return -1;

}

// Creates a new inode and returns it locked
static struct inode *create(char *path, enum file_type type) {
    // Get parent
    char name[DIRSIZE+1];
    struct inode *parent = parentlookup(path, name);
    if (parent == NULL) return NULL;
    ilock(parent);
    if (parent->d.type != F_DIRECTORY) goto fail;

    // Check if name already exists. If it's a file, return it
    struct inode *ip;
    if ((ip = dirlookup(parent, name, NULL)) != NULL) {
        iunlock(parent);
        iput(parent);
        ilock(ip);
        if (type == F_FILE && ip->d.type == F_FILE) {
            return ip;
        }
        iunlock(ip);
        iput(ip);
        return NULL;
    }

    // Create inode
    ip = ialloc(type);
    if (ip == NULL) goto fail;
    ilock(ip);

    ip->d.nlink = 1;
    iupdate(ip);

    // If directory, add default entries
    if (type == F_DIRECTORY) {
        add_direntry(ip, ".", ip->inum);
        add_direntry(ip, "..", parent->inum);
    }

    // Link to parent
    add_direntry(parent, name, ip->inum);

    iunlock(parent);
    iput(parent);
    return ip;

    fail:
    iunlock(parent);
    iput(parent);
    return NULL;
}

static uint32_t sys_open(void) {
    char *path = (char *) argraw(0);
    unsigned int mode = (int) argraw(1);

    if ((mode & O_WRONLY) && (mode & O_RDONLY)) return -1;

    begin_op();

    struct inode *ip;
    if (mode & O_CREAT) {
        // Returns the inode locked
        ip = create(path, F_FILE);
        if (ip == NULL) {
            end_op();
            return -1;
        }
    }
    else {
        ip = namelookup(path);
        if (ip == NULL) {
            end_op();
            return -1;
        }
        ilock(ip);
        // directories shouldn't be writable
        if (ip->d.type == F_DIRECTORY && mode != O_RDONLY) {
            iunlock(ip);
            iput(ip);
            end_op();
            return -1;
        }
    }


    // Allocate a file
    struct file *fp = filealloc();
    if (fp == NULL) {
        iunlock(ip);
        iput(ip);
        end_op();
        return -1;
    }

    // Allocate a file descriptor
    int fd = fdalloc(fp);
    if (fd == -1) {
        iunlock(ip);
        iput(ip);
        fileclose(fp);
        end_op();
        return -1;
    }

    // Populate file (references already incremented in filealloc())
    fp->type = FD_INODE;
    fp->offset = 0;
    fp->readable = (char) (mode & O_RDWR) || ((mode & O_RDONLY) && !(mode & O_WRONLY));
    fp->writable = (char) (mode & O_RDWR) || ((mode & O_WRONLY) && !(mode & O_RDONLY));
    fp->ip = ip;

    if (mode & O_TRUNC && ip->d.type == F_FILE) {
        itrunc(ip);
    }

    iunlock(ip);
    end_op();
    return fd;
}

static uint32_t sys_mkdir(void) {
    char *path = (char *) argraw(0);

    begin_op();

    // Try creating
    struct inode *ip = create(path, F_DIRECTORY);
    if (ip == NULL) {
        end_op();
        return -1;
    }

    iunlock(ip);
    iput(ip);
    end_op();
    return 0;

}

static uint32_t sys_dup(void) {
    // Duplicate the file and get a new file descriptor
    int oldfd = (int) argraw(0);

    struct file *oldfp = getfile(oldfd);
    if (oldfp == NULL) return -1;

    int fd = fdalloc(filedup(oldfp));
    return fd;
}

static uint32_t chdir(char *path) {
    begin_op();
    struct inode *ip = namelookup(path);
    if (ip == NULL) {
        printk("chdir: path not found\n\r");
        return -1;
    }
    ilock(ip);
    if (ip->d.type != F_DIRECTORY) {
        printk("chdir: path %s is not a directory\n\r", path);
        iunlock(ip);
        iput(ip);
        return -1;
    }
    iunlock(ip);

    struct proc *p = getproc();
    iput(p->cwd);

    end_op();
    p->cwd = ip;
    return 0;
}

static uint32_t sys_chdir(void) {
    char *path = (char *) argraw(0);
    return chdir(path);
}

static uint32_t sys_close(void) {
    int fd = (int) argraw(0);
    struct file *fp = getfile(fd);
    if (fp == NULL) return -1;
    fileclose(fp);
    clearfd(fd);
    return 0;
}

static int dirempty(struct inode *ip) {
    struct direntry de;
    for (int off = 2 * sizeof(struct direntry); off < ip->d.size; off += sizeof(struct direntry)) {
        if (readi(ip, &de, off, sizeof(struct direntry)) != sizeof(struct direntry)) {
            panic("dirempty");
        }
        if (de.inode != 0) return 0;
    }
    return 1;
}

static uint32_t sys_unlink(void) {
    char *path = (char *) argraw(0);

    begin_op();

    // Parent dir
    char name[DIRSIZE+1];
    struct inode *dp = parentlookup(path, name);
    if (dp == NULL) {
        end_op();
        return -1;
    }
    ilock(dp);

    // Can't unlink . and ..
    if (strncmp(name, ".", DIRSIZE+1) == 0 || strncmp(name, "..", DIRSIZE+1) == 0) {
        iunlock(dp);
        iput(dp);
        end_op();
        return -1;
    }

    // Target inode
    uint32_t off;
    struct inode *ip = dirlookup(dp, name, &off);
    if (ip == NULL) {
        iunlock(dp);
        iput(dp);
        end_op();
        return -1;
    }
    ilock(ip);

    if (ip->d.nlink < 1) panic("unlink");

    // Can't delete non-empty directory
    if (ip->d.type == F_DIRECTORY) {
        if (!dirempty(ip)) {
            iunlock(ip);
            iput(ip);
            iunlock(dp);
            iput(dp);
            end_op();
            return -1;
        }
    }

    // Remove from parent directory
    struct direntry de = {0};
    if (writei(dp, &de, off, sizeof(de)) != sizeof(de)) {
        panic("sys_unlink");
    }

    // If this was the last entry
    if (off + sizeof(de) >= dp->d.size) {
        dp->d.size = off;
        iupdate(dp);
    }

    // If this was a directory, we also delete the .. entry
    if (ip->d.type == F_DIRECTORY) {
        dp->d.nlink--;
        iupdate(dp);
    }

    iunlock(dp);
    iput(dp);

    // Remove link
    ip->d.nlink--;
    iupdate(ip);
    iunlock(ip);
    iput(ip); // will see that nlink == 0 and delete the inode

    end_op();
    return 0;
}

static char *prepend(char *str, char *i) {
    size_t n = strlen(str);
    i -= n;
    memcpy(i, str, n);
    return i;
}

static uint32_t sys_getcwd(void) {
    char *buf = (char *) argraw(0);

    uint32_t cwdino = getproc()->cwd->inum;
    uint32_t lastino = cwdino;

    char path[128] = {0};
    char *i = &path[127];

    while (getproc()->cwd->inum != ROOTINUM) {
        // Go inside .. entry
        chdir("..");

        if (lastino != cwdino) i = prepend("/", i);

        struct inode *dp = getproc()->cwd;

        // Look for entry pointing to lastino
        ilock(dp);
        struct direntry dirent;
        for (uint32_t off = 0; off < dp->d.size; off += sizeof(struct direntry)) {
            readi(dp, &dirent, off, sizeof(dirent));
            if (dirent.inode == lastino) {
                i = prepend(dirent.name, i);
                break;
            }
        }
        lastino = dp->inum;
        iunlock(dp);
    }

    i = prepend("/", i);
    size_t n = i-path;
    memcpy(buf, i, n);

    // Get back original cwd
    getproc()->cwd = iget(cwdino);
    return (uint32_t) buf;
}

static uint32_t (*syscalls[])(void) = {
    [SYS_sbrk]          = sys_sbrk,
    [SYS_exit]          = sys_exit,
    [SYS_termctl]       = sys_termctl,
    [SYS_fork]          = sys_fork,
    [SYS_wait]          = sys_wait,
    [SYS_sleep]         = sys_sleep,

    [SYS_read]          = sys_read,
    [SYS_write]         = sys_write,
    [SYS_exec]          = sys_exec,
    [SYS_link]          = sys_link,
    [SYS_open]          = sys_open,
    [SYS_mkdir]         = sys_mkdir,
    [SYS_dup]           = sys_dup,
    [SYS_chdir]         = sys_chdir,
    [SYS_close]         = sys_close,
    [SYS_unlink]        = sys_unlink,
    [SYS_getcwd]        = sys_getcwd,
};

void syscall() {
    struct trapframe *trapframe = get_trapframe();

    const uint32_t num = trapframe->v0;

    if (num > 0 && num <= SYSDOMAIN && syscalls[num]) {
        trapframe->v0 = syscalls[num]();
    } else {
        printk("Unknown syscall: ");
        if (num <= 0) {
            printk("Less than 0\n\r");
        }
        if (num > SYSDOMAIN) {
            printk("Too big (%u)\n\r", num);
        }
        if (!syscalls[num]) {
            printk("Not implemented\n\r");
        }
        trapframe->v0 = -1;
    }
}
