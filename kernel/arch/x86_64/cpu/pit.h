#pragma once
#include <stdint.h>
void pit_oneshot_init(void);
void pit_oneshot_us(uint32_t us);
uint16_t pit_read_count(void);
void pit_stop(void);
