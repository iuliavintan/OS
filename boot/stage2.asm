[BITS 16]

extern main                 ; main() in C code (boot/stage2.c)

;[org 0x1000]               ; stage-1 will load us at 0000:1000

start:
    cli
    mov ax, cs
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7c00          ;setting up a temporary stack
    sti
 
    call a20_check
    cmp ax,1 
    je a20_already_enabled

    call a20_enable_safe
    cmp ax, 1
    jne a20_fail

    jmp e820_caller

e820_caller:
    xor ax, ax
    mov es, ax
    call do_e820
    test bp, bp
    jz .e820_fail
    ; mov bx, E820_SUCCESS_MSG
    ; call print_string
    jmp .after_e820
.e820_fail:
;     mov bx, E820_FAIL_MSG
    
;     call print_string
    hlt
    ; jmp $
    

.after_e820:
    jmp load_gdt

; --- A20 enabling code (safe method with fallbacks) ---
%include "a20en.inc"
; --- E820 memory map retrieval code ---
%include "e820_memmap.inc"
; --- GDT setup code ---
%include "gdt.inc"
; --- GDT loading and PE entry code ---

load_gdt:
    [BITS 16]
    cli
    ; 1) mask NMI (CLI does not mask NMI)
    in   al, 0x70
    or   al, 0x80            ; bit7 = 1 disables NMI
    out  0x70, al
    in   al, 0x71            ; dummy read (stabilize)

    ; 2) mask both PICs completely (no IRQs)
    mov  al, 0xFF
    out  0x21, al            ; master PIC
    out  0xA1, al            ; slave PIC

    ; 3) send EOI to clear any in-service IRQs that BIOS left
    mov  al, 0x20
    out  0x20, al            ; EOI master
    out  0xA0, al            ; EOI slave

    mov ax, 0x0003         ; 80x25 text mode (forces VGA text memory active)
    int 0x10

    ; 4) load a dummy IDT (base=0, limit=0) so any stray int faults cleanly
    align 8
    idt_ptr16:  dw 0
                dd 0
    lidt [cs:idt_ptr16]      
    lgdt [cs:gdt_desc]

    ; 6) enter PE and FAR-JUMP
    mov  eax, cr0
    or   eax, 1              ; CR0.PE = 1
    mov  cr0, eax
    jmp  CODE32_SEL:pm_entry ; far jump flushes CS


[BITS 32]
pm_entry:
   cli
    mov ax, DATA_SEL
    mov ds, ax
    mov es, ax         
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x90000
    

    call main  
    
    hlt

    

.halt:  hlt
        jmp .halt


%include "msg_inc.inc"


