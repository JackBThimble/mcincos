#include "syscall.h"
#include "core/print.h"
#include "cpu_local.h"
#include "gdt.h"
#include "msr.h"

#define IA32_EFER 0xc0000080u
#define IA32_STAR 0xc0000081u
#define IA32_LSTAR 0xc0000082u
#define IA32_FMASK 0xc0000084u
#define IA32_GS_BASE 0xc0000101u
#define IA32_KERNEL_GS_BASE 0xc0000102u

#define EFER_SCE (1u << 0)

extern void syscall_entry(void);

uint64_t syscall_dispatch(uint64_t num, uint64_t a1, uint64_t a2, uint64_t a3,
                          uint64_t a4, uint64_t a5, uint64_t a6) {
    (void)a3;
    (void)a4;
    (void)a5;
    (void)a6;

    switch (num) {
    case SYS_debug_write: {
        const char *s = (const char *)a1;
        uint64_t len = a2;
        for (uint64_t i = 0; i < len; i++) {
            kprint((char[]){s[i], 0});
        }
        return len;
    }
    case SYS_exit:
        kprint("\n[user] exit()\n");
        for (;;)
            __asm__ volatile("cli; hlt");
    default:
        kprint("\n[syscall] unknown\n");
        return (uint64_t)-1;
    }
}

void syscall_init() {
    uint64_t efer = rdmsr(IA32_EFER);
    wrmsr(IA32_EFER, efer | EFER_SCE);

    /*
     * STAR layout:
     * bits 47:32 = kernel CS
     * bits 63:48 = user CS
     * On SYSCALL:
     * CS = kernel_cs
     * SS = kernel_cs + 8
     * On SYSRET:
     * CS = user_cs + 16
     * SS = user_cs + 8
     * So user_cs should be the selector for user code.
     */

    uint64_t star =
        ((uint64_t)(USER_CS - 16) << 48) | ((uint64_t)KERNEL_CS << 32);
    wrmsr(IA32_STAR, star);

    wrmsr(IA32_LSTAR, (uint64_t)&syscall_entry);

    wrmsr(IA32_FMASK, (1u << 9));

    wrmsr(IA32_GS_BASE, 0);
    wrmsr(IA32_KERNEL_GS_BASE, (uint64_t)&g_cpu_local);

    kprint("[syscall] init ok\n");
}
