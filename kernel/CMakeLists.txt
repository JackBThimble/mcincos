add_executable(cincos.elf)

target_compile_options(cincos.elf PRIVATE
  -ffreestanding
  -fno-stack-protector
  -fno-omit-frame-pointer
  -mno-red-zone
  -mwaitpkg
  -fno-pic
  -Wall -Wextra -Werror
)

target_link_options(cincos.elf PRIVATE
  -nostdlib
  -Wl,-no-pie
  -Wl,--build-id=none
  -T ${CMAKE_CURRENT_SOURCE_DIR}/arch/x86_64/boot/linker.ld
)

target_sources(cincos.elf PRIVATE
  boot/boot_info.c
  core/kmain.c
  core/print.c
  core/panic.c
  core/sched.c
  core/timerq.c
  core/string.c
  arch/x86_64/boot/entry.S
  arch/x86_64/boot/limine.c
  arch/x86_64/cpu/arch.c
  arch/x86_64/cpu/idt.c
  arch/x86_64/cpu/idt_handler.c
  arch/x86_64/cpu/isr.S
  arch/x86_64/cpu/cpu_local.c
  arch/x86_64/cpu/syscall.c
  arch/x86_64/cpu/syscall_entry.S
  arch/x86_64/cpu/gdt.c
  arch/x86_64/cpu/gdt_flush.S
  arch/x86_64/cpu/pic.c
  arch/x86_64/cpu/pit.c
  arch/x86_64/cpu/irq.c
  arch/x86_64/cpu/lapic.c
  arch/x86_64/cpu/timer.c
  arch/x86_64/cpu/relax.c
  mm/pmm.c
  mm/vmm.c
  mm/mmio.c
)

target_include_directories(cincos.elf PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/mm
  ${CMAKE_CURRENT_SOURCE_DIR}/arch/x86_64/cpu
  ${CMAKE_SOURCE_DIR}/third_party/limine/limine-protocol/include
)

set_target_properties(cincos.elf PROPERTIES OUTPUT_NAME cincos)
