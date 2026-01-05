#include <limine.h>
#include <stdint.h>

extern void _start(void);

__attribute__((used, section(".limine_requests"))) static volatile uint64_t
    limine_requests_start_marker[4] = LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".limine_requests"))) static volatile uint64_t
    limine_base_revision[] = LIMINE_BASE_REVISION(4);

__attribute__((
    used,
    section(".limine_requests"))) volatile struct limine_bootloader_info_request
    limine_bootloader_info_request = {
        .id = LIMINE_BOOTLOADER_INFO_REQUEST_ID,
        .revision = 0,
        .response = 0,
};

__attribute__((
    used,
    section(
        ".limine_requests"))) volatile struct limine_executable_cmdline_request
    limine_executable_cmdline_request = {
        .id = LIMINE_EXECUTABLE_CMDLINE_REQUEST_ID,
        .revision = 0,
        .response = 0,
};

__attribute__((
    used,
    section(".limine_requests"))) volatile struct limine_firmware_type_request
    limine_firmware_type_request = {
        .id = LIMINE_FIRMWARE_TYPE_REQUEST_ID,
        .revision = 0,
        .response = 0,
};

__attribute__((
    used,
    section(".limine_requests"))) volatile struct limine_stack_size_request
    limine_stack_size_request = {
        .id = LIMINE_STACK_SIZE_REQUEST_ID,
        .revision = 0,
        .response = 0,
};

__attribute__((used,
               section(".limine_requests"))) volatile struct limine_hhdm_request
    limine_hhdm_request = {
        .id = LIMINE_HHDM_REQUEST_ID, .revision = 0, .response = 0};

__attribute__((
    used,
    section(".limine_requests"))) volatile struct limine_framebuffer_request
    limine_framebuffer_request = {
        .id = LIMINE_FRAMEBUFFER_REQUEST_ID, .revision = 0, .response = 0};

__attribute__((
    used,
    section(".limine_requests"))) volatile struct limine_paging_mode_request
    limine_paging_mode_request = {.id = LIMINE_PAGING_MODE_REQUEST_ID,
                                  .response = 0,
                                  .revision = 0,
                                  .max_mode = 1,
                                  .min_mode = 0,
                                  .mode = 0};

__attribute__((used,
               section(".limine_requests"))) volatile struct limine_mp_request
    limine_mp_request = {
        .id = LIMINE_MP_REQUEST_ID, .revision = 0, .response = 0, .flags = 0};

__attribute__((
    used, section(".limine_requests"))) volatile struct limine_memmap_request
    limine_memmap_request = {
        .id = LIMINE_MEMMAP_REQUEST_ID,
        .revision = 0,
        .response = 0,
};

__attribute__((
    used,
    section(".limine_requests"))) volatile struct limine_entry_point_request
    limine_entry_point_request = {
        .id = LIMINE_ENTRY_POINT_REQUEST_ID,
        .revision = 0,
        .response = 0,
        .entry = _start,
};

__attribute__((
    used,
    section(".limine_requests"))) volatile struct limine_executable_file_request
    limine_executable_file_request = {
        .id = LIMINE_EXECUTABLE_FILE_REQUEST_ID,
        .revision = 0,
        .response = 0,
};

__attribute__((
    used, section(".limine_requests"))) volatile struct limine_module_request
    limine_module_request = {
        .id = LIMINE_MODULE_REQUEST_ID,
        .response = 0,
        .revision = 0,
        .internal_module_count = 0,
        .internal_modules = 0,
};

__attribute__((used,
               section(".limine_requests"))) volatile struct limine_rsdp_request
    limine_rsdp_request = {
        .id = LIMINE_RSDP_REQUEST_ID, .revision = 0, .response = 0};

__attribute__((
    used, section(".limine_requests"))) volatile struct limine_smbios_request
    limine_smbios_request = {
        .id = LIMINE_SMBIOS_REQUEST_ID,
        .revision = 0,
        .response = 0,
};

__attribute__((
    used,
    section(
        ".limine_requests"))) volatile struct limine_efi_system_table_request
    limine_efi_system_table_request = {
        .id = LIMINE_EFI_SYSTEM_TABLE_REQUEST_ID,
        .revision = 0,
        .response = 0,
};

__attribute__((
    used,
    section(".limine_requests"))) volatile struct limine_efi_memmap_request
    limine_efi_memmap_request = {
        .id = LIMINE_EFI_MEMMAP_REQUEST_ID,
        .revision = 0,
        .response = 0,
};

__attribute__((
    used,
    section(".limine_requests"))) volatile struct limine_date_at_boot_request
    limine_date_at_boot_request = {
        .id = LIMINE_DATE_AT_BOOT_REQUEST_ID,
        .revision = 0,
        .response = 0,
};

__attribute__((
    used,
    section(
        ".limine_requests"))) volatile struct limine_executable_address_request
    limine_executable_address_request = {
        .id = LIMINE_EXECUTABLE_ADDRESS_REQUEST_ID,
        .revision = 0,
        .response = 0};

__attribute__((used,
               section(".limine_requests"))) volatile struct limine_dtb_request
    limine_dtb_request = {
        .id = LIMINE_DTB_REQUEST_ID,
        .revision = 0,
        .response = 0,
};

__attribute__((
    used,
    section(
        ".limine_requests"))) volatile struct limine_riscv_bsp_hartid_request
    limine_riscv_bsp_hartid_request = {
        .id = LIMINE_RISCV_BSP_HARTID_REQUEST_ID,
        .revision = 0,
        .response = 0,
};

__attribute__((used, section(".limine_requests"))) volatile struct
    limine_bootloader_performance_request
        limine_bootloader_performance_request = {
            .id = LIMINE_BOOTLOADER_PERFORMANCE_REQUEST_ID,
            .revision = 0,
            .response = 0,
};

__attribute__((used, section(".limine_requests"))) static volatile uint64_t
    limine_requests_end_marker[2] = LIMINE_REQUESTS_END_MARKER;

int limine_check_base_revision(void) {
    return LIMINE_BASE_REVISION_SUPPORTED(limine_base_revision);
}
