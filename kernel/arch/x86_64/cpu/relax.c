#include "relax.h"
#include "cpuid.h"
#include "tsc.h"
#include <emmintrin.h>
#include <immintrin.h>

static int g_has_tpause = 0;
static uint64_t g_tpause_step = 1000;
static uint64_t g_tpause_deadline = 0;

static int cpu_has_waitpkg(void) {
    if (cpuid_max_leaf() < 7)
        return 0;
    cpuid_regs_t r = cpuid(7, 0);
    return (r.ecx >> 5) & 1u;
}

int cpu_has_tpause(void) { return g_has_tpause; }

void cpu_relax_set_tpause_step(uint64_t tpause_step_cycles) {
    if (tpause_step_cycles)
        g_tpause_step = tpause_step_cycles;
}

void cpu_relax_init(uint64_t tpause_step_cycles) {
    g_has_tpause = cpu_has_waitpkg();
    if (tpause_step_cycles)
        g_tpause_step = tpause_step_cycles;
    g_tpause_deadline = rdtsc();
}

void cpu_relax(void) {
    if (!g_has_tpause) {
        _mm_pause();
        return;
    }

    g_tpause_deadline += g_tpause_step;

#if defined(__WAITPKG__) && defined(__has_builtin) &&                          \
    __has_builtin(__builtin_ia32_tpause)
    _tpause(0, g_tpause_deadline);
#elif defined(__WAITPKG__)
    uint32_t lo = (uint32_t)g_tpause_deadline;
    uint32_t hi = (uint32_t)(g_tpause_deadline >> 32);
    __asm__ volatile(".intel_syntax noprefix\n"
                     "tpause\n"
                     ".att_syntax prefix\n"
                     :
                     : "a"(lo), "d"(hi), "c"(0)
                     : "cc");
#else
    _mm_pause();
#endif
}
