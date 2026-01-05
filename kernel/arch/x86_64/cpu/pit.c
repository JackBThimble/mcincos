#include "pit.h"
#include "io.h"

#define PIT_CH0 0x40
#define PIT_CMD 0x43
#define PIT_BASE_HZ 1193182u

static uint16_t clamp_div(uint32_t div) {
    if (div < 1)
        return 1;
    if (div > 0xffff)
        return 0xffff;
    return (uint16_t)div;
}

void pit_init(uint32_t hz) {
    if (hz < 19)
        hz = 19;
    uint32_t divisor = PIT_BASE_HZ / hz;
    if (divisor == 0)
        divisor = 1;
    if (divisor > 0xffff)
        divisor = 0xffff;

    outb(PIT_CMD, 0x36);
    outb(PIT_CH0, (uint8_t)(divisor & 0xff));
    outb(PIT_CH0, (uint8_t)(divisor >> 8));
}

void pit_oneshot_init(void) {
    // Mode 0: Interrupt on terminal count (one-shot)
    // Access: lobyte/hibyte
    outb(PIT_CMD, 0x30);
}

void pit_oneshot_us(uint32_t us) {
    // div = PIT_BASE_HZ * (us / 1e6)
    // Use 64-bit to avoid overflow.
    uint64_t div64 = ((uint64_t)PIT_BASE_HZ * (uint64_t)us) / 1000000ull;
    uint16_t div = clamp_div((uint32_t)div64);

    outb(PIT_CMD, 0x30);
    outb(PIT_CH0, (uint8_t)(div & 0xff));
    outb(PIT_CH0, (uint8_t)((div >> 8) & 0xff));
}

void pit_stop(void) { pit_oneshot_us((uint32_t)0xfffffffffu); }

uint16_t pit_read_count(void) {
    outb(PIT_CMD, 0x00);
    uint8_t lo = inb(PIT_CH0);
    uint8_t hi = inb(PIT_CH0);
    return (uint16_t)(lo | ((uint16_t)hi << 8));
}
