#include "sched.h"
#include "../arch/x86_64/cpu/cpu_local.h"
#include "../arch/x86_64/cpu/gdt.h"
#include <stddef.h>
#include <stdint.h>

static thread_t *g_current = 0;

static void zero_thread(thread_t *t) {
    uint8_t *p = (uint8_t *)t;
    for (size_t i = 0; i < sizeof(*t); i++)
        p[i] = 0;
}

void thread_init_user(thread_t *t, uint64_t entry, uint64_t user_stack_top,
                      uint64_t kstack_top) {
    zero_thread(t);
    t->frame.rip = entry;
    t->frame.cs = USER_CS;
    t->frame.rflags = 0x202;
    t->frame.rsp = user_stack_top;
    t->frame.ss = USER_DS;
    t->kstack_top = kstack_top;
    t->state = THREAD_READY;
}

void sched_init(thread_t *bootstrap) {
    g_current = bootstrap;
    bootstrap->state = THREAD_RUNNING;
    bootstrap->next = bootstrap;
}

void sched_add(thread_t *t) {
    if (!g_current) {
        sched_init(t);
        return;
    }

    t->next = g_current->next;
    g_current->next = t;
}

static thread_t *pick_next(void) {
    thread_t *t = g_current->next;
    while (t != g_current && t->state != THREAD_READY)
        t = t->next;
    return (t->state == THREAD_READY) ? t : g_current;
}

void sched_on_tick(isr_frame_t *frame) {
    if (!g_current)
        return;

    if ((frame->cs & 3) != 3)
        return;

    g_current->frame = *frame;

    thread_t *next = pick_next();
    if (next == g_current)
        return;

    g_current->state = THREAD_READY;
    next->state = THREAD_RUNNING;
    g_current = next;

    g_cpu_local.kernel_rsp = next->kstack_top;
    gdt_set_kernel_stack(next->kstack_top);

    *frame = next->frame;
}
