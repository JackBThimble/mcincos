#pragma once
#include <stdint.h>

typedef void (*timer_cb_t)(void *arg);

typedef struct timer_event {
        uint64_t deadline_ns;
        timer_cb_t cb;
        void *arg;
        struct timer_event *next;
} timer_event_t;

void timerq_insert(timer_event_t *ev);
uint64_t timerq_next_deadline(void);
void timerq_run_expired(uint64_t now);
