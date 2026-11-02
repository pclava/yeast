#include "lock.h"
#include "cpu.h"

void push_off(void) {
    // Was 0
    if (cpu.intr_off == 0) int_off();
    cpu.intr_off += 1;
}

void pop_off(void) {
    if (cpu.intr_off == 0) return;
    cpu.intr_off -= 1;
    if (cpu.intr_off == 0) int_on();
}

// Note: on a single core CPU a spinlock is just disabling/enabling interrupts. there's no need to specify the lock target
// this is because, in theory, a spinlock is owned by the whole cpu, but we only have one
// the only way to interrupt kernel code is with interrupts (or exceptions, which should panic)
void acquire(void) {
    push_off();
}

void release(void) {
    pop_off();
}

void acquire_sleep(struct sleeplock *s) {
    acquire();
    // The condition check must occur in a while loop in case of **spurious wakeups**
    while (s->locked) {
        sleep_prepare(s);
        release();
        sleep();
        acquire();
    }
    s->locked = 1;
    release();
}

void release_sleep(struct sleeplock *s) {
    acquire();
    s->locked = 0;
    wakeup(s);
    release();
}