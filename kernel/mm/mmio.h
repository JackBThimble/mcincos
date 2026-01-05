#pragma once
#include <stddef.h>
#include <stdint.h>

typedef enum {
    MMIO_CACHE_UC, // Uncacheable (safe default for device regs)
    MMIO_CACHE_WC, // Write-combining (useful for framebuffers)
    MMIO_CACHE_WT, // Write-through (rare)
    MMIO_CACHE_WB, // Write-back (not for device MMIO)
} mmio_cache_t;

typedef struct {
        mmio_cache_t cache;
        int global;
        int writable;
        int nx;
} mmio_opts_t;

/**
 * @brief Map a physical MMIO range and return a kernel virtual address.
 *
 * @param phys Physical base address
 * @param size Size in bytes
 * @param opts Caching + flags (NULL -> safe defaults)
 *
 * @return virtual address (higher-half), or 0 on failure.
 */
uintptr_t mmio_map(uintptr_t phys, size_t size, const mmio_opts_t *opts);

/**
 * @brief Unmap a previously mapped MMIO range (optional for now)
 */
void mmio_unmap(uintptr_t virt, size_t size);

/**
 * @brief Map a single 4K page.
 */
static inline uintptr_t mmio_map_page(uintptr_t phys) {
    return mmio_map(phys, 0x1000, NULL);
}

// Strongly typed volatile accessors (avoid UB and compiler reordering)
static inline uint32_t mmio_read32(uintptr_t base, size_t off) {
    return *(volatile uint32_t *)(base + off);
}

static inline void mmio_write32(uintptr_t base, size_t off, uint32_t v) {
    *(volatile uint32_t *)(base + off) = v;
}

static inline uint64_t mmio_read64(uintptr_t base, size_t off) {
    return *(volatile uint64_t *)(base + off);
}

static inline void mmio_write64(uintptr_t base, size_t off, uint64_t v) {
    *(volatile uint64_t *)(base + off) = v;
}
