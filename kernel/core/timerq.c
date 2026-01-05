#include "timerq.h"
#include <stddef.h>

static timer_event_t *g_head;

void timerq_insert(timer_event_t *ev) {
    timer_event_t **pp = &g_head;
    while (*pp && (*pp)->deadline_ns <= ev->deadline_ns)
        pp = &(*pp)->next;
    ev->next = *pp;
    *pp = ev;
}

uint64_t timerq_next_deadline(void) { return g_head ? g_head->deadline_ns : 0; }

void timerq_run_expired(uint64_t now) {
    while (g_head && g_head->deadline_ns <= now) {
        timer_event_t *ev = g_head;
        g_head = ev->next;
        ev->next = NULL;
        if (ev->cb)
            ev->cb(ev->arg);
    }
}
