#include "irq.h"
#include "pic.h"

#define IRQ_BASE 32

static irq_handler_t g_handlers[16];

void irq_init(void) {
    pic_init(IRQ_BASE, IRQ_BASE + 8);
    for (uint8_t i = 0; i < 16; i++)
        pic_set_mask(i);
    pic_clear_mask(0);
}

void irq_register_handler(uint8_t irq, irq_handler_t handler) {
    if (irq < 16)
        g_handlers[irq] = handler;
}

void irq_common_handler(isr_frame_t *f) {
    uint8_t irq = (uint8_t)(f->vector - IRQ_BASE);
    if (irq < 16 && g_handlers[irq])
        g_handlers[irq](f);
    pic_send_eoi(irq);
}
