#include "irq.h"
#include "lapic.h"
#include "pic.h"

#define IRQ_BASE 32

static irq_handler_t g_handlers[16];
static irq_handler_t g_vector_handlers[256];

void irq_init(void) {
    pic_init(IRQ_BASE, IRQ_BASE + 8);
    for (uint8_t i = 0; i < 16; i++)
        pic_set_mask(i);
}

void irq_register_handler(uint8_t irq, irq_handler_t handler) {
    if (irq < 16)
        g_handlers[irq] = handler;
}

void irq_register_vector_handler(uint8_t vector, irq_handler_t handler) {
    g_vector_handlers[vector] = handler;
}

void irq_common_handler(isr_frame_t *f) {
    uint8_t vec = (uint8_t)f->vector;

    if (vec >= IRQ_BASE && vec < IRQ_BASE + 16) {
        uint8_t irq = (uint8_t)(vec - IRQ_BASE);
        if (g_handlers[irq])
            g_handlers[irq](f);
        pic_send_eoi(irq);
        return;
    }

    if (g_vector_handlers[vec])
        g_vector_handlers[vec](f);

    if (lapic_is_enabled() && vec != LAPIC_SPURIOUS_VECTOR)
        lapic_eoi();
}
