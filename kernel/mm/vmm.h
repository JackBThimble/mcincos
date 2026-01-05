#pragma once
#include <stddef.h>
#include <stdint.h>

#define PAGE_SIZE 4096ULL

#define PTE_P (1ull << 0)
#define PTE_W (1ull << 1)
#define PTE_U (1ull << 2)
#define PTE_PWT (1ull << 3)
#define PTE_PCD (1ull << 4)
#define PTE_A (1ull << 5)
#define PTE_D (1ull << 6)
#define PTE_PS (1ull << 7)
#define PTE_G (1ull << 8)
#define PTE_NX (1ull << 63)

#define VMM_FLAG_READ 0
#define VMM_FLAG_WRITE PTE_W
#define VMM_FLAG_USER PTE_U
#define VMM_FLAG_EXEC 0
#define VMM_FLAG_NOEXEC PTE_NX

typedef struct vmm_space {
        uint64_t pml4_phys;
} vmm_space_t;

void vmm_init(void);

/* map 4kib page in current address space */
int vmm_map_page(uint64_t virt, uint64_t phys, uint64_t flags);
int vmm_unmap_page(uint64_t virt);

int vmm_map_page_space(vmm_space_t space, uint64_t virt, uint64_t phys,
                       uint64_t flags);
int vmm_unmap_page_space(vmm_space_t space, uint64_t virt);

int vmm_map_page_mmio(uint64_t virt, uint64_t phys, uint64_t flags);
/* switch CR3 */
void vmm_switch_space(vmm_space_t space);

/* create a fresh address space (copies kernel mappings) */
vmm_space_t vmm_create_space(void);
