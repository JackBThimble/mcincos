#include "idt.h"

typedef struct __attribute__((packed)) {
        uint16_t offset_low;
        uint16_t selector;
        uint8_t ist;
        uint8_t type_attr;
        uint16_t offset_mid;
        uint32_t offset_high;
        uint32_t zero;
} idt_entry_t;

typedef struct __attribute__((packed)) {
        uint16_t limit;
        uint64_t base;
} idtr_t;

static idt_entry_t g_idt[256];

extern void isr_stub_table(void);

void idt_set_gate(uint8_t vec, void (*isr)(void), uint8_t type_attr) {
    uint64_t addr = (uint64_t)isr;
    g_idt[vec] =
        (idt_entry_t){.offset_low = (uint16_t)(addr & 0xffff),
                      .selector = 0x28,
                      .ist = 0,
                      .type_attr = type_attr,
                      .offset_mid = (uint16_t)((addr >> 16) & 0xffff),
                      .offset_high = (uint32_t)((addr >> 32) & 0xffffffff),
                      .zero = 0};
}

static inline void lidt(const idtr_t *idtr) {
    __asm__ volatile(".intel_syntax noprefix\n"
                     "lidt [rax]\n"
                     ".att_syntax prefix\n"
                     :
                     : "a"(idtr)
                     : "memory");
}

void idt_init(void) {
    void (**table)(void) = (void (**)(void))&isr_stub_table;
    for (uint8_t i = 0; i < 32; i++) {
        idt_set_gate(i, table[i], IDT_TYPE_INTERRUPT);
    }

    idtr_t idtr = {.limit = (uint16_t)(sizeof(g_idt) - 1),
                   .base = (uint64_t)&g_idt[0]};

    lidt(&idtr);
    __asm__ volatile("sti");
}
