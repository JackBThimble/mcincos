#pragma once
#include <stdint.h>

static inline uint64_t rdmsr(uint32_t msr) {
    uint32_t lo, hi;
    __asm__ volatile(".intel_syntax noprefix\n"
                     "rdmsr\n"
                     ".att_syntax prefix\n"
                     : "=a"(lo), "=d"(hi)
                     : "c"(msr)
                     : "memory");

    return ((uint64_t)hi << 32) | lo;
}

static inline void wrmsr(uint32_t msr, uint64_t val) {
    uint32_t lo = (uint32_t)val;
    uint32_t hi = (uint32_t)(val >> 32);
    __asm__ volatile(".intel_syntax noprefix\n"
                     "wrmsr\n"
                     ".att_syntax prefix\n"
                     :
                     : "c"(msr), "a"(lo), "d"(hi)
                     : "memory");
}

static inline uint64_t rdcr3(void) {
    uint64_t v;
    __asm__ volatile(".intel_syntax noprefix\n"
                     "mov rax, cr3\n"
                     ".att_syntax prefix\n"
                     : "=a"(v)
                     :
                     : "memory");
    return v;
}
