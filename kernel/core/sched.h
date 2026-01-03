#pragma once
#include "../arch/x86_64/cpu/regs.h"
#include <stdint.h>

typedef enum {
    THREAD_READY,
    THREAD_RUNNING,
    THREAD_BLOCKED,
} thread_state_t;

typedef struct thread {
        isr_frame_t frame;
        uint64_t kstack_top;
        thread_state_t state;
        struct thread *next;
} thread_t;

void sched_init(thread_t *bootstrap);
void sched_add(thread_t *t);
void sched_on_tick(isr_frame_t *frame);

void thread_init_user(thread_t *t, uint64_t entry, uint64_t user_stack_top,
                      uint64_t kstack_top);
