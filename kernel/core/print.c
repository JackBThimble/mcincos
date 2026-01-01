#include <stdint.h>
#include "print.h"

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("out dx, al" :: "d"(port), "a"(val));
}

void kprint(const char *s) {
    while (*s) {
        outb(0x3f8, *s++);
    }
}
