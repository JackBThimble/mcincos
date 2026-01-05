#pragma once
#include <stdint.h>

typedef enum {
    TIMER_SRC_NONE = 0,
    TIMER_SRC_TSC_DEADLINE,
    TIMER_SRC_LAPIC,
} timer_source_t;

int timer_init(uint8_t vector);
timer_source_t timer_source(void);
uint8_t timer_vector(void);

void timer_oneshot_ns(uint64_t ns);
void timer_stop(void);

uint64_t timer_now_ns(void);
