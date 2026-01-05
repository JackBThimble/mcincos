#include <stdint.h>

#include "../arch/x86_64/cpu/cpu_local.h"
#include "../arch/x86_64/cpu/gdt.h"
#include "../arch/x86_64/cpu/idt.h"
#include "../arch/x86_64/cpu/irq.h"
#include "../arch/x86_64/cpu/lapic.h"
#include "../arch/x86_64/cpu/syscall.h"
#include "../arch/x86_64/cpu/timer.h"

#include "../mm/pmm.h"
#include "../mm/vmm.h"

#include "../boot/boot_info.h"

#include "core/panic.h"
#include "lapic.h"
#include "print.h"
#include "sched.h"

#define TIMER_VECTOR 0xf0
#define SCHED_QUANTUM_NS 5000000ull

static void early_banner(void) {
    kprintln(
        "================================================================");
    kprintln(
        "========================= mCincOS ==============================");
    kprintln(
        "================================================================");
}

static void build_user_blob(uint8_t *dst) {
    const char msg[] = "hello from user\n";
    const uint32_t len = (uint32_t)(sizeof(msg) - 1);

    uint8_t code[] = {
        0xb8, 0x00, 0x00, 0x00, 0x00,       /* mov eax, 0 */
        0x48, 0x8d, 0x3d, 0,    0,    0, 0, /* lea rdi, [rip+disp32] */
        0xbe, 0,    0,    0,    0,          /* mov esi, len */
        0x0f, 0x05,                         /* syscall */
        0xeb, 0xfe                          /* jmp $ */
    };

    code[1] = 0;

    *(uint32_t *)&code[13] = len;

    uint32_t msg_off = (uint32_t)sizeof(code);
    int32_t disp = (int32_t)msg_off - (int32_t)12;
    *(int32_t *)&code[8] = disp;

    for (size_t i = 0; i < sizeof(code); i++)
        dst[i] = code[i];
    for (size_t i = 0; i < len; i++)
        dst[sizeof(code) + i] = (uint8_t)msg[i];
}

__attribute__((noreturn)) static void enter_user(uint64_t user_rip,
                                                 uint64_t user_rsp) {
    __asm__ volatile(".intel_syntax noprefix\n"
                     "cli\n"
                     "mov ax, %c2\n"
                     "mov ds, ax\n"
                     "mov es, ax\n"
                     "mov fs, ax\n"
                     "mov gs, ax\n"

                     "push %c2\n"
                     "push rsi\n"
                     "pushfq\n"
                     "or qword ptr [rsp], 0x200\n"
                     "push %c3\n"
                     "push rdi\n"
                     "iretq\n"
                     ".att_syntax prefix\n"
                     :
                     : "D"(user_rip), "S"(user_rsp), "i"(USER_DS), "i"(USER_CS)
                     : "memory", "rax");

    for (;;)
        __asm__ volatile("hlt");
}

static thread_t t0;
static thread_t t1;

void kmain(void) {
    // =========================================================================
    // 0) EARLY BOOT: assume Limine got us to long mode/paging
    // =========================================================================
    early_banner();
    boot_info_init();
    print_boot_info();

    // TODO: route early logs to BOTH serial + limine terminal (if present)
    // TODO: add log levels + ring buffer for post-mortem panic dumps

    // =========================================================================
    // 1) CPU TABLES: own the platform (do NOT rely on Limine)
    // =========================================================================

    kprintln("[init] gdt");
    gdt_init(); // installs known selectors for kernel + user

    // =========================================================================
    // 2) BOOT INFO: grab Limine-provided data while it's valid
    // =========================================================================
    // NOTE: keep Limine response pointers around, but treat them read-only
    // TODO: parse and store:
    //      - kernel phys/virt range
    //      - HHDM offset
    //      - memmap snapshot (copy it, don't depend on bootloader memory long
    //      term)
    //      - framebuffer(s), ACPI RSDP, modules, SMP info

    // =========================================================================
    // 3) MEMORY: physical allocator first, then virtual
    // =========================================================================
    kprintln("[init] pmm");
    pmm_init(); // uses Limine memmap + HHDM; excludes kernel/bitmap/etc

    kprintln("[init] vmm");
    vmm_init();

    // TODO: build and switch to kernel-owned page tables (own CR3)
    //      - keep HHDM mapping
    //      - map kernel higher-half RX/RO/RW properly
    //      - enforce W^X + NX where possible
    //      - add guard pages (catch stack/heap overruns)

    // =========================================================================
    // 4) PER-CPU / STACKS: needed for syscalls + interrupts later
    // =========================================================================
    kprintln("[init] cpu_local + kernel stack");
    {
        // TODO: set up TSS + IST stacks and load TR (needed for robust
        // faults)
        // TODO: when SMP: allocate cpu_local per core + set
        // MSR_KERNEL_GS_BASE per core

        kprintln("df_stack");
        void *df_stack = pmm_alloc_pages(2);
        gdt_set_ist(1, (uint64_t)phys_to_virt((uint64_t)df_stack + 2 * 4096));

        kprintln("alloc pf stack");
        void *pf_stack = pmm_alloc_pages(2);
        gdt_set_ist(2, (uint64_t)phys_to_virt((uint64_t)pf_stack + 2 * 4096));

        kprintln("alloc nmi stack");
        void *nmi_stack = pmm_alloc_pages(2);
        gdt_set_ist(3, (uint64_t)phys_to_virt((uint64_t)nmi_stack + 2 * 4096));
        // TODO: allocate per-cpu kernel stacks and store in cpu_local via GS
        // base
        // For now: single-core bootstrap

        void *kstack0 = pmm_alloc_pages(2);
        void *kstack1 = pmm_alloc_pages(2);

        uint64_t kstack0_top =
            (uint64_t)phys_to_virt((uint64_t)kstack0 + 2 * 4096);
        uint64_t kstack1_top =
            (uint64_t)phys_to_virt((uint64_t)kstack1 + 2 * 4096);

        // map two user stacks and two code pages
        uint64_t user_code0 = 0x0000000000400000ull;
        uint64_t user_stack0 = 0x0000000000700000ull;
        uint64_t user_code1 = 0x0000000000500000ull;
        uint64_t user_stack1 = 0x0000000000800000ull;

        void *uc0_phys = pmm_alloc_pages(1);
        void *us0_phys = pmm_alloc_pages(1);
        void *uc1_phys = pmm_alloc_pages(1);
        void *us1_phys = pmm_alloc_pages(1);

        if (!uc0_phys || !us0_phys || !uc1_phys || !us1_phys)
            panic("user pages allocation failed");
        vmm_map_page(user_code0, (uint64_t)uc0_phys, PTE_U | PTE_W);
        vmm_map_page(user_stack0, (uint64_t)uc0_phys, PTE_U | PTE_W);
        vmm_map_page(user_code1, (uint64_t)uc0_phys, PTE_U | PTE_W);
        vmm_map_page(user_code0, (uint64_t)uc0_phys, PTE_U | PTE_W);
        // allocate/map pages
        kprintln("build blob 1");
        build_user_blob((uint8_t *)user_code0);
        kprintln("build blob 2");
        build_user_blob((uint8_t *)user_code1);

        kprintln("thread init 1");
        thread_init_user(&t0, user_code0, user_stack0 + 4096, kstack0_top);
        kprintln("thread 2");
        thread_init_user(&t1, user_code1, user_stack1 + 4096, kstack1_top);

        g_cpu_local.kernel_rsp = kstack0_top;
        gdt_set_kernel_stack(kstack0_top);

        kprintln("[init] sched");
        sched_init(&t0);

        sched_add(&t1);

        kprintln("[init] idt");
        idt_init();
        irq_init();

        kprintln("[init] timer");
        timer_init(TIMER_VECTOR);

        timer_source_t src = timer_source();
        if (src == TIMER_SRC_TSC_DEADLINE)
            kprintln("[timer] tsc-deadline");
        else if (src == TIMER_SRC_LAPIC)
            kprintln("[timer] lapic oneshot");
        else
            panic("timer init failed");

        irq_register_vector_handler(timer_vector(), sched_on_tick);
        sched_set_quantum_ns(SCHED_QUANTUM_NS);
        sched_start();

        idt_enable();
        kprintln("[init] syscall");
        syscall_init();

        enter_user(user_code0, user_stack0 + 4096);
    }

    // =========================================================================
    // 5) SYSCALL ABI: establish user/kernel boundary early
    // =========================================================================

    // TODO: define syscall ABI doc (registers, error returns, struct packing)
    // TODO: implement copyin/copyout (validate user pointers, avoid kernel
    // faults)
    // TODO: add per-kernel stacks + swapgs discipline
    // TODO: add basic handle/capability object model for microkernel IPC

    // =========================================================================
    // 6) TIMERS + IRQ ROUTING (optional before usermode, but soon)
    // =========================================================================
    // TODO: init APIC (xAPIC/x2APIC) + IOAPIC
    // TODO: PIT/HPET/TSC calibration
    // TODO: periodic timer interrupt (schedular tick) OR tickless design
    // TODO: IRQ masking, spurious, EOI, interrupt priorities

    // =========================================================================
    // 7) SCHEDULER + THREADS: make the kernel a runtime
    // =========================================================================
    // TODO: kernel thread primitive (kthread)
    // TODO: context switch (save/restore regs, FPU state later)
    // TODO: run queue + priorities (desktop: latency aware)
    // TODO: preemption boundaries (kernel critical sections)

    // =========================================================================
    // 8) USERMODE BRING-UP: create init process
    // =========================================================================
    // TODO: create address space object (new PMML4 copying kernel half)
    // TODO: map user stack + program image (ELF loader later)
    // TODO: enter ring3 (iretq/sysretq path)
    // TODO: init process calls into "process manager" service via IPC

    // =========================================================================
    // 9) MICROKERNEL SERVICES: move policy out of the kernel
    // =========================================================================
    // TODO: service model:
    //      - memory manager (userland) on top of kernel primitives
    //      - process manager
    //      - device manager / driver host(s)
    //      - file system service (VFS in userland or hybrid)
    //      - compositor/window server (fast path compromises allowed)
    //
    // TODO: IPC:
    //      - synchronous call/reply + async notifications
    //      - shared memory objects
    //      - capabilities and rights (send/dup/restrict)
    //      - zero-copy fast path for GUI/IO

    // =========================================================================
    // 10) DRIVER STRATEGY (phased)
    // =========================================================================
    // TODO: early console: serial + framebuffer text
    // TODO: PCIe enumeration
    // TODO: NVMe + AHCI (storage)
    // TODO: USB (later)
    // TODO: NIC (Intel/Realtek first)
    //
    // Desktop:
    // TODO: GPU (simple KMS first), then accelerated compositor
    // Embedded/RTOS:
    // TODO: deterministic timers + priority inheritance + bounded IPC

    // =========================================================================
    // 11) SECURITY HARDENING (build it in from day 1)
    // =========================================================================
    // TODO: W^X everywhere (no RWX)
    // TODO: NX for data, SMEP/SMAP/UMIP if available
    // TODO: KASLR (randomize kernel base)
    // TODO: kernel stack canaries (or shadow stacks later)
    // TOOD: CFI/pointer auth style mitigations where feasible
    // TODO: fuzzable syscall surface + strict validation

    // =========================================================================
    // 12) FILESYSTEM + APP PLATFORM
    // =========================================================================
    // TODO: ELF loader in userland
    // TODO: dynamic linker strategy (ld.so style or static first)
    // TODO: POSIX compatibility layer as a userland subsystem (optional)
    // TODO: language runtime
    //      - Zig/Rust/Odin: native toolchain target
    //      - Python: userland runtime + sandboxed permissions

    // =========================================================================
    // 13) GUI PATH (desktop "slick as fuck")
    // =========================================================================
    // TODO: compositor service (Wayland-like protocol or custom)
    // TODO: graphics memory sharing fast path (shmem + GPU buffers)
    // TODO: input stack (HID, touch, keymaps)
    // TODO: window manager API + theming
    // TODO: support alternate WMs/DEs as first-class citizens

    // =========================================================================
    // TEMP: For now, stop here.
    // =========================================================================
    uint64_t user_code_va = 0x0000000000400000ull;
    uint64_t user_stack_va = 0x0000000000700000ull;

    void *uc_phys = pmm_alloc_pages(1);
    void *us_phys = pmm_alloc_pages(1);

    vmm_map_page(user_code_va, (uint64_t)uc_phys, PTE_U | PTE_W);
    vmm_map_page(user_stack_va, (uint64_t)us_phys, PTE_U | PTE_W);

    build_user_blob((uint8_t *)user_code_va);

    enter_user(user_code_va, user_stack_va + 4096);

    kprint("[vmm] map+touch ok\n");

    kprintln("[init] kernel idle");
    for (;;) {
        __asm__ volatile("hlt");
    }
}
