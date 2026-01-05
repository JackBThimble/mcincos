#include "lapic.h"
#include "mmio.h"
#include "msr.h"
#include "pit.h"
#include "relax.h"
#include "tsc.h"
#include <emmintrin.h>
#include <stdint.h>

#define IA32_APIC_BASE 0x1b
#define IA32_APIC_BASE_ENABLE (1u << 11)
#define IA32_APIC_BASE_X2APIC (1u << 10)

#define LAPIC_REG_TPR 0x80
#define LAPIC_REG_EOI 0xb0
#define LAPIC_REG_SVR 0xf0
#define LAPIC_REG_LVT_TIMER 0x320
#define LAPIC_REG_TMR_INITCNT 0x380
#define LAPIC_REG_TMR_CURRCNT 0x390
#define LAPIC_REG_TMR_DIV 0x3e0

#define LAPIC_SVR_ENABLE (1u << 8)
#define LAPIC_LVT_MASK (1u << 16)
#define LAPIC_LVT_TSC_DEADLINE (1u << 18)

#define LAPIC_TIMER_DIV_16 0x3

// static volatile uint32_t *g_lapic = 0;
static uint8_t g_timer_vector = 0;
static uintptr_t g_lapic_base = 0;

static inline void lapic_write(uint32_t reg, uint32_t val) {
    // *(volatile uint32_t *)((uintptr_t)g_lapic + reg) = val;
    // (void)*(volatile uint32_t *)((uintptr_t)g_lapic + reg);
    mmio_write32(g_lapic_base, reg, val);
}

static inline uint32_t lapic_read(uint32_t reg) {
    // return *(volatile uint32_t *)((uintptr_t)g_lapic + reg);
    return mmio_read32(g_lapic_base, reg);
}

int lapic_is_enabled(void) {
    return (rdmsr(IA32_APIC_BASE) & IA32_APIC_BASE_ENABLE) != 0;
}

void lapic_init(void) {
    uint64_t base = rdmsr(IA32_APIC_BASE);
    base &= ~IA32_APIC_BASE_X2APIC;
    base |= IA32_APIC_BASE_ENABLE;
    wrmsr(IA32_APIC_BASE, base);

    uint64_t phys = base & 0xfffff000ull;
    // g_lapic = (volatile uint32_t *)phys_to_virt(phys);

    if (!g_lapic_base) {
        mmio_opts_t o = {
            .cache = MMIO_CACHE_UC, .writable = 1, .global = 1, .nx = 1};
        g_lapic_base = mmio_map((uintptr_t)phys, 0x1000, &o);
    }
    lapic_write(LAPIC_REG_TPR, 0);
    lapic_write(LAPIC_REG_SVR, LAPIC_SPURIOUS_VECTOR | LAPIC_SVR_ENABLE);
}

void lapic_eoi(void) {
    // if (!g_lapic)
    if (!g_lapic_base)
        return;
    lapic_write(LAPIC_REG_EOI, 0);
}

void lapic_timer_set_oneshot(uint8_t vector) {
    g_timer_vector = vector;
    lapic_write(LAPIC_REG_TMR_DIV, LAPIC_TIMER_DIV_16);
    lapic_write(LAPIC_REG_LVT_TIMER, (uint32_t)vector);
    lapic_write(LAPIC_REG_TMR_INITCNT, 0);
}

void lapic_timer_set_tsc_deadline(uint8_t vector) {
    g_timer_vector = vector;
    lapic_write(LAPIC_REG_TMR_DIV, LAPIC_TIMER_DIV_16);
    lapic_write(LAPIC_REG_LVT_TIMER, (uint32_t)vector | LAPIC_LVT_TSC_DEADLINE);
    lapic_write(LAPIC_REG_TMR_INITCNT, 0);
}

uint32_t lapic_timer_calibrate_tsc(uint64_t tsc_hz, uint32_t us) {
    if (!g_lapic_base || !g_timer_vector || !tsc_hz || us == 0)
        return 0;

    lapic_write(LAPIC_REG_TMR_DIV, LAPIC_TIMER_DIV_16);
    lapic_write(LAPIC_REG_LVT_TIMER, (uint32_t)g_timer_vector | LAPIC_LVT_MASK);

    lapic_write(LAPIC_REG_TMR_INITCNT, 0xffffffffu);

    uint64_t tsc_delta = (tsc_hz * us) / 1000000ull;
    if (!tsc_delta)
        tsc_delta = 1;

    uint64_t start = rdtsc();
    uint64_t target = start + tsc_delta;
    while ((int64_t)(rdtsc() - target) < 0)
        cpu_relax();

    uint32_t cur = lapic_read(LAPIC_REG_TMR_CURRCNT);
    uint64_t elapsed = (uint64_t)0xffffffffu - cur;
    uint64_t tpu = elapsed / (uint64_t)us;
    if (tpu == 0)
        tpu = 1;
    lapic_timer_stop();
    return (uint32_t)tpu;
}

uint32_t lapic_timer_calibrate_pit(uint32_t pit_us) {
    if (!g_lapic_base || !g_timer_vector || pit_us == 0)
        return 0;

    lapic_write(LAPIC_REG_TMR_DIV, LAPIC_TIMER_DIV_16);
    lapic_write(LAPIC_REG_LVT_TIMER, (uint32_t)g_timer_vector | LAPIC_LVT_MASK);

    pit_oneshot_us(pit_us);
    lapic_write(LAPIC_REG_TMR_INITCNT, 0xffffffffu);

    while (pit_read_count() != 0)
        _mm_pause();

    uint32_t cur = lapic_read(LAPIC_REG_TMR_CURRCNT);
    uint64_t elapsed = (uint64_t)0xffffffffu - cur;
    uint64_t tpu = elapsed / (uint64_t)pit_us;
    if (tpu == 0)
        tpu = 1;
    lapic_timer_stop();
    return (uint32_t)tpu;
}

void lapic_timer_oneshot(uint32_t ticks) {
    if (!g_lapic_base || !g_timer_vector)
        return;
    if (ticks == 0)
        ticks = 1;

    lapic_write(LAPIC_REG_LVT_TIMER, (uint32_t)g_timer_vector);
    lapic_write(LAPIC_REG_TMR_INITCNT, ticks);
}

void lapic_timer_stop(void) {
    if (!g_lapic_base || !g_timer_vector)
        return;
    lapic_write(LAPIC_REG_LVT_TIMER, (uint32_t)g_timer_vector | LAPIC_LVT_MASK);
    lapic_write(LAPIC_REG_TMR_INITCNT, 0);
}
