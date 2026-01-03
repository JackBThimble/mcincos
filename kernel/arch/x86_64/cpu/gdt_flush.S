#include "gdt.h"
.intel_syntax noprefix
.global gdt_flush

gdt_flush:
    lgdt [rdi]

    /* Reload data segments */
    mov ax, KERNEL_DS /* kernel data */
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov fs, ax
    mov gs, ax

    /* Far return to reload CS safely */
    push KERNEL_CS  /* kernel code selector */
    lea rax, [rip + .reload]
    push rax
    retfq           

.reload:
    mov ax, TSS_SEL
    ltr ax
    ret
