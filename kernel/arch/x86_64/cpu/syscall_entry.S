.intel_syntax noprefix
.code64

.global syscall_entry
.extern syscall_dispatch

.set CPU_LOCAL_KERNEL_RSP, 0
.set CPU_LOCAL_USER_RSP, 8

.section .text
syscall_entry:
    swapgs

    /* save user rsp */
    mov gs:[CPU_LOCAL_USER_RSP], rsp

    /* switch to kernel stack */
    mov rsp, gs:[CPU_LOCAL_KERNEL_RSP]

    /* preserve user rip + rflags for sysretq */
    push r11
    push rcx

    /* save clobbered regs (we'll keep it minimal) */
    push rbx
    push rbp
    push r12
    push r13
    push r14
    push r15

    /*
    snapshot syscall args before we clobber for SysV:
        rax = num
        rdi, rsi, rdx, r10, r8, r9 = a1..a6
        stash a6 in r12 (callee saved, already saved).
    */
    mov r12, r9

    /*
    arrange SysV call:
        rdi=num
        rsi=a1
        rdx=a2
        rcx=a3
        r8 =a4
        r9 =a5
        stack = a6
    */

    mov r9, r8      /* a5 <- old a4? careful: old a5 is in 48, old a4 in r10 */
    mov r8, r10     /* a4 */
    mov rcx, rdx    /* a3 */
    mov rdx, rsi    /* a2 */
    mov rsi, rdi    /* a1 */
    mov rdi, rax    /* num */
    sub rsp, 8      /* align stack for SysV call */
    push r12        /* a6 as 7th arg on stack */
    call syscall_dispatch
    add rsp, 16      /* pop a6 */

    /* restore non-volatiles */
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbp
    pop rbx

    /* restore user rip + eflags */
    pop rcx
    pop r11

    /* restore user rsp */
    mov rsp, gs:[CPU_LOCAL_USER_RSP]

    swapgs
    sysretq

    mov rdi, rax

