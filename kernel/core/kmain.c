#include "print.h"

void kmain(void) {
    kprint("[kernel] booted with clang + intel asm\n");

    for (;;) {
        __asm__ volatile ("hlt");
    }
}
