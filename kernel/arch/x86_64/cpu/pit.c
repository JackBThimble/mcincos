#include "pit.h"
#include "io.h"

#define PIT_CH0 0x40
#define PIT_CMD 0x43
#define PIT_BASE_HZ 1193182u

void pit_init(uint32_t hz) {
    if (hz < 19)
        hz = 19;
    uint32_t divisor = PIT_BASE_HZ / hz;

    outb(PIT_CMD, 0x36);
    outb(PIT_CH0, (uint8_t)(divisor & 0xff));
    outb(PIT_CH0, (uint8_t)((divisor >> 8) & 0xff));
}
