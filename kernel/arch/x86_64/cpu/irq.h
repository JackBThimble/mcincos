#include "regs.h"
#include <stdint.h>

typedef void (*irq_handler_t)(isr_frame_t *f);

void irq_init(void);
void irq_register_handler(uint8_t irq, irq_handler_t handler);
void irq_common_handler(isr_frame_t *f);
