#include "mmio.h"
#include "../boot/boot_info.h"
#include "immintrin.h"
#include "vmm.h"
#include <stdint.h>

#define PAGE_MASK (PAGE_SIZE - 1ull)

static inline uintptr_t align_down(uintptr_t x) { return x & ~PAGE_MASK; }
static inline uintptr_t align_up(uintptr_t x) {
    return (x + PAGE_MASK) & ~PAGE_MASK;
}

/*
 * IMPORTANT:
 * - Devices should not be mapped as write-back cache
 * - Easiest correct approach early: PCD=1, PWT=1 for UC-like behavior.
 * - Later implement PAT for true UC/WC/WT selections.
 */

uintptr_t mmio_map(uintptr_t phys, size_t size, const mmio_opts_t *opts) {
    if (size == 0)
        return 0;

    mmio_opts_t def = {
        .cache = MMIO_CACHE_UC,
        .writable = 1,
        .global = 1,
    };

    if (!opts)
        opts = &def;

    uintptr_t p0 = align_down(phys);
    uintptr_t p1 = align_up(phys + size);

    // simple kernel policy
    // map mmio into the higher half direct map region for convenience
    uintptr_t v0 = p0 + (uintptr_t)g_boot_info.hhdm_offset;

    uint64_t flags =
        PTE_P | (opts->writable ? PTE_W : 0) | (opts->global ? PTE_G : 0);

    // Cache control (simple & safe early): UC-ish via PCD+PWT
    // TODO: PAT for real WC.
    flags |= PTE_PCD | PTE_PWT;

    for (uintptr_t p = p0, v = v0; p < p1; p += PAGE_SIZE, v += PAGE_SIZE) {

        // IMPORTANT: vmm_map_page strips PCD/PWT. So for MMIO we must map
        // directly.
        // Workaround: call vmm_map_page_space on kernel space? No, it strips
        // too.
        //
        // So: add a VMM internal API for "raw map" OR (easier now) allow MMIO
        // cache flags via a separate vmm_map_page_raw(). For now, simplest:
        // temporarily call a raw local helper by duplicating the mapping logic
        // is too much.
        //
        // Better: allow MMIO flags ONLY from mmio by adding
        // vmm_map_page_mmio().
        //
        // For now, we’ll assume you will add vmm_map_page_mmio below.
        if (vmm_map_page_mmio((uint64_t)v, (uint64_t)p, flags) != 0)
            return 0;
    }

    return v0 + (phys - p0);
}

void mmio_unmap(uintptr_t virt, size_t size) {
    if (size == 0)
        return;
    uintptr_t v0 = align_down(virt);
    uintptr_t v1 = align_up(virt + size);

    for (uintptr_t v = v0; v < v1; v += PAGE_SIZE) {
        vmm_unmap_page((uint64_t)v);
    }
}
