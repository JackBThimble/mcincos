#pragma once
#include <limine.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
        uint64_t hhdm_offset;

        struct limine_bootloader_info_response bootloader_info;

        struct limine_executable_cmdline_response cmdline;

        struct limine_firmware_type_response firmware_type;

        struct limine_stack_size_response stack_size;

        struct limine_paging_mode_response paging_mode;

        struct limine_entry_point_response entry_point;

        struct limine_executable_file_response kernel;

        struct limine_module_response module;

        struct limine_smbios_response smbios;

        struct limine_efi_system_table_response efi_system_table;

        struct limine_efi_memmap_response efi_memmap;

        struct limine_date_at_boot_response boot_date;

        struct limine_dtb_response device_tree_blob;

        struct limine_riscv_bsp_hartid_response riscv_bsp_hartid;

        struct limine_bootloader_performance_response performance;

        struct limine_executable_address_response kernel_address;

        struct limine_memmap_response memmap;

        struct limine_rsdp_response acpi;

        struct limine_mp_response smp;

        struct limine_framebuffer_response framebuffer;
} boot_info_t;

extern boot_info_t g_boot_info;

void boot_info_init(void);
void print_boot_info(void);
