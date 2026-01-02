#include "panic.h"
#include "print.h"

__attribute__((noreturn)) void panic(const char *msg) {
    kprint("\n[PANIC] ");
    kprint(msg);
    kprint("\n");
    for (;;)
        __asm__ volatile("cli; hlt");
}
