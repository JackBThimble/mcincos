add_executable(cincos.elf)

target_compile_options(cincos.elf PRIVATE
  -ffreestanding
  -fno-stack-protector
  -fno-omit-frame-pointer
  -mno-red-zone
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
  core/kmain.c
  core/print.c
  core/panic.c
  arch/x86_64/boot/entry.S
  arch/x86_64/boot/limine.c
  arch/x86_64/cpu/idt.c
  arch/x86_64/cpu/idt_handler.c
  arch/x86_64/cpu/isr.S
  arch/x86_64/cpu/cpu_local.c
  arch/x86_64/cpu/syscall.c
  arch/x86_64/cpu/syscall_entry.S
  arch/x86_64/cpu/gdt.c
  arch/x86_64/cpu/gdt_flush.S
  mem/pmm.c
  mem/vmm.c
)

target_include_directories(cincos.elf PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/mem
  ${CMAKE_CURRENT_SOURCE_DIR}/arch/x86_64/cpu
  ${CMAKE_SOURCE_DIR}/third_party/limine/limine-protocol/include
)

set_target_properties(cincos.elf PROPERTIES OUTPUT_NAME cincos)
