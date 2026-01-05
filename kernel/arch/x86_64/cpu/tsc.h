#pragma once
#include <immintrin.h>
#include <stdint.h>
#include <x86intrin.h>

static inline uint64_t rdtsc(void) {
    _mm_lfence();
    return (uint64_t)_rdtsc();
}
