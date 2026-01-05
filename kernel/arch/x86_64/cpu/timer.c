#include "timer.h"
#include "cpuid.h"
#include "lapic.h"
#include "msr.h"
#include "pit.h"
#include "relax.h"
#include "tsc.h"

#include <immintrin.h>
#include <stddef.h>
#include <stdint.h>
#include <x86intrin.h>

#define IA32_TSC_DEADLINE 0x6e0
#define TIMER_CAL_US 10000u

static timer_source_t g_src = TIMER_SRC_NONE;
static uint8_t g_vector = 0;

/*
 * Single fixed-point conversion path:
 *
 *   ticks = (ns * scale) >> 32
 *   scale = (tick_num / tick_den) * 2^32
 *
 * where tick_num / tick_den is "ticks per nanosecond".
 *
 * - For TSC-deadline: tick_num = tsc_hz, tick_den = 1e9
 * - For LAPIC:        tick_num = (ticks_per_us * 1000), tick_den = 1e9
 *                     (because ticks_per_us / 1000 = ticks_per_ns)
 */
static uint64_t g_tick_num = 0;
static uint64_t g_tick_den = 1;
static uint64_t g_scale = 0;
static uint32_t g_scale_shift = 32;

static inline uint64_t rdtsc_ordered(void) {
    _mm_lfence();
    uint64_t t = _rdtsc();
    _mm_lfence();
    return t;
}

static inline uint64_t mul_u64_div_u64(uint64_t a, uint64_t b, uint64_t d) {
    if (d == 0)
        return 0;
    uint64_t lo, hi, q, r;

    // RDX:RAX = a * b
    __asm__ volatile(".intel_syntax noprefix\n"
                     "mul rcx\n"
                     ".att_syntax prefix\n"
                     : "=a"(lo), "=d"(hi)
                     : "a"(a), "c"(b)
                     : "cc");

    // if hi >= d, (hi:lo)/d would overflow 64-bit quotient. saturate.
    if (hi >= d)
        return UINT64_MAX;

    // quotient = (hi:lo) / d
    __asm__ volatile(".intel_syntax noprefix\n"
                     "div rcx\n"
                     ".att_syntax prefix\n"
                     : "=a"(q), "=d"(r)
                     : "a"(lo), "d"(hi), "c"(d)
                     : "cc");

    (void)r;
    return q;
}

static inline void timer_set_scale(uint64_t num, uint64_t den) {
    g_scale_shift = 32;
    g_scale = mul_u64_div_u64(num, 1ull << g_scale_shift, den);
}

static inline uint64_t ticks_from_ns(uint64_t ns) {
    if (!g_scale)
        return 0;
    // 128-bit product via MUL: RDX:RAX = ns * g_scale
    uint64_t lo, hi;
    __asm__ volatile(".intel_syntax noprefix\n"
                     "mul rcx\n"
                     ".att_syntax prefix\n"
                     : "=a"(lo), "=d"(hi)
                     : "a"(ns), "c"(g_scale)
                     : "cc");

    // (RDX:RAX) >> 32, truncated to 64-bit
    return (hi << 32) | (lo >> 32);
}

static int cpu_has_tsc_deadline(void) {
    cpuid_regs_t r = cpuid(1, 0);
    return (r.ecx >> 24) & 1u;
}

static int cpu_has_invariant_tsc(void) {
    if (cpuid_max_ext_leaf() < 0x80000007u)
        return 0;
    cpuid_regs_t r = cpuid(0x80000007u, 0);
    return (r.edx >> 8) & 1u;
}

static uint64_t tsc_hz_from_cpuid(void) {
    uint32_t max = cpuid_max_leaf();

    if (max >= 0x15) {
        cpuid_regs_t r15 = cpuid(0x15, 0);
        if (r15.eax && r15.ebx) {
            uint64_t crystal = r15.ecx;
            if (!crystal && max >= 0x16) {
                cpuid_regs_t r16 = cpuid(0x16, 0);
                if (r16.eax)
                    return (uint64_t)r16.eax * 1000000ull;
                return 0;
            }
            if (crystal)
                return (crystal * (uint64_t)r15.ebx) / (uint64_t)r15.eax;
        }
    }

    if (max >= 0x16) {
        cpuid_regs_t r16 = cpuid(0x16, 0);
        if (r16.eax)
            return (uint64_t)r16.eax * 1000000ull;
    }
    return 0;
}

static uint64_t tsc_hz_calibrate_pit(uint32_t pit_us) {
    if (pit_us == 0)
        return 0;
    pit_oneshot_us(pit_us);
    uint64_t start = rdtsc_ordered();
    while (pit_read_count() != 0)
        _mm_pause();
    uint64_t end = rdtsc_ordered();

    uint64_t delta = end - start;
    return (delta * 1000000ull) / pit_us;
}

int timer_init(uint8_t vector) {
    g_vector = vector;
    lapic_init();

    const int has_deadline = cpu_has_tsc_deadline();
    const int has_invtsc = cpu_has_invariant_tsc();

    if (!has_invtsc)
        return -1;

    uint64_t tsc_hz = tsc_hz_from_cpuid();
    if (!tsc_hz)
        tsc_hz = tsc_hz_calibrate_pit(TIMER_CAL_US);
    if (!tsc_hz)
        return -1;

    if (has_deadline) {
        g_src = TIMER_SRC_TSC_DEADLINE;

        g_tick_num = tsc_hz;
        g_tick_den = 1000000000ull; // 1e9 ns/sec
        timer_set_scale(g_tick_num, g_tick_den);

        lapic_timer_set_tsc_deadline(vector);
        timer_stop();
        return 0;
    }

    // fallback: LAPIC one-shot calibrated against TSC
    lapic_timer_set_oneshot(vector);
    uint32_t tpu = lapic_timer_calibrate_tsc(tsc_hz, TIMER_CAL_US);
    if (!tpu)
        return -1;

    g_src = TIMER_SRC_LAPIC;

    // convert TPU -> ticks per ns by multiplying numerator by 1000 and using
    // 1e9 denominator
    g_tick_num = (uint64_t)tpu * 1000ull;
    g_tick_den = 1000000000ull;
    timer_set_scale(g_tick_num, g_tick_den);

    timer_stop();
    return 0;
}

timer_source_t timer_source(void) { return g_src; }
uint8_t timer_vector(void) { return g_vector; }

void timer_oneshot_ns(uint64_t ns) {
    if (!g_tick_num || !g_scale)
        return;

    uint64_t ticks = ticks_from_ns(ns);
    if (ticks == 0)
        ticks = 1;

    if (g_src == TIMER_SRC_TSC_DEADLINE) {
        // TODO: 64 bit ticks are used, clamp for max sleep bound
        // Ensure the LVT stays in TSC-deadline mode and unmasked.
        lapic_timer_set_tsc_deadline(g_vector);
        const uint64_t now = rdtsc_ordered();
        wrmsr(IA32_TSC_DEADLINE, now + ticks);
        return;
    }

    if (g_src == TIMER_SRC_LAPIC) {
        if (ticks > 0xffffffffull)
            ticks = 0xffffffffull;
        lapic_timer_oneshot((uint32_t)ticks);
        return;
    }
}

void timer_stop(void) {
    if (g_src == TIMER_SRC_TSC_DEADLINE) {
        // clear deadline and mask the timer LVT
        wrmsr(IA32_TSC_DEADLINE, 0);
        lapic_timer_stop();
        return;
    }
    if (g_src == TIMER_SRC_LAPIC) {
        lapic_timer_stop();
        return;
    }
}

uint64_t timer_now_ns(void) {
    if (!g_tick_num)
        return 0;
    uint64_t tsc = rdtsc_ordered();
    return mul_u64_div_u64(tsc, g_tick_den, g_tick_num);
}
