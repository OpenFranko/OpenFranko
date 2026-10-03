OUTPUT_FORMAT("elf32-m68k")
OUTPUT_ARCH(m68k)
ENTRY(_start)

MEMORY
{
  ROM (rx) : ORIGIN = 0x802000, LENGTH = 0x5FE000
  RAM (rwx) : ORIGIN = 0x4000, LENGTH = 0x1FC000
}

__stack_top = 0x200000;
__stack_size = 0x8000;

SECTIONS
{
  .boot :
  {
    KEEP(*(.text.boot))
  } > ROM

  .text : ALIGN(8)
  {
    __text_start = .;
    *(.text.unlikely .text.*_unlikely .text.unlikely.*)
    *(.text.exit .text.exit.*)
    *(.text.startup .text.startup.*)
    *(.text.hot .text.hot.*)
    *(.text .stub .text.* .gnu.linkonce.t.*)
    . = ALIGN(8);
    __text_end = .;
  } > RAM AT > ROM

  __text_load = LOADADDR(.text);

  .init :
  {
    PROVIDE(_init = .);
    KEEP(*crtbegin.o(.init))
    KEEP(*(EXCLUDE_FILE(*crtend.o) .init))
    KEEP(*crtend.o(.init))
    SHORT(0x4E75)
  } > ROM

  .fini :
  {
    PROVIDE(_fini = .);
    KEEP(*(SORT_NONE(.fini)))
    SHORT(0x4E75)
  } > ROM

  .rodata :
  {
    *(.rodata .rodata.* .gnu.linkonce.r.*)
  } > ROM

  .eh_frame :
  {
    KEEP(*crtbegin.o(.eh_frame))
    KEEP(*(EXCLUDE_FILE(*crtend.o) .eh_frame))
    KEEP(*crtend.o(.eh_frame))
  } > ROM

  .gcc_except_table :
  {
    *(.gcc_except_table .gcc_except_table.*)
  } > ROM

  .preinit_array :
  {
    PROVIDE_HIDDEN(__preinit_array_start = .);
    KEEP(*(.preinit_array))
    PROVIDE_HIDDEN(__preinit_array_end = .);
  } > ROM

  .init_array :
  {
    PROVIDE_HIDDEN(__init_array_start = .);
    KEEP(*(SORT_BY_INIT_PRIORITY(.init_array.*) SORT_BY_INIT_PRIORITY(.ctors.*)))
    KEEP(*(.init_array EXCLUDE_FILE(*crtbegin.o *crtend.o) .ctors))
    PROVIDE_HIDDEN(__init_array_end = .);
  } > ROM

  .fini_array :
  {
    PROVIDE_HIDDEN(__fini_array_start = .);
    KEEP(*(SORT_BY_INIT_PRIORITY(.fini_array.*) SORT_BY_INIT_PRIORITY(.dtors.*)))
    KEEP(*(.fini_array EXCLUDE_FILE(*crtbegin.o *crtend.o) .dtors))
    PROVIDE_HIDDEN(__fini_array_end = .);
  } > ROM

  .ctors :
  {
    KEEP(*crtbegin.o(.ctors))
    KEEP(*(EXCLUDE_FILE(*crtend.o) .ctors))
    KEEP(*(SORT(.ctors.*)))
    KEEP(*(.ctors))
  } > ROM

  .dtors :
  {
    KEEP(*crtbegin.o(.dtors))
    KEEP(*(EXCLUDE_FILE(*crtend.o) .dtors))
    KEEP(*(SORT(.dtors.*)))
    KEEP(*(.dtors))
  } > ROM

  .data : ALIGN(8)
  {
    __data_start = .;
    *(.data .data.* .gnu.linkonce.d.*)
    *(.tm_clone_table)
    . = ALIGN(8);
    __data_end = .;
  } > RAM AT > ROM

  __data_load = LOADADDR(.data);
  __rom_end = ALIGN(__data_load + SIZEOF(.data), 8);

  .bss (NOLOAD) : ALIGN(8)
  {
    __bss_start = .;
    *(.bss .bss.* .gnu.linkonce.b.*)
    *(COMMON)
    . = ALIGN(8);
    __bss_end = .;
  } > RAM

  __heap_start = ALIGN(__bss_end, 8);
  __heap_end = __stack_top - __stack_size;
  PROVIDE(end = __heap_start);
  PROVIDE(_end = __heap_start);

  /DISCARD/ :
  {
    *(.comment)
    *(.note .note.*)
  }
}
