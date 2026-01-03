#pragma once
#include <stdint.h>

typedef struct cpu_local {
        uint64_t kernel_rsp;
        uint64_t user_rsp;
} cpu_local_t;

extern cpu_local_t g_cpu_local;
