#include <limine.h>
#include <stdint.h>

__attribute__((used, section(".limine_requests"))) static volatile uint64_t
    limine_requests_start_marker[4] = LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".limine_requests"))) static volatile uint64_t
    limine_base_revision[3] = LIMINE_BASE_REVISION(2);

__attribute__((used,
               section(".limine_requests"))) volatile struct limine_hhdm_request
    limine_hhdm_request = {
        .id = LIMINE_HHDM_REQUEST_ID, .revision = 0, .response = 0};

__attribute__((
    used, section(".limine_requests"))) volatile struct limine_memmap_request
    limine_memmap_request = {
        .id = LIMINE_MEMMAP_REQUEST_ID,
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

__attribute__((
    used,
    section(".limine_requests"))) volatile struct limine_framebuffer_request
    limine_framebuffer_request = {
        .id = LIMINE_FRAMEBUFFER_REQUEST_ID, .revision = 0, .response = 0};

__attribute__((used,
               section(".limine_requests"))) volatile struct limine_mp_request
    limine_mp_request = {
        .id = LIMINE_MP_REQUEST_ID, .revision = 0, .response = 0, .flags = 0};

__attribute__((
    used,
    section(".limine_requests"))) volatile struct limine_paging_mode_request
    limine_paging_mode_request = {.id = LIMINE_PAGING_MODE_REQUEST_ID,
                                  .revision = 0,
                                  .response = 0,
                                  .mode = LIMINE_PAGING_MODE_X86_64_4LVL};

__attribute__((used, section(".limine_requests"))) static volatile uint64_t
    limine_requests_end_marker[2] = LIMINE_REQUESTS_END_MARKER;

int limine_check_base_revision(void) {
    return LIMINE_BASE_REVISION_SUPPORTED(limine_base_revision);
}
