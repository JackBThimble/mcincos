#pragma once
#include <stdint.h>

void cpu_relax_init(uint64_t tpause_step_cycles);
void cpu_relax_set_tpause_step(uint64_t tpause_step_cycles);
void cpu_relax(void);
int cpu_has_tpause(void);
