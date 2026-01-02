#include "../core/panic.h"
#include "../core/print.h"
#include "regs.h"
#include <stdint.h>

static inline uint64_t read_cr2(void) {
    uint64_t v;
    __asm__ volatile(".intel_syntax noprefix\n"
                     "mov rax, cr2\n"
                     ".att_syntax prefix\n"
                     : "=a"(v)
                     :
                     : "memory");
    return v;
}

static const char *exc_name(uint64_t vec) {
    switch (vec) {
    case 0:
        return "#DE Divide Error";
    case 6:
        return "#UD Invalid Opcode";
    case 13:
        return "#GP General Protection";
    case 14:
        return "#PF Page Fault";
    default:
        return "CPU Exception";
    }
}

void isr_common_handler(isr_frame_t *f) {
    kprint("\n[EXC] ");
    kprint(exc_name(f->vector));
    kprint(" vec=");
    kprint_hex_u64(f->vector);
    kprint(" err=");
    kprint_hex_u64(f->error);

    kprint(" rip=");
    kprint_hex_u64(f->rip);

    if (f->vector == 14) {
        uint64_t cr2 = read_cr2();
        kprint(" cr2=");
        kprint_hex_u64(cr2);
    }

    panic("unhandled exception");
}
