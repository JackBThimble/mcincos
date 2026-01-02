#include "../arch/x86_64/cpu/idt.h"
#include "../mem/pmm.h"
#include "../mem/vmm.h"
#include "print.h"
#include <stdint.h>

void kmain(void) {
    kprintln("======================================================");
    kprintln("====================== mCincOS =======================");
    kprintln("======================================================");

    kprintln("[kernel] Initializing physical memory");
    pmm_init();
    kprintln("[kernel] Physical memory initialization successful");

    kprintln("[kernel] Initializing interrupts");
    idt_init();
    kprintln("[kernel] Interrupts initialization successful");

    kprintln("[kernel] Initializing virtual memory");
    vmm_init();
    kprintln("[kernel] Virtual memory initialization successful");

    void *page = pmm_alloc_pages(1);
    uint64_t phys = (uint64_t)page;

    uint64_t test_va = 0xffff800010000000ull;
    vmm_map_page(test_va, phys, PTE_W);
    *(volatile uint64_t *)test_va = 0x1122334455667788ull;

    kprint("[vmm] map+touch ok\n");

    for (;;) {
        __asm__ volatile("hlt");
    }
}
