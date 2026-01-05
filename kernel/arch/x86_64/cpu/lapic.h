#pragma once
#include <stdint.h>

#define LAPIC_SPURIOUS_VECTOR 0xff

void lapic_init(void);
int lapic_is_enabled(void);
void lapic_eoi(void);

void lapic_timer_set_oneshot(uint8_t vector);
void lapic_timer_set_tsc_deadline(uint8_t vector);
uint32_t lapic_timer_calibrate_pit(uint32_t pit_us);
uint32_t lapic_timer_calibrate_tsc(uint64_t tsc_hz, uint32_t us);
void lapic_timer_oneshot(uint32_t ticks);
void lapic_timer_stop(void);
