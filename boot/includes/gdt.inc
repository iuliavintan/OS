 align 8
gdt_start:
gdt_null:                   ; selector 0x00
    dq 0

gdt_code32:                 ; selector 0x08  (32-bit CS)
    dw 0xFFFF               ; limit 0..15
    dw 0x0000               ; base 0..15
    db 0x00                 ; base 16..23
    db 0x9a           ; access = 0x9A (code, readable, ring0, present)
    db 0xcf           ; flags+limitHi = 0xCF (G=1, D=1, L=0, AVL=0; limitHi=0xF)
    db 0x00                 ; base 24..31

gdt_data:                   ; selector 0x10  (DS/ES/SS/FS/GS)
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 0x92           ; access = 0x92 (data, rw, ring0, present)
    db 0xcf            ; 0xCF (G=1, D=1)
    db 0x00

gdt_end:

gdt_desc:
    dw gdt_end - gdt_start - 1
    dd gdt_start

; handy selectors
CODE32_SEL equ (gdt_code32 - gdt_start)    ; 0x08
DATA_SEL   equ (gdt_data   - gdt_start)    ; 0x10