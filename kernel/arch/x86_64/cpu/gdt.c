#include "gdt.h"

#include <stddef.h>
#include <stdint.h>

#define GDT_ACCESS_CODE 0x9a
#define GDT_ACCESS_DATA 0x92
#define GDT_ACCESS_TSS 0x89
#define GDT_ACCESS_RING3 0x60

#define GDT_FLAG_GRAN 0x80
#define GDT_FLAG_SIZE 0x40
#define GDT_FLAG_LONG 0x20

struct __attribute__((packed)) gdt_ptr {
        uint16_t limit;
        uint64_t base;
};

static struct tss g_tss;
static uint64_t gdt[GDT_COUNT] __attribute__((aligned(16)));
static struct gdt_ptr gdtr = {
    .limit = sizeof(gdt) - 1,
    .base = (uint64_t)gdt,
};

extern void gdt_flush(uint64_t);

static uint64_t gdt_entry(uint32_t base, uint32_t limit, uint8_t access,
                          uint8_t flags) {
    uint64_t desc = 0;
    desc |= (limit & 0xffff);
    desc |= (uint64_t)(base & 0xffff) << 16;
    desc |= (uint64_t)((base >> 16) & 0xff) << 32;
    desc |= (uint64_t)access << 40;
    desc |= (uint64_t)((limit >> 16) & 0x0f) << 48;
    desc |= (uint64_t)(flags & 0xf0) << 48;
    desc |= (uint64_t)((base >> 24) & 0xff) << 56;
    return desc;
}

static void gdt_set_tss(uint64_t base, uint32_t limit) {
    uint64_t low = 0;
    uint64_t high = 0;

    low |= (limit & 0xffff);
    low |= (base & 0xffff) << 16;
    low |= ((base >> 16) & 0xff) << 32;
    low |= (uint64_t)GDT_ACCESS_TSS << 40;
    low |= (uint64_t)((limit >> 16) & 0x0f) << 48;
    low |= ((base >> 24) & 0xff) << 56;

    high = base >> 32;

    gdt[GDT_TSS] = low;
    gdt[GDT_TSS_HI] = high;
}

void gdt_init(void) {
    gdt[GDT_NULL] = 0;
    gdt[GDT_KCODE] =
        gdt_entry(0, 0xfffff, GDT_ACCESS_CODE, GDT_FLAG_GRAN | GDT_FLAG_LONG);
    gdt[GDT_KDATA] =
        gdt_entry(0, 0xfffff, GDT_ACCESS_DATA, GDT_FLAG_GRAN | GDT_FLAG_SIZE);
    gdt[GDT_UDATA] = gdt_entry(0, 0xfffff, GDT_ACCESS_DATA | GDT_ACCESS_RING3,
                               GDT_FLAG_GRAN | GDT_FLAG_SIZE);
    gdt[GDT_UCODE] = gdt_entry(0, 0xfffff, GDT_ACCESS_CODE | GDT_ACCESS_RING3,
                               GDT_FLAG_GRAN | GDT_FLAG_LONG);

    g_tss.iomap_base = sizeof(g_tss);
    gdt_set_tss((uint64_t)&g_tss, sizeof(g_tss) - 1);

    gdt_flush((uint64_t)&gdtr);
}

void gdt_set_kernel_stack(uint64_t rsp0) { g_tss.rsp0 = rsp0; }

void gdt_set_ist(uint8_t index, uint64_t rsp) {
    switch (index) {
    case 1:
        g_tss.ist1 = rsp;
        break;
    case 2:
        g_tss.ist2 = rsp;
        break;
    case 3:
        g_tss.ist3 = rsp;
        break;
    case 4:
        g_tss.ist4 = rsp;
        break;
    case 5:
        g_tss.ist5 = rsp;
        break;
    case 6:
        g_tss.ist6 = rsp;
        break;
    case 7:
        g_tss.ist7 = rsp;
        break;
    default:
        break;
    }
}
