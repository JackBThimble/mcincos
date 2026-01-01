.intel_syntax noprefix
.code64

.extern isr_common_handler

.macro ISR_NOERR n
.global isr_\n
isr_\n:
    push 0
    push \n
    jmp isr_common
.endm

.macro ISR_ERR n
.global isr_\n
isr_\n:
    push \n
    jmp isr_common
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

ISR_NOERR 0
ISR_NOERR 1
ISR_ERR   14
