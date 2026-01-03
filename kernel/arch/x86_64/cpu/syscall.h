#pragma once
#include <stdint.h>

void syscall_init(void);

enum {
    SYS_debug_write = 0,
    SYS_exit = 1,
};
