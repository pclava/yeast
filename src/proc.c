#include "proc.h"
#include <stddef.h>
#include "cpu.h"
#include "fs.h"
#include "inode.h"
#include "kalloc.h"
#include "main.h"
#include "lock.h"
#include "log.h"
#include "memlayout.h"
#include "printk.h"
#include "string.h"
#include "mof.h"
#include "vm.h"
#include "trap.h"

struct proc proctable[NPROC];

struct proc * getproc(void) {
    // int_off();
    struct proc *p = cpu.proc;
    // int_on();
    return p;
}

void procinit(void) {
    memset(proctable, 0, sizeof(proctable));
    unsigned int index = 0;
    for (struct proc *p = proctable; p < &proctable[NPROC]; p++) {
        p->state = UNUSED;
        p->PID = index;
        index++;
    }
}

struct proc * pfind(void) {
    unsigned int index = 0;
    for (struct proc *p = proctable; p < &proctable[NPROC]; p++) {
        if (p->state == UNUSED) {
            p->PID = index;
            return p;
        }
        index++;
    }
    return NULL;
}

void scheduler() {
    while (1) {

        // This is here in case the CPU has interrupts disabled when it goes to sleep
        int_on();
        int_off();

        int found = 0;
        for (struct proc *p = proctable; p < &proctable[NPROC]; p++) {
            acquire();
            if (p->state == RUNNABLE) {
                switch_to(p);
                found = 1;
            }
            release();
        }

        if (!found) {
            asm volatile ("wait");
        }
    }
}

void yield(void) {
    struct proc *p = getproc();
    p->state = RUNNABLE;
    acquire();
    sched(p);
    release();
}

void sched(struct proc *p) {
    swtch(&p->context, &cpu.context);
}

void switch_to(struct proc *p) {
    // This code occurs in the scheduler context

    // Register trapframe
    register_trapframe(p->trapframe);

    // Set current process ID and register process with CPU
    set_asid(p->PID);
    cpu.proc = p;

    // Make running
    p->state = RUNNING;

    // Switch
    swtch(&cpu.context , &p->context);

    // When process switches back to kernel (say, on yield() or sleep()), we end up here
}

int first = 1;
void forkret(void) {
    release();
    struct proc *p = getproc();
    if (first) {
        // First process! Was not created by a fork, and thus has not been exec'd yet
        first = 0;

        // Initialize page table
        pt_init(0);

        fs_init();
        printk(KERNEL_MSG "File system initialized\n\r");

        p->cwd = namelookup("/");

        printk(KERNEL_MSG "Entering normal operation\n\r");
        if (kexec("/init", 1, (char *[]){"/init"}) == -1) {
            panic("could not exec");
        }
    }

    prepare_return();
    userret();
}

void first_proc(void) {

    // Find an open process, and assign its PID
    struct proc *p = pfind();
    if (p == NULL) return; // TODO

    // Allocate trapframe
    p->trapframe = (struct trapframe *)(kalloc() + UMEM_ALIASDIFF);
    if (p->trapframe == NULL) {
        return; // TODO: could fail
    }

    // Initialize context
    memset(&p->context, 0, sizeof(struct context));
    p->context.ra = (uint32_t)forkret;              // Set context return to forkret, so we end up there on first scheduling
    p->context.sp = KSTACK0-((p->PID+1)*PAGE_SIZE);  // Allocate a page of kernel stack

    // Initialize trapframe
    memset(p->trapframe, 0, sizeof(struct trapframe));

    // Prepare for run
    p->state = RUNNABLE;
}

void sleep_prepare(void *chan) {
    struct proc *p = getproc();
    acquire();
    if (chan == NULL) panic("tried to sleep on 0");
    p->chan = chan;
    release();
}

void sleep() {
    struct proc *p = getproc();
    acquire();
    // sleep_prepare would have set the channel, but a wakeup() may have come in the meantime, setting the channel to 0
    if (p->chan != NULL) {
        p->state = SLEEPING;
        sched(p);
    }
    release();
}

void wakeup(const void *chan) {
    // Loop through process table waking up any sleeping process
    for (struct proc *p = proctable; p < &proctable[NPROC]; p++) {
        acquire();
        if (p->chan == chan) {
            p->chan = NULL;
            if (p->state == SLEEPING) p->state = RUNNABLE;
        }
        release();
    }
}

// Note: exec'd process keeps its open files
int kexec(char *path, int argc, char **argv) {
    struct proc *p = getproc();
    struct inode *ip = NULL;
    struct mof_header hdr;

    // === Stack Arguments (part 1) ===
    // Place stack arguments into temporary buffer before we wipe the page table
    char *buf = kalloc() + UMEM_ALIASDIFF;
    char *stack_buf = buf + PAGE_SIZE;
    uint32_t n_fixed = 4+4*argc;
    uint32_t n_strings = 0;
    for (int i = 0; i < argc; i++) {
        n_strings += 1 + strlen(argv[i]);
    }
    uint32_t size = n_fixed + n_strings;
    while (size % 4 != 0) size++;

    // Pointers
    void *start = (void*)(stack_buf - size);    // where stack pointer will be
    void *usr_start = (void *)(STACKTOP - size);
    uint32_t *fixed = (uint32_t *)start;

    char *strings_start = start + n_fixed;  // where the strings start (going up)
    char *strings_start_usr = usr_start + n_fixed;

    // Write argc
    fixed[0] = argc;
    // Copy strings
    for (int i = 0; i < argc; i++) {
        // Copy into strings_start
        size_t n = 1 + strlen(argv[i]);
        memcpy(strings_start, argv[i], n);
        // Write pointer
        fixed[i+1] = (uint32_t) strings_start_usr;
        // Increment
        strings_start += n;
        strings_start_usr += n;
    }

    // === Read file into memory ===
    begin_op();
    if ((ip = namelookup(path)) == NULL) {
        printk("file not found: %s\n\r", path);
        end_op();
        return -1;
    }
    ilock(ip);

    // Clear page table
    free_pagetable(p->PID);
    p->pages = 0;

    // Read header
    if (readi(ip, &hdr, 0, sizeof(struct mof_header)) != sizeof(struct mof_header)) {
        printk("kexec: failed to read header\n\r");
        goto fail;
    }
    if (!mof_is_valid(&hdr)) {
        printk("kexec: invalid executable\n\r");
        goto fail;
    }

    // Read text
    void *text = (void *)UTEXT0;
    if (readi(ip, text, MOF_TXTOFF, hdr.text) != hdr.text) {
        goto fail;
    }

    // Read data
    void *data = (void *)UDATA0;
    if (readi(ip, data, MOF_DTAOFF(&hdr), hdr.data) != hdr.data) {
        printk(":(\n\r");
        goto fail;
    }

    iunlock(ip);
    iput(ip);
    end_op();

    // === Stack Arguments (part 2) ===
    // Copy from buffer to new user stack
    memcpy(usr_start, start, size);

    // === Prepare to execute ===
    p->trapframe->pc = hdr.entry;
    p->trapframe->sp = (uint32_t) usr_start;
    kfree(buf - UMEM_ALIASDIFF);
    return 0;

    fail:
    kfree(buf - UMEM_ALIASDIFF);
    end_op();
    iunlock(ip);
    iput(ip);
    return -1;
}

uint32_t fork(void) {
    struct proc *p = getproc();

    // Allocate new process
    struct proc *np = pfind();
    unsigned char pid = np->PID;
    if (np == NULL) return -1; // TODO

    // Initialize page table
    pt_init(pid);

    // Copy memory from parent to child
    vm_copy(p, np);

    // Allocate trapframe and copy (includes PC!)
    np->trapframe = (struct trapframe *)(kalloc() + UMEM_ALIASDIFF);
    if (np->trapframe == NULL) return -1; // TODO
    *(np->trapframe) = *(p->trapframe);
    // np->trapframe->pc += 4;

    // Set fork return value to 0
    np->trapframe->v0 = 0;

    // Initialize process and context
    memset(&np->context, 0, sizeof(struct context));
    np->context.ra = (uint32_t)forkret;
    np->context.sp = KSTACK0-((pid+1)*PAGE_SIZE);
    np->state = RUNNABLE;
    np->pbrk = p->pbrk;
    np->parent = p->PID;
    np->pages = p->pages;
    np->cwd = idup(p->cwd);

    // Duplicate file references
    memset(&np->ofiles, 0, sizeof(np->ofiles));
    for (int i = 0; i < NOPEN; i++) {
        if (p->ofiles[i]) {
            np->ofiles[i] = filedup(p->ofiles[i]);
        }
    }

    // Return pid to parent
    return pid;
}

void procdump(void) {
    for (int i = 0; i < NPROC; i++) {
        struct proc *p = &proctable[i];
        printk("%d", p->state);
    }
    printk("\n\r");
}

// A zombie process only keeps its state (ZOMBIE), parent, and exit code
void exit(uint32_t code) {
    struct proc *p = getproc();
    unsigned char pid = p->PID;
    if (pid == 0) {
        panic("tried to exit init");
    }

    // Give children to init
    for (struct proc *i = proctable; i < &proctable[NPROC]; i++) {
        if (i->state != UNUSED && i->parent == pid) {
            i->parent = 0;
            acquire();
            wakeup(&proctable[0]); // wakeup init if needed
            release();
        }
    }

    // Close all files
    for (int i = 0; i < NOPEN; i++) {
        struct file *fp = p->ofiles[i];
        if (fp == NULL) continue;

        fileclose(fp);

        p->ofiles[i] = NULL;
    }

    // Destroy process resources (note: xv6 does this when wait() is called but I don't see the point in not doing this now)
    // Trapframe
    if (p->trapframe) kfree((void *) p->trapframe - UMEM_ALIASDIFF);
    p->trapframe = NULL;
    // Page table and physical memory
    free_pagetable(p->PID);
    // Other PCB variables
    p->chan = NULL;
    p->pbrk = 0;
    p->pages = 0;

    // Mark as zombie
    p->state = ZOMBIE;

    // Save code
    p->code = code;

    // Wake up parent
    acquire();
    wakeup(&proctable[p->parent]);
    release();

    // Return to scheduler context one last time
    acquire();
    sched(p);
    release();
}

uint32_t wait(uint32_t *code) {

    struct proc *p = getproc();
    unsigned char pid = p->PID;

    acquire();
    while (1) {
        // Loop through process table
        int has_children = 0;
        for (struct proc *i = proctable; i < &proctable[NPROC]; i++) {
            // Check if child
            if (i->state != UNUSED && i->parent == pid) {
                has_children = 1;

                if (i->state == ZOMBIE) {
                    // Zombie child, collect exit code, destroy child, and return PID
                    if (code != NULL) *code = i->code;
                    int ret = i->PID;
                    memset(i, 0, sizeof(struct proc));
                    i->parent = 0;
                    i->state = UNUSED;
                    i->PID = ret;
                    release();
                    return ret;
                }

                // Child is still running, don't do anything
            }
        }

        if (!has_children) {
            release();
            return -1;
        }

        // Sleep until a child wakes us up with wakeup() (see exit())
        sleep_prepare(&proctable[pid]);
        release();
        sleep();
        acquire();
    }
}