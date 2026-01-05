#pragma once
#include <stdint.h>

typedef struct cpuid_regs {
        uint32_t eax, ebx, ecx, edx;
} cpuid_regs_t;

static inline cpuid_regs_t cpuid(uint32_t leaf, uint32_t subleaf) {
    cpuid_regs_t r;
    __asm__ volatile(".intel_syntax noprefix\n"
                     "cpuid\n"
                     ".att_syntax prefix\n"
                     : "=a"(r.eax), "=b"(r.ebx), "=c"(r.ecx), "=d"(r.edx)
                     : "a"(leaf), "c"(subleaf));
    return r;
}

static inline uint32_t cpuid_max_leaf(void) { return cpuid(0, 0).eax; }
static inline uint32_t cpuid_max_ext_leaf(void) {
    return cpuid(0x80000000u, 0).eax;
}
