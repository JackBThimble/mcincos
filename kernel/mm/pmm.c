#include "pmm.h"
#include "boot/boot_info.h"
#include <limine.h>
#include <stdint.h>

// extern volatile struct limine_hhdm_request limine_hhdm_request;
// extern volatile struct limine_memmap_request limine_memmap_request;
// extern volatile struct limine_executable_address_request
//     limine_executable_address_request;

extern uint8_t __kernel_start;
extern uint8_t __kernel_end;

/* 4KiB pages for now */
#define PAGE_SIZE 4096ull

static uint64_t g_hhdm = 0;
static uint8_t *g_bitmap = 0;
static uint64_t g_bitmap_bytes;
static uint64_t g_total_bytes;
static uint64_t g_free_bytes;
static uint64_t g_total_pages = 0;

static uint64_t align_down(uint64_t x, uint64_t a) { return x & ~(a - 1); }
static uint64_t align_up(uint64_t x, uint64_t a) {
    return (x + a - 1) & ~(a - 1);
}

static inline void bitmap_set(uint64_t idx) {
    g_bitmap[idx >> 3] |= (uint8_t)(1u << (idx & 7));
}
static inline void bitmap_clear(uint64_t idx) {
    g_bitmap[idx >> 3] &= (uint8_t)~(1u << (idx & 7));
}

static inline int bitmap_test(uint64_t idx) {
    return (g_bitmap[idx >> 3] >> (idx & 7)) & 1u;
}

uint64_t hhdm_offset(void) { return g_hhdm; }

static void mark_range(uint64_t base, uint64_t len, int used) {
    uint64_t start = align_down(base, PAGE_SIZE);
    uint64_t end = align_up(base + len, PAGE_SIZE);
    for (uint64_t p = start; p < end; p += PAGE_SIZE) {
        uint64_t idx = p / PAGE_SIZE;
        if (idx >= g_total_pages)
            break;
        if (used)
            bitmap_set(idx);
        else
            bitmap_clear(idx);
    }
}

static int ranges_overlap(uint64_t a0, uint64_t a1, uint64_t b0, uint64_t b1) {
    return (a0 < b1) && (b0 < a1);
}

static void kernel_phys_range(uint64_t *out_start, uint64_t *out_end) {
    // struct limine_executable_address_response *er =
    //     limine_executable_address_request.response;
    // if (!er) {
    //     for (;;)
    //         __asm__ volatile("hlt");
    // }

    uint64_t virt_base = g_boot_info.kernel_address.virtual_base;
    uint64_t phys_base = g_boot_info.kernel_address.physical_base;

    uint64_t kstart_v = (uint64_t)&__kernel_start;
    uint64_t kend_v = (uint64_t)&__kernel_end;

    uint64_t kstart_p = phys_base + (kstart_v - virt_base);
    uint64_t kend_p = phys_base + (kend_v - virt_base);

    *out_start = kstart_p;
    *out_end = kend_p;
}

static uint64_t find_max_phys(void) {
    // struct limine_memmap_response *mm = limine_memmap_request.response;
    uint64_t maxp = 0;

    for (uint64_t i = 0; i < g_boot_info.memmap.entry_count; i++) {
        struct limine_memmap_entry *e = g_boot_info.memmap.entries[i];
        if (e->type != LIMINE_MEMMAP_USABLE)
            continue;
        uint64_t end = e->base + e->length;
        if (end > maxp)
            maxp = end;
    }
    return maxp;
}

static uint64_t place_bitmap_phys(uint64_t bytes_needed, uint64_t kphys0,
                                  uint64_t kphys1) {
    struct limine_memmap_response *mm = &g_boot_info.memmap;

    for (uint64_t i = 0; i < mm->entry_count; i++) {
        struct limine_memmap_entry *e = mm->entries[i];
        if (e->type != LIMINE_MEMMAP_USABLE)
            continue;

        uint64_t region_start = align_up(e->base, PAGE_SIZE);
        uint64_t region_end = align_down(e->base + e->length, PAGE_SIZE);
        if (region_end <= region_start)
            continue;

        uint64_t region_size = region_end - region_start;

        if (bytes_needed > region_size)
            continue;

        uint64_t bmp_end = region_end;
        uint64_t bmp_start = align_down(bmp_end - bytes_needed, PAGE_SIZE);
        if (ranges_overlap(bmp_start, bmp_start + bytes_needed, kphys0, kphys1))
            continue;
        return bmp_start;
    }
    return 0;
}

void pmm_init(void) {
    g_hhdm = g_boot_info.hhdm_offset;

    struct limine_memmap_response *mm = &g_boot_info.memmap;

    uint64_t kphys0 = 0, kphys1 = 0;
    kernel_phys_range(&kphys0, &kphys1);

    uint64_t max_phys = find_max_phys();
    g_total_pages = align_up(max_phys, PAGE_SIZE) / PAGE_SIZE;
    g_bitmap_bytes = (g_total_pages + 7) / 8;

    uint64_t bmp_phys = place_bitmap_phys(g_bitmap_bytes, kphys0, kphys1);
    if (!bmp_phys) {
        for (;;)
            __asm__ volatile("hlt");
    }
    g_bitmap = (uint8_t *)((uint64_t)bmp_phys + g_hhdm);

    for (uint64_t i = 0; i < g_bitmap_bytes; i++)
        g_bitmap[i] = 0xff;

    for (uint64_t i = 0; i < mm->entry_count; i++) {
        struct limine_memmap_entry *e = mm->entries[i];
        if (e->type == LIMINE_MEMMAP_USABLE) {
            mark_range(e->base, e->length, 0);
        }
    }

    mark_range(kphys0, kphys1 - kphys0, 1);
    mark_range(bmp_phys, g_bitmap_bytes, 1);

    g_total_bytes = g_total_pages * PAGE_SIZE;

    uint64_t free_pages = 0;
    for (uint64_t i = 0; i < g_total_pages; i++) {
        if (!bitmap_test(i))
            free_pages++;
    }
    g_free_bytes = free_pages * PAGE_SIZE;
}

void *pmm_alloc_pages(size_t page_count) {
    if (page_count == 0)
        return 0;

    uint64_t run = 0;
    uint64_t run_start = 0;

    for (uint64_t i = 0; i < g_total_pages; i++) {
        if (!bitmap_test(i)) {
            if (run == 0)
                run_start = i;
            run++;
            if (run == page_count) {
                for (uint64_t j = 0; j < page_count; j++)
                    bitmap_set(run_start + j);
                g_free_bytes -= (uint64_t)page_count * PAGE_SIZE;
                return (void *)(run_start * PAGE_SIZE);
            }
        } else {
            run = 0;
        }
    }
    return 0;
}

void pmm_free_pages(void *phys, size_t pages) {
    if (!phys || pages == 0)
        return;
    uint64_t base = (uint64_t)phys;
    uint64_t start = base / PAGE_SIZE;

    for (uint64_t i = 0; i < pages; i++) {
        uint64_t idx = start + i;
        if (idx < g_total_pages)
            bitmap_clear(idx);
    }
    g_free_bytes += (uint64_t)pages * PAGE_SIZE;
}

uint64_t pmm_total_bytes(void) { return g_total_bytes; }
uint64_t pmm_free_bytes(void) { return g_free_bytes; }
