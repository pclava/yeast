#include "proc.h"
#include <stddef.h>
#include "cpu.h"
#include "kalloc.h"
#include "main.h"
#include "memlayout.h"
#include "printk.h"
#include "vm.h"
#include "trap.h"

struct proc proctable[NPROC];

static void * memset (void *dest, register int val, register size_t len)
{
    register unsigned char *ptr = (unsigned char*)dest;
    while (len-- > 0)
        *ptr++ = val;
    return dest;
}

void * memcpy(void *vdst, const void *vsrc, int n)
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

struct proc * getproc(void) {
    // int_off();
    struct proc *p = cpu.proc;
    // int_on();
    return p;
}

void procinit(void) {
    struct proc *p;
    unsigned int index = 0;
    for (p = proctable; p < &proctable[NPROC]; p++) {
        p->state = UNUSED;
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
        int found = 0;
        for (struct proc *p = proctable; p < &proctable[NPROC]; p++) {
            if (p->state == RUNNABLE) {
                switch_to(p);
                found = 1;
            }
        }
        if (!found) {
            asm volatile ("wait");
        }
    }
}

void yield(void) {
    struct proc *p = getproc();
    p->state = RUNNABLE;
    sched(p);
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
    sched(p);

    // When process switches back to kernel (say, on yield() or sleep()), we end up here
}

void forkret(void) {

    struct proc *p = getproc();
    if (p->PID == 0) {
        // First process! Was not created by a fork, and thus has not been exec'd yet
        exec(69420);
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

void sleep(void *chan) {
    struct proc *p = getproc();
    p->state = SLEEPING;
    p->chan = chan;
    sched(p);
}

void wakeup(const void *chan) {
    // Loop through process table waking up any sleeping process
    for (struct proc *p = proctable; p < &proctable[NPROC]; p++) {
        if (p->state == SLEEPING && p->chan == chan) {
            p->state = RUNNABLE;
        }
    }
}

static uint32_t *file_reg = (uint32_t *)FILE_REG;
static uint8_t *file_buffer = (uint8_t *)FILE_BUFFER;

// Loads a user program
void exec(int arg) {
    // Write all of user text and user data

    *file_reg &= ~(0b111);
    uint8_t *start = (uint8_t *)UTEXT0;
    int count = 0;
    while ((*file_reg & 0b100) == 0) {
        // Ask for text
        *file_reg |= 0b01;
        // Text should be in file_buffer
        for (int offset = 0; offset < PAGE_SIZE; offset++) {
            start[offset] = file_buffer[offset];
        }
        start += PAGE_SIZE;
        count++;
    }

    *file_reg &= ~(0b111);
    start = (uint8_t *)UDATA0;
    while ((*file_reg & 0b100) == 0) {
        // Ask for data
        *file_reg |= 0b11;
        // Text should be in file_buffer
        for (int offset = 0; offset < PAGE_SIZE; offset++) {
            start[offset] = file_buffer[offset];
        }
        start += PAGE_SIZE;
    }

    // Place var in stack
    *(uint32_t *)STACK0 = arg;
    struct PTE pte;
    pte_decode(&pte, pte_lookup(0, STACK0));

    // Setup trapframe
    struct proc *p = getproc();
    p->trapframe->pc = UTEXT0;
    p->trapframe->sp = STACK0-4; // 1 argument has been placed already
    p->pbrk = (void *)HEAP0;
}

uint32_t fork(void) {
    struct proc *p = getproc();

    // Allocate new process
    struct proc *np = pfind();
    unsigned char pid = np->PID;
    if (np == NULL) return -1; // TODO

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
