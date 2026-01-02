#pragma once
#include <stdint.h>

typedef struct isr_frame {
        uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
        uint64_t rdi, rsi, rbp, rbx, rdx, rcx, rax;

        uint64_t vector;
        uint64_t error;

        uint64_t rip;
        uint64_t cs;
        uint64_t rflags;
        uint64_t rsp;
        uint64_t ss;
} isr_frame_t;
