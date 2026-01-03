#pragma once
#include <stdint.h>

void idt_init(void);
void idt_set_gate(uint8_t vec, void (*isr)(void), uint8_t type_attr,
                  uint8_t ist);

#define IDT_TYPE_INTERRUPT 0x8e
#define IDT_TYPE_TRAP 0x8f
