#include <limine.h>

__attribute__((used, section(".limine_requests")))
              static volatile LIMINE_BASE_REVISION(2);

__attribute__((used, sectoin(".limine_requests")))
static volatile struct limine_terminal_request term_req = {
    .id = LIMINE_TERMINAL_REQUEST,
    .revision = 0
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_framebuffer_request fb_req = {
    .id = LIMINE_FRAMEBUFFER_REQUEST,
    .revision = 0
};
