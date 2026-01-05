#pragma once
#include <stddef.h>
#include <stdint.h>

void pmm_init(void);

void *pmm_alloc_pages(size_t page_count);
void pmm_free_pages(void *phys, size_t pages);

uint64_t pmm_total_bytes(void);
uint64_t pmm_free_bytes(void);

uint64_t hhdm_offset(void);
static inline void *phys_to_virt(uint64_t phys) {
    return (void *)(phys + hhdm_offset());
}
static inline uint64_t virt_to_phys(const void *virt) {
    return (uint64_t)virt - hhdm_offset();
}
