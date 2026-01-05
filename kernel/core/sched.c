#include "sched.h"
#include "../arch/x86_64/cpu/cpu_local.h"
#include "../arch/x86_64/cpu/gdt.h"
#include "../arch/x86_64/cpu/idt.h"
#include "../arch/x86_64/cpu/timer.h"
#include "regs.h"
#include <stddef.h>
#include <stdint.h>

static thread_t *g_current;
static uint32_t g_quantum_ns;
static uint64_t g_slice_end_ns;

// static void sched_wake_cb(void *arg) {
//     thread_t *t = (thread_t *)arg;
//     t->state = THREAD_READY;
// }

static thread_t *pick_next(void) {
    thread_t *t = g_current->next;
    while (t != g_current && t->state != THREAD_READY)
        t = t->next;
    return (t->state == THREAD_READY) ? t : g_current;
}

static uint64_t sched_next_deadline(uint64_t now) {
    uint64_t deadline = timerq_next_deadline();

    if (g_quantum_ns && pick_next() != g_current) {
        if (!g_slice_end_ns || g_slice_end_ns <= now) {
            g_slice_end_ns = now + g_quantum_ns;
        }
        if (!deadline || g_slice_end_ns < deadline)
            deadline = g_slice_end_ns;
    } else {
        g_slice_end_ns = 0;
    }
    return deadline;
}

static void sched_arm_timer(void) {
    if (!g_quantum_ns && !timerq_next_deadline()) {
        timer_stop();
        return;
    }

    uint64_t now = timer_now_ns();
    uint64_t deadline = sched_next_deadline(now);

    if (!deadline) {
        timer_stop();
        return;
    }

    uint64_t delta = (deadline > now) ? (deadline - now) : 1;
    timer_oneshot_ns(delta);
}
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

void sched_set_quantum_ns(uint64_t ns) {
    g_quantum_ns = ns;
    if (!ns)
        timer_stop();
    else
        sched_arm_timer();
}

void sched_add(thread_t *t) {
    if (!g_current) {
        sched_init(t);
        return;
    }

    t->next = g_current->next;
    g_current->next = t;

    g_slice_end_ns = 0;
    sched_arm_timer();
}

void sched_start(void) { sched_arm_timer(); }

void sched_on_tick(isr_frame_t *frame) {
    if (!g_current)
        return;

    uint64_t now = timer_now_ns();
    timerq_run_expired(now);

    if ((frame->cs & 3) != 3) {
        sched_arm_timer();
        return;
    }

    if (!g_slice_end_ns || now < g_slice_end_ns) {
        sched_arm_timer();
        return;
    }

    g_current->frame = *frame;

    thread_t *next = pick_next();
    if (next != g_current) {
        g_current->state = THREAD_READY;
        next->state = THREAD_RUNNING;
        g_current = next;

        g_cpu_local.kernel_rsp = next->kstack_top;
        gdt_set_kernel_stack(next->kstack_top);

        *frame = next->frame;
    }

    g_slice_end_ns = 0;
    sched_arm_timer();
}
