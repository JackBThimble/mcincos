ENTRY(_start)

PHDRS
{
  text PT_LOAD FLAGS(5); /* R + X */
  rodata PT_LOAD FLAGS(4); /* R */
  data PT_LOAD FLAGS(6); /* R + W */
}

SECTIONS {
  . = 0xffffffff80000000;
  __kernel_start = .;

  .text : ALIGN(0x1000) {
    *(.text .text.*)
  } : text

  . = ALIGN(0x1000);

  .rodata : ALIGN(0x1000) {
    *(.rodata .rodata.*)
    KEEP(*(.limine_requests))
  } : rodata

  . = ALIGN(0x1000);

  .data : ALIGN(0x1000) {
    *(.data .data.*)
  } : data

  .bss : ALIGN(0x1000) {
    *(COMMON)
    *(.bss .bss.*)
  } : data

  __kernel_end = .;

  /DISCARD/ : {
    *(.eh_frame)
    *(.note*)
    *(.comment)
  }
}

