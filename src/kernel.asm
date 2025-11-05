[BITS 32]
global pm_entry
extern kmain
pm_entry:
   cli
    mov ax, 0x10
    mov ds, ax
    mov es, ax         
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov ebp, 0x90000
    mov esp, ebp

    call kmain  
    
    hlt

.halt:  hlt
        jmp .halt

%include "gdt.inc"
