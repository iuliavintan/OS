/* kernel.ld */
OUTPUT_FORMAT(elf32-i386)
ENTRY(pm_entry)
SECTIONS {
  . = 0x00010000;             /* kernel runs from 0x10000 */
  .text : { *(.text*) }
  .rodata : { *(.rodata*) }
  .data :  { *(.data*) }
  .bss  :  { *(.bss*) *(COMMON) }
}
