.intel_syntax noprefix
.code64

.extern isr_common_handler
.extern irq_common_handler

.macro ISR_NOERR vec
.global isr_\vec
isr_\vec:
    push 0
    push \vec
    jmp isr_common
.endm

.macro ISR_ERR vec
.global isr_\vec
isr_\vec:
    push \vec
    jmp isr_common
.endm

.macro IRQ vec
.global irq_\vec
irq_\vec:
    push 0
    push \vec
    jmp irq_common
.endm

isr_common:
    push rax
    push rcx
    push rdx
    push rbx
    push rbp
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    mov rdi, rsp
    call isr_common_handler

    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rbp
    pop rbx
    pop rdx
    pop rcx
    pop rax

    add rsp, 16
    iretq

irq_common:
    push rax
    push rcx
    push rdx
    push rbx
    push rbp
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    mov rdi, rsp
    call irq_common_handler

    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi 
    pop rbp
    pop rbx
    pop rdx
    pop rcx
    pop rax

    add rsp, 16
    iretq

ISR_NOERR 0   /* #DE */
ISR_NOERR 1   /* #DB */
ISR_NOERR 2   /* NMI */
ISR_NOERR 3   /* #BP */
ISR_NOERR 4   /* #OF */
ISR_NOERR 5   /* #BR */
ISR_NOERR 6   /* #UD */
ISR_NOERR 7   /* #NM */
ISR_ERR   8   /* #DF */
ISR_NOERR 9   /* (legacy) */
ISR_ERR   10  /* #TS */
ISR_ERR   11  /* #NP */
ISR_ERR   12  /* #SS */
ISR_ERR   13  /* #GP */
ISR_ERR   14  /* #PF */
ISR_NOERR 15
ISR_NOERR 16  /* #MF */
ISR_ERR   17  /* #AC */
ISR_NOERR 18  /* #MC */
ISR_NOERR 19  /* #XM/#XF */
ISR_NOERR 20
ISR_ERR   21  /* #CP (if supported) */
ISR_NOERR 22
ISR_NOERR 23
ISR_NOERR 24
ISR_NOERR 25
ISR_NOERR 26
ISR_NOERR 27
ISR_NOERR 28
ISR_NOERR 29
ISR_NOERR 30
ISR_NOERR 31

IRQ 32
IRQ 33
IRQ 34
IRQ 35
IRQ 36
IRQ 37
IRQ 38
IRQ 39
IRQ 40
IRQ 41
IRQ 42
IRQ 43
IRQ 44
IRQ 45
IRQ 46
IRQ 47

/* Table of stub pointers for idt.c */
.section .rodata
.global isr_stub_table
isr_stub_table:
    .quad isr_0,  isr_1,  isr_2,  isr_3,  isr_4,  isr_5,  isr_6,  isr_7
    .quad isr_8,  isr_9,  isr_10, isr_11, isr_12, isr_13, isr_14, isr_15
    .quad isr_16, isr_17, isr_18, isr_19, isr_20, isr_21, isr_22, isr_23
    .quad isr_24, isr_25, isr_26, isr_27, isr_28, isr_29, isr_30, isr_31

.global irq_stub_table
irq_stub_table:
    .quad irq_32, irq_33, irq_34, irq_35, irq_36, irq_37, irq_38, irq_39
    .quad irq_40, irq_41, irq_42, irq_43, irq_44, irq_45, irq_46, irq_47
