#pragma once
#include <stdint.h>

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile(".intel_syntax noprefix\n"
                     "out dx, al\n"
                     ".att_syntax prefix\n"
                     :
                     : "a"(val), "d"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t val;
    __asm__ volatile(".intel_syntax noprefix\n"
                     "in al, dx\n"
                     ".att_syntax prefix\n"
                     : "=a"(val)
                     : "d"(port));
    return val;
}

static inline void io_wait(void) { outb(0x80, 0); }
