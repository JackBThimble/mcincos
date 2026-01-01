.intel_syntax noprefix
.code64

.global _start
.extern kmain

.section .text
_start:
    cli
    mov rbp, rsp

    call kmain

.hang:
    hlt
    jmp .hang
