#include "boot_info.h"
#include "core/print.h"
#include <limine.h>

boot_info_t g_boot_info;

extern volatile struct limine_hhdm_request limine_hhdm_request;
extern volatile struct limine_executable_address_request
    limine_executable_address_request;
extern volatile struct limine_memmap_request limine_memmap_request;
extern volatile struct limine_rsdp_request limine_rsdp_request;
extern volatile struct limine_mp_request limine_mp_request;
extern volatile struct limine_framebuffer_request limine_framebuffer_request;
extern volatile struct limine_executable_file_request
    limine_executable_file_request;
extern volatile struct limine_bootloader_performance_request
    limine_bootloader_performance_request;
extern volatile struct limine_executable_cmdline_request
    limine_executable_cmdline_request;
extern volatile struct limine_bootloader_info_request
    limine_bootloader_info_request;
extern volatile struct limine_date_at_boot_request limine_date_at_boot_request;
extern volatile struct limine_executable_cmdline_request
    limine_executable_cmdline_request;
extern volatile struct limine_dtb_request limine_dtb_request;
extern volatile struct limine_efi_memmap_request limine_efi_memmap_request;
extern volatile struct limine_efi_system_table_request
    limine_efi_system_table_request;
extern volatile struct limine_entry_point_request limine_entry_point_request;
extern volatile struct limine_firmware_type_request
    limine_firmware_type_request;
extern volatile struct limine_module_request limine_module_request;
extern volatile struct limine_paging_mode_request limine_paging_mode_request;
extern volatile struct limine_riscv_bsp_hartid_request
    limine_riscv_bsp_hartid_request;
extern volatile struct limine_smbios_request limine_smbios_request;
extern volatile struct limine_stack_size_request limine_stack_size_request;

static const char *firmware_type_name(uint64_t type) {
    switch (type) {
    case LIMINE_FIRMWARE_TYPE_X86BIOS:
        return "x86bios";
    case LIMINE_FIRMWARE_TYPE_EFI32:
        return "efi32";
    case LIMINE_FIRMWARE_TYPE_EFI64:
        return "efi64";
    case LIMINE_FIRMWARE_TYPE_SBI:
        return "sbi";
    default:
        return "unknown";
    }
}

static const char *paging_mode_name(uint64_t mode) {
#if defined(__x86_64__) || defined(__i386__)
    switch (mode) {
    case LIMINE_PAGING_MODE_X86_64_4LVL:
        return "x86_64 4lvl";
    case LIMINE_PAGING_MODE_X86_64_5LVL:
        return "x86_64 5lvl";
    default:
        return "x86_64 unknown";
    }
#elif defined(__aarch64__)
    switch (mode) {
    case LIMINE_PAGING_MODE_AARCH64_4LVL:
        return "aarch64 4lvl";
    case LIMINE_PAGING_MODE_AARCH64_5LVL:
        return "aarch64 5lvl";
    default:
        return "aarch64 unknown";
    }
#elif defined(__riscv) && (__riscv_xlen == 64)
    switch (mode) {
    case LIMINE_PAGING_MODE_RISCV_SV39:
        return "riscv sv39";
    case LIMINE_PAGING_MODE_RISCV_SV48:
        return "riscv sv48";
    case LIMINE_PAGING_MODE_RISCV_SV57:
        return "riscv sv57";
    default:
        return "riscv unknown";
    }
#elif defined(__loongarch__) && (__loongarch_grlen == 64)
    switch (mode) {
    case LIMINE_PAGING_MODE_LOONGARCH_4LVL:
        return "loongarch 4lvl";
    default:
        return "loongarch unknown";
    }
#else
    (void)mode;
    return "unknown";
#endif
}

static const char *memmap_type_name(uint64_t type) {
    switch (type) {
    case LIMINE_MEMMAP_USABLE:
        return "usable";
    case LIMINE_MEMMAP_RESERVED:
        return "reserved";
    case LIMINE_MEMMAP_ACPI_RECLAIMABLE:
        return "acpi_reclaimable";
    case LIMINE_MEMMAP_ACPI_NVS:
        return "acpi_nvs";
    case LIMINE_MEMMAP_BAD_MEMORY:
        return "bad_memory";
    case LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE:
        return "bootloader_reclaimable";
    case LIMINE_MEMMAP_EXECUTABLE_AND_MODULES:
        return "executable_and_modules";
    case LIMINE_MEMMAP_FRAMEBUFFER:
        return "framebuffer";
    case LIMINE_MEMMAP_ACPI_TABLES:
        return "acpi_tables";
    default:
        return "unknown";
    }
}

static const char *media_type_name(uint32_t type) {
    switch (type) {
    case LIMINE_MEDIA_TYPE_GENERIC:
        return "generic";
    case LIMINE_MEDIA_TYPE_OPTICAL:
        return "optical";
    case LIMINE_MEDIA_TYPE_TFTP:
        return "tftp";
    default:
        return "unknown";
    }
}

static const char *framebuffer_memory_model_name(uint8_t model) {
    switch (model) {
    case LIMINE_FRAMEBUFFER_RGB:
        return "rgb";
    default:
        return "unknown";
    }
}

static void print_hex_fixed(uint64_t value, int digits) {
    static const char hex[] = "0123456789abcdef";

    for (int i = digits - 1; i >= 0; i--) {
        uint8_t nib = (uint8_t)((value >> (i * 4)) & 0xf);
        kprintf("%c", hex[nib]);
    }
}

static void print_uuid(const struct limine_uuid *uuid) {
    if (!uuid) {
        kprintf("(null)");
        return;
    }

    print_hex_fixed(uuid->a, 8);
    kprintf("-");
    print_hex_fixed(uuid->b, 4);
    kprintf("-");
    print_hex_fixed(uuid->c, 4);
    kprintf("-");
    print_hex_fixed(uuid->d[0], 2);
    print_hex_fixed(uuid->d[1], 2);
    kprintf("-");
    for (size_t i = 2; i < 8; i++)
        print_hex_fixed(uuid->d[i], 2);
}

static void print_signed_u64_line(const char *label, int64_t value) {
    uint64_t abs_value;

    if (value < 0) {
        abs_value = (uint64_t)(-(value + 1)) + 1;
        kprintf("  %s: -%llu\n", label, (unsigned long long)abs_value);
    } else {
        kprintf("  %s: %llu\n", label, (unsigned long long)value);
    }
}

static void print_limine_file(const char *label,
                              const struct limine_file *file) {
    if (!file) {
        kprintlnf("  %s: (null)", label);
        return;
    }

    kprintlnf("  %s:", label);
    kprintlnf("    revision: %llu", (unsigned long long)file->revision);
    kprintlnf("    address: 0x%llx",
              (unsigned long long)(uint64_t)(uintptr_t)file->address);
    kprintlnf("    size: %llu", (unsigned long long)file->size);
    kprintlnf("    path: %s", file->path);
    kprintlnf("    string: %s", file->string);
    kprintlnf("    media_type: %u (%s)", (unsigned)file->media_type,
              media_type_name(file->media_type));
    kprintlnf("    unused: 0x%x", (unsigned)file->unused);
    kprintlnf("    tftp_ip: 0x%x", (unsigned)file->tftp_ip);
    kprintlnf("    tftp_port: %u", (unsigned)file->tftp_port);
    kprintlnf("    partition_index: %u", (unsigned)file->partition_index);
    kprintlnf("    mbr_disk_id: 0x%x", (unsigned)file->mbr_disk_id);
    kprintf("    gpt_disk_uuid: ");
    print_uuid(&file->gpt_disk_uuid);
    kprintf("\n");
    kprintf("    gpt_part_uuid: ");
    print_uuid(&file->gpt_part_uuid);
    kprintf("\n");
    kprintf("    part_uuid: ");
    print_uuid(&file->part_uuid);
    kprintf("\n");
}

static void print_limine_file_indexed(const char *label, uint64_t index,
                                      const struct limine_file *file) {
    if (!file) {
        kprintlnf("  %s[%llu]: (null)", label, (unsigned long long)index);
        return;
    }

    kprintlnf("  %s[%llu]:", label, (unsigned long long)index);
    kprintlnf("    revision: %llu", (unsigned long long)file->revision);
    kprintlnf("    address: 0x%llx",
              (unsigned long long)(uint64_t)(uintptr_t)file->address);
    kprintlnf("    size: %llu", (unsigned long long)file->size);
    kprintlnf("    path: %s", file->path);
    kprintlnf("    string: %s", file->string);
    kprintlnf("    media_type: %u (%s)", (unsigned)file->media_type,
              media_type_name(file->media_type));
    kprintlnf("    unused: 0x%x", (unsigned)file->unused);
    kprintlnf("    tftp_ip: 0x%x", (unsigned)file->tftp_ip);
    kprintlnf("    tftp_port: %u", (unsigned)file->tftp_port);
    kprintlnf("    partition_index: %u", (unsigned)file->partition_index);
    kprintlnf("    mbr_disk_id: 0x%x", (unsigned)file->mbr_disk_id);
    kprintf("    gpt_disk_uuid: ");
    print_uuid(&file->gpt_disk_uuid);
    kprintf("\n");
    kprintf("    gpt_part_uuid: ");
    print_uuid(&file->gpt_part_uuid);
    kprintf("\n");
    kprintf("    part_uuid: ");
    print_uuid(&file->part_uuid);
    kprintf("\n");
}

void boot_info_init(void) {
    for (size_t i = 0; i < sizeof(g_boot_info); i++)
        ((uint8_t *)&g_boot_info)[i] = 0;

    if (limine_executable_address_request.response)
        g_boot_info.kernel_address = *limine_executable_address_request.response;
    if (limine_memmap_request.response)
        g_boot_info.memmap = *limine_memmap_request.response;
    if (limine_executable_file_request.response)
        g_boot_info.kernel = *limine_executable_file_request.response;
    if (limine_bootloader_info_request.response)
        g_boot_info.bootloader_info = *limine_bootloader_info_request.response;
    if (limine_date_at_boot_request.response)
        g_boot_info.boot_date = *limine_date_at_boot_request.response;
    if (limine_rsdp_request.response)
        g_boot_info.acpi = *limine_rsdp_request.response;
    if (limine_executable_cmdline_request.response)
        g_boot_info.cmdline = *limine_executable_cmdline_request.response;
    if (limine_dtb_request.response)
        g_boot_info.device_tree_blob = *limine_dtb_request.response;
    if (limine_efi_memmap_request.response)
        g_boot_info.efi_memmap = *limine_efi_memmap_request.response;
    if (limine_efi_system_table_request.response)
        g_boot_info.efi_system_table = *limine_efi_system_table_request.response;
    if (limine_entry_point_request.response)
        g_boot_info.entry_point = *limine_entry_point_request.response;
    if (limine_firmware_type_request.response)
        g_boot_info.firmware_type = *limine_firmware_type_request.response;
    if (limine_framebuffer_request.response)
        g_boot_info.framebuffer = *limine_framebuffer_request.response;
    if (limine_hhdm_request.response)
        g_boot_info.hhdm_offset = limine_hhdm_request.response->offset;
    if (limine_module_request.response)
        g_boot_info.module = *limine_module_request.response;
    if (limine_paging_mode_request.response)
        g_boot_info.paging_mode = *limine_paging_mode_request.response;
    if (limine_bootloader_performance_request.response)
        g_boot_info.performance =
            *limine_bootloader_performance_request.response;
    if (limine_riscv_bsp_hartid_request.response) {
        g_boot_info.riscv_bsp_hartid =
            *limine_riscv_bsp_hartid_request.response;
    }
    if (limine_smbios_request.response)
        g_boot_info.smbios = *limine_smbios_request.response;
    if (limine_mp_request.response)
        g_boot_info.smp = *limine_mp_request.response;
    if (limine_stack_size_request.response)
        g_boot_info.stack_size = *limine_stack_size_request.response;
}

void print_boot_info(void) {
    kprintln("==========================================================");
    kprintln("=====================BOOT INFO============================");
    kprintln("==========================================================");

    kprintln("HHDM:");
    if (!limine_hhdm_request.response) {
        kprintln("  not provided");
    } else {
        kprintlnf("  revision: %llu",
                  (unsigned long long)limine_hhdm_request.response->revision);
        kprintlnf("  offset: 0x%llx",
                  (unsigned long long)g_boot_info.hhdm_offset);
    }

    kprintln("Bootloader info:");
    if (!limine_bootloader_info_request.response) {
        kprintln("  not provided");
    } else {
        kprintlnf("  revision: %llu",
                  (unsigned long long)g_boot_info.bootloader_info.revision);
        kprintlnf("  name: %s", g_boot_info.bootloader_info.name);
        kprintlnf("  version: %s", g_boot_info.bootloader_info.version);
    }

    kprintln("Executable cmdline:");
    if (!limine_executable_cmdline_request.response) {
        kprintln("  not provided");
    } else {
        kprintlnf("  revision: %llu",
                  (unsigned long long)g_boot_info.cmdline.revision);
        kprintlnf("  cmdline: %s", g_boot_info.cmdline.cmdline);
    }

    kprintln("Firmware type:");
    if (!limine_firmware_type_request.response) {
        kprintln("  not provided");
    } else {
        kprintlnf("  revision: %llu",
                  (unsigned long long)g_boot_info.firmware_type.revision);
        kprintlnf("  firmware_type: %llu (%s)",
                  (unsigned long long)g_boot_info.firmware_type.firmware_type,
                  firmware_type_name(g_boot_info.firmware_type.firmware_type));
    }

    kprintln("Stack size:");
    if (!limine_stack_size_request.response) {
        kprintln("  not provided");
    } else {
        kprintlnf("  revision: %llu",
                  (unsigned long long)g_boot_info.stack_size.revision);
    }

    kprintln("Paging mode:");
    if (!limine_paging_mode_request.response) {
        kprintln("  not provided");
    } else {
        kprintlnf("  revision: %llu",
                  (unsigned long long)g_boot_info.paging_mode.revision);
        kprintlnf("  mode: %llu (%s)",
                  (unsigned long long)g_boot_info.paging_mode.mode,
                  paging_mode_name(g_boot_info.paging_mode.mode));
    }

    kprintln("Entry point:");
    if (!limine_entry_point_request.response) {
        kprintln("  not provided");
    } else {
        kprintlnf("  revision: %llu",
                  (unsigned long long)g_boot_info.entry_point.revision);
    }

    kprintln("Executable file:");
    if (!limine_executable_file_request.response) {
        kprintln("  not provided");
    } else {
        kprintlnf("  revision: %llu",
                  (unsigned long long)g_boot_info.kernel.revision);
        print_limine_file("kernel",
                          g_boot_info.kernel.executable_file);
    }

    kprintln("Modules:");
    if (!limine_module_request.response) {
        kprintln("  not provided");
    } else {
        kprintlnf("  revision: %llu",
                  (unsigned long long)g_boot_info.module.revision);
        kprintlnf("  module_count: %llu",
                  (unsigned long long)g_boot_info.module.module_count);
        if (!g_boot_info.module.modules && g_boot_info.module.module_count) {
            kprintln("  modules: (null)");
        }
        for (uint64_t i = 0; i < g_boot_info.module.module_count; i++) {
            struct limine_file *mod = g_boot_info.module.modules
                ? g_boot_info.module.modules[i]
                : NULL;
            print_limine_file_indexed("module", i, mod);
        }
    }

    kprintln("SMBIOS:");
    if (!limine_smbios_request.response) {
        kprintln("  not provided");
    } else {
        kprintlnf("  revision: %llu",
                  (unsigned long long)g_boot_info.smbios.revision);
        kprintlnf("  entry_32: 0x%llx",
                  (unsigned long long)g_boot_info.smbios.entry_32);
        kprintlnf("  entry_64: 0x%llx",
                  (unsigned long long)g_boot_info.smbios.entry_64);
    }

    kprintln("EFI system table:");
    if (!limine_efi_system_table_request.response) {
        kprintln("  not provided");
    } else {
        kprintlnf("  revision: %llu",
                  (unsigned long long)g_boot_info.efi_system_table.revision);
        kprintlnf("  address: 0x%llx",
                  (unsigned long long)g_boot_info.efi_system_table.address);
    }

    kprintln("EFI memmap:");
    if (!limine_efi_memmap_request.response) {
        kprintln("  not provided");
    } else {
        kprintlnf("  revision: %llu",
                  (unsigned long long)g_boot_info.efi_memmap.revision);
        kprintlnf("  memmap: 0x%llx",
                  (unsigned long long)(uint64_t)(uintptr_t)
                      g_boot_info.efi_memmap.memmap);
        kprintlnf("  memmap_size: %llu",
                  (unsigned long long)g_boot_info.efi_memmap.memmap_size);
        kprintlnf("  desc_size: %llu",
                  (unsigned long long)g_boot_info.efi_memmap.desc_size);
        kprintlnf("  desc_version: %llu",
                  (unsigned long long)g_boot_info.efi_memmap.desc_version);
    }

    kprintln("Date at boot:");
    if (!limine_date_at_boot_request.response) {
        kprintln("  not provided");
    } else {
        kprintlnf("  revision: %llu",
                  (unsigned long long)g_boot_info.boot_date.revision);
        print_signed_u64_line("timestamp", g_boot_info.boot_date.timestamp);
    }

    kprintln("Device tree blob:");
    if (!limine_dtb_request.response) {
        kprintln("  not provided");
    } else {
        kprintlnf("  revision: %llu",
                  (unsigned long long)g_boot_info.device_tree_blob.revision);
        kprintlnf("  dtb_ptr: 0x%llx",
                  (unsigned long long)(uint64_t)(uintptr_t)
                      g_boot_info.device_tree_blob.dtb_ptr);
    }

    kprintln("RISC-V BSP hartid:");
    if (!limine_riscv_bsp_hartid_request.response) {
        kprintln("  not provided");
    } else {
        kprintlnf("  revision: %llu",
                  (unsigned long long)g_boot_info.riscv_bsp_hartid.revision);
        kprintlnf("  bsp_hartid: %llu",
                  (unsigned long long)g_boot_info.riscv_bsp_hartid.bsp_hartid);
    }

    kprintln("Bootloader performance:");
    if (!limine_bootloader_performance_request.response) {
        kprintln("  not provided");
    } else {
        kprintlnf("  revision: %llu",
                  (unsigned long long)g_boot_info.performance.revision);
        kprintlnf("  reset_usec: %llu",
                  (unsigned long long)g_boot_info.performance.reset_usec);
        kprintlnf("  init_usec: %llu",
                  (unsigned long long)g_boot_info.performance.init_usec);
        kprintlnf("  exec_usec: %llu",
                  (unsigned long long)g_boot_info.performance.exec_usec);
    }

    kprintln("Executable address:");
    if (!limine_executable_address_request.response) {
        kprintln("  not provided");
    } else {
        kprintlnf("  revision: %llu",
                  (unsigned long long)g_boot_info.kernel_address.revision);
        kprintlnf("  physical_base: 0x%llx",
                  (unsigned long long)g_boot_info.kernel_address.physical_base);
        kprintlnf("  virtual_base: 0x%llx",
                  (unsigned long long)g_boot_info.kernel_address.virtual_base);
    }

    kprintln("Memory map:");
    if (!limine_memmap_request.response) {
        kprintln("  not provided");
    } else {
        kprintlnf("  revision: %llu",
                  (unsigned long long)g_boot_info.memmap.revision);
        kprintlnf("  entry_count: %llu",
                  (unsigned long long)g_boot_info.memmap.entry_count);
        if (!g_boot_info.memmap.entries && g_boot_info.memmap.entry_count) {
            kprintln("  entries: (null)");
        }
        for (uint64_t i = 0; i < g_boot_info.memmap.entry_count; i++) {
            struct limine_memmap_entry *e = g_boot_info.memmap.entries
                ? g_boot_info.memmap.entries[i]
                : NULL;
            if (!e) {
                kprintlnf("  entry[%llu]: (null)",
                          (unsigned long long)i);
                continue;
            }
            kprintlnf(
                "  entry[%llu]: base=0x%llx length=0x%llx type=%llu (%s)",
                (unsigned long long)i, (unsigned long long)e->base,
                (unsigned long long)e->length, (unsigned long long)e->type,
                memmap_type_name(e->type));
        }
    }

    kprintln("ACPI RSDP:");
    if (!limine_rsdp_request.response) {
        kprintln("  not provided");
    } else {
        kprintlnf("  revision: %llu",
                  (unsigned long long)g_boot_info.acpi.revision);
        kprintlnf("  address: 0x%llx",
                  (unsigned long long)(uint64_t)(uintptr_t)
                      g_boot_info.acpi.address);
    }

    kprintln("SMP:");
    if (!limine_mp_request.response) {
        kprintln("  not provided");
    } else {
        kprintlnf("  revision: %llu",
                  (unsigned long long)g_boot_info.smp.revision);
#if defined(__x86_64__) || defined(__i386__)
        kprintlnf("  flags: 0x%x", (unsigned)g_boot_info.smp.flags);
#else
        kprintlnf("  flags: 0x%llx",
                  (unsigned long long)g_boot_info.smp.flags);
#endif
#if defined(__x86_64__) || defined(__i386__)
        kprintlnf("  x2apic: %s",
                  (g_boot_info.smp.flags & LIMINE_MP_RESPONSE_X86_64_X2APIC)
                      ? "yes"
                      : "no");
        kprintlnf("  bsp_lapic_id: %u",
                  (unsigned)g_boot_info.smp.bsp_lapic_id);
#elif defined(__aarch64__)
        kprintlnf("  bsp_mpidr: 0x%llx",
                  (unsigned long long)g_boot_info.smp.bsp_mpidr);
#elif defined(__riscv) && (__riscv_xlen == 64)
        kprintlnf("  bsp_hartid: %llu",
                  (unsigned long long)g_boot_info.smp.bsp_hartid);
#endif
        kprintlnf("  cpu_count: %llu",
                  (unsigned long long)g_boot_info.smp.cpu_count);
        if (!g_boot_info.smp.cpus && g_boot_info.smp.cpu_count) {
            kprintln("  cpus: (null)");
        }
        for (uint64_t i = 0; i < g_boot_info.smp.cpu_count; i++) {
            struct limine_mp_info *info = g_boot_info.smp.cpus
                ? g_boot_info.smp.cpus[i]
                : NULL;
            if (!info) {
                kprintlnf("  cpu[%llu]: (null)", (unsigned long long)i);
                continue;
            }
            kprintlnf("  cpu[%llu]:", (unsigned long long)i);
#if defined(__x86_64__) || defined(__i386__)
            kprintlnf("    processor_id: %u", (unsigned)info->processor_id);
            kprintlnf("    lapic_id: %u", (unsigned)info->lapic_id);
            kprintlnf("    reserved: 0x%llx",
                      (unsigned long long)info->reserved);
#elif defined(__aarch64__)
            kprintlnf("    processor_id: %u", (unsigned)info->processor_id);
            kprintlnf("    mpidr: 0x%llx", (unsigned long long)info->mpidr);
            kprintlnf("    reserved: 0x%llx",
                      (unsigned long long)info->reserved);
#elif defined(__riscv) && (__riscv_xlen == 64)
            kprintlnf("    processor_id: %llu",
                      (unsigned long long)info->processor_id);
            kprintlnf("    hartid: %llu",
                      (unsigned long long)info->hartid);
            kprintlnf("    reserved: 0x%llx",
                      (unsigned long long)info->reserved);
#endif
            kprintlnf("    goto_address: 0x%llx",
                      (unsigned long long)(uint64_t)(uintptr_t)
                          info->goto_address);
            kprintlnf("    extra_argument: 0x%llx",
                      (unsigned long long)info->extra_argument);
        }
    }

    kprintln("Framebuffer:");
    if (!limine_framebuffer_request.response) {
        kprintln("  not provided");
    } else {
        kprintlnf("  revision: %llu",
                  (unsigned long long)g_boot_info.framebuffer.revision);
        kprintlnf("  framebuffer_count: %llu",
                  (unsigned long long)g_boot_info.framebuffer.framebuffer_count);
        if (!g_boot_info.framebuffer.framebuffers &&
            g_boot_info.framebuffer.framebuffer_count) {
            kprintln("  framebuffers: (null)");
        }
        for (uint64_t i = 0; i < g_boot_info.framebuffer.framebuffer_count;
             i++) {
            struct limine_framebuffer *fb = g_boot_info.framebuffer.framebuffers
                ? g_boot_info.framebuffer.framebuffers[i]
                : NULL;
            if (!fb) {
                kprintlnf("  framebuffer[%llu]: (null)",
                          (unsigned long long)i);
                continue;
            }
            kprintlnf("  framebuffer[%llu]:", (unsigned long long)i);
            kprintlnf("    address: 0x%llx",
                      (unsigned long long)(uint64_t)(uintptr_t)fb->address);
            kprintlnf("    width: %llu", (unsigned long long)fb->width);
            kprintlnf("    height: %llu", (unsigned long long)fb->height);
            kprintlnf("    pitch: %llu", (unsigned long long)fb->pitch);
            kprintlnf("    bpp: %u", (unsigned)fb->bpp);
            kprintlnf("    memory_model: %u (%s)",
                      (unsigned)fb->memory_model,
                      framebuffer_memory_model_name(fb->memory_model));
            kprintlnf("    red_mask_size: %u", (unsigned)fb->red_mask_size);
            kprintlnf("    red_mask_shift: %u", (unsigned)fb->red_mask_shift);
            kprintlnf("    green_mask_size: %u", (unsigned)fb->green_mask_size);
            kprintlnf("    green_mask_shift: %u",
                      (unsigned)fb->green_mask_shift);
            kprintlnf("    blue_mask_size: %u", (unsigned)fb->blue_mask_size);
            kprintlnf("    blue_mask_shift: %u",
                      (unsigned)fb->blue_mask_shift);
            kprintlnf("    edid_size: %llu",
                      (unsigned long long)fb->edid_size);
            kprintlnf("    edid: 0x%llx",
                      (unsigned long long)(uint64_t)(uintptr_t)fb->edid);
            kprintlnf("    mode_count: %llu",
                      (unsigned long long)fb->mode_count);
            if (!fb->modes && fb->mode_count) {
                kprintln("    modes: (null)");
            }
            for (uint64_t m = 0; m < fb->mode_count; m++) {
                struct limine_video_mode *mode = fb->modes
                    ? fb->modes[m]
                    : NULL;
                if (!mode) {
                    kprintlnf("    mode[%llu]: (null)",
                              (unsigned long long)m);
                    continue;
                }
                kprintlnf("    mode[%llu]:", (unsigned long long)m);
                kprintlnf("      pitch: %llu",
                          (unsigned long long)mode->pitch);
                kprintlnf("      width: %llu",
                          (unsigned long long)mode->width);
                kprintlnf("      height: %llu",
                          (unsigned long long)mode->height);
                kprintlnf("      bpp: %u", (unsigned)mode->bpp);
                kprintlnf("      memory_model: %u (%s)",
                          (unsigned)mode->memory_model,
                          framebuffer_memory_model_name(mode->memory_model));
                kprintlnf("      red_mask_size: %u",
                          (unsigned)mode->red_mask_size);
                kprintlnf("      red_mask_shift: %u",
                          (unsigned)mode->red_mask_shift);
                kprintlnf("      green_mask_size: %u",
                          (unsigned)mode->green_mask_size);
                kprintlnf("      green_mask_shift: %u",
                          (unsigned)mode->green_mask_shift);
                kprintlnf("      blue_mask_size: %u",
                          (unsigned)mode->blue_mask_size);
                kprintlnf("      blue_mask_shift: %u",
                          (unsigned)mode->blue_mask_shift);
            }
        }
    }
}
