OUTPUT_FORMAT(elf32-i386)
ENTRY(pm_entry)

SECTIONS {
    . = 0x1000;      /* echivalentul [org 0x1000] */

    .text : { *(.text*) }
    .data : { *(.data*) }
    .bss  : { *(.bss*) *(COMMON) }
}
