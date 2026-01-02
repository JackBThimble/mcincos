#include "vmm.h"
#include "../core/print.h"
#include "pmm.h"

static inline uint64_t align_down(uint64_t x) { return x & ~(PAGE_SIZE - 1); }

static inline uint64_t read_cr3(void) {
    uint64_t v;
    __asm__ volatile(".intel_syntax noprefix\n"
                     "mov rax, cr3\n"
                     ".att_syntax prefix\n"
                     : "=a"(v)
                     :
                     : "memory");
    return v;
}

static inline void write_cr3(uint64_t v) {
    __asm__ volatile(".intel_syntax noprefix\n"
                     "mov cr3, rax\n"
                     ".att_syntax prefix\n"
                     :
                     : "a"(v)
                     : "memory");
}

static inline void invlpg(uint64_t va) {
    __asm__ volatile(".intel_syntax noprefix\n"
                     "invlpg [rax]\n"
                     ".att_syntax prefix\n"
                     :
                     : "a"(va)
                     : "memory");
}

static inline uint16_t idx_pml4(uint64_t va) { return (va >> 39) & 0x1ff; }
static inline uint16_t idx_pdpt(uint64_t va) { return (va >> 30) & 0x1ff; }
static inline uint16_t idx_pd(uint64_t va) { return (va >> 21) & 0x1ff; }
static inline uint16_t idx_pt(uint64_t va) { return (va >> 12) & 0x1ff; }

static uint64_t g_kernel_cr3_phys = 0;

static inline uint64_t *pt_virt(uint64_t pt_phys) {
    return (uint64_t *)phys_to_virt(pt_phys);
}

static uint64_t alloc_pt_page_phys(void) {
    void *p = pmm_alloc_pages(1);
    if (!p)
        return 0;
    uint64_t phys = (uint64_t)p;

    uint64_t *v = pt_virt(phys);
    for (size_t i = 0; i < 512; i++)
        v[i] = 0;
    return phys;
}

static uint64_t ensure_pt(uint64_t pml4_phys, uint64_t va, uint64_t flags) {
    uint64_t *pml4 = pt_virt(pml4_phys);
    uint16_t i4 = idx_pml4(va);

    if (!(pml4[i4] & PTE_P)) {
        uint64_t new_pdpt = alloc_pt_page_phys();
        if (!new_pdpt)
            return 0;
        pml4[i4] = new_pdpt | PTE_P | PTE_W | (flags & PTE_U);
    }

    uint64_t pdpt_phys = pml4[i4] & ~0xfffull;
    uint64_t *pdpt = pt_virt(pdpt_phys);
    uint16_t i3 = idx_pdpt(va);

    if (!(pdpt[i3] & PTE_P)) {
        uint64_t new_pd = alloc_pt_page_phys();
        if (!new_pd)
            return 0;
        pdpt[i3] = new_pd | PTE_P | PTE_W | (flags & PTE_U);
    }
    uint64_t pd_phys = pdpt[i3] & ~0xfffull;
    uint64_t *pd = pt_virt(pd_phys);
    uint64_t i2 = idx_pd(va);

    if (!(pd[i2] & PTE_P)) {
        uint64_t new_pt = alloc_pt_page_phys();
        if (!new_pt)
            return 0;
        pd[i2] = new_pt | PTE_P | PTE_W | (flags & PTE_U);
    }
    return pd[i2] & ~0xfffull;
}

int vmm_map_page(uint64_t virt, uint64_t phys, uint64_t flags) {
    virt = align_down(virt);
    phys = align_down(phys);

    uint64_t cr3 = read_cr3() & ~0xfffull;
    uint64_t pt_phys = ensure_pt(cr3, virt, flags);
    if (!pt_phys)
        return -1;

    uint64_t *pt = pt_virt(pt_phys);
    uint16_t i1 = idx_pt(virt);

    pt[i1] = (phys & ~0xfffull) | (flags & ~PTE_NX) | PTE_P;
    if (flags & PTE_NX)
        pt[i1] |= PTE_NX;

    invlpg(virt);
    return 0;
}

int vmm_unmap_page(uint64_t virt) {
    virt = align_down(virt);

    uint64_t cr3 = read_cr3() & ~0xfffull;
    uint64_t *pml4 = pt_virt(cr3);

    uint64_t e4 = pml4[idx_pml4(virt)];
    if (!(e4 & PTE_P))
        return -1;
    uint64_t *pdpt = pt_virt(e4 & ~0xfffull);

    uint64_t e3 = pdpt[idx_pdpt(virt)];
    if (!(e3 & PTE_P))
        return -1;
    uint64_t *pd = pt_virt(e3 & ~0xfffull);

    uint64_t e2 = pd[idx_pd(virt)];
    if (!(e2 & PTE_P))
        return -1;
    uint64_t *pt = pt_virt(e2 & ~0xfffull);

    uint16_t i1 = idx_pt(virt);
    pt[i1] = 0;
    invlpg(virt);
    return 0;
}

void vmm_init(void) {
    g_kernel_cr3_phys = read_cr3() & ~0xfffull;
    kprint("[vmm] init ok\n");
}

vmm_space_t vmm_create_space(void) {
    uint64_t new_pml4 = alloc_pt_page_phys();
    if (!new_pml4)
        return (vmm_space_t){0};

    uint64_t *src = pt_virt(g_kernel_cr3_phys);
    uint64_t *dst = pt_virt(new_pml4);
    for (size_t i = 256; i < 512; i++)
        dst[i] = src[i];

    return (vmm_space_t){.pml4_phys = new_pml4};
}

void vmm_switch_space(vmm_space_t space) {
    if (!space.pml4_phys)
        return;
    write_cr3(space.pml4_phys);
}
