#pragma once

#define GDT_NULL 0
#define GDT_KCODE 1
#define GDT_KDATA 2
#define GDT_UDATA 3
#define GDT_UCODE 4
#define GDT_TSS 5
#define GDT_TSS_HI 6
#define GDT_COUNT 7

#ifdef __ASSEMBLER__
#define GDT_SEL(idx, rpl) ((idx << 3) | (rpl))
#else
#include <stdint.h>
#define GDT_SEL(idx, rpl) ((uint16_t)(((idx) << 3) | (rpl)))

struct tss {
        uint32_t reserved0;
        uint64_t rsp0;
        uint64_t rsp1;
        uint64_t rsp2;
        uint64_t reserved1;
        uint64_t ist1;
        uint64_t ist2;
        uint64_t ist3;
        uint64_t ist4;
        uint64_t ist5;
        uint64_t ist6;
        uint64_t ist7;
        uint64_t reserved2;
        uint16_t reserved3;
        uint16_t iomap_base;
} __attribute__((packed));

void gdt_init(void);
void gdt_set_kernel_stack(uint64_t rsp0);
void gdt_set_ist(uint8_t index, uint64_t rsp);
#endif

#define KERNEL_CS GDT_SEL(GDT_KCODE, 0)
#define KERNEL_DS GDT_SEL(GDT_KDATA, 0)
#define USER_DS GDT_SEL(GDT_UDATA, 3)
#define USER_CS GDT_SEL(GDT_UCODE, 3)
#define TSS_SEL GDT_SEL(GDT_TSS, 0)
