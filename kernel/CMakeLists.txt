add_executable(cincos.elf)

target_compile_options(cincos.elf PRIVATE
  -ffreestanding
  -fno-stack-protector
  -fno-omit-frame-pointer
  -mno-red-zone
  -Wall -Wextra -Werror
)

target_link_options(cincos.elf PRIVATE
  -nostdlib
  -Wl,--build-id=none
  -T ${CMAKE_CURRENT_SOURCE_DIR}/arch/x86_64/boot/linker.ld
)

target_sources(cincos.elf PRIVATE
  core/kmain.c
  core/print.c
  arch/x86_64/boot/entry.S
  arch/x86_64/boot/limine.c
)

target_include_directories(cincos.elf PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/third_party/limine
)

set_target_properties(cincos.elf PROPERTIES OUTPUT_NAME cincos)
