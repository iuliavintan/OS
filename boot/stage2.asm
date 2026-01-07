[BITS 16]
global start

%define KERNEL_ENTRY 0x00020000   ; matches boot/kernel.ld origin for pm_entry

start:
    cli
    
    mov ax, cs
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7c00          ;setting up a temporary stack
    mov [BootDrive], dl     ; remember BIOS boot drive for later INT 13h calls
  ;  sti
 
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
    ; mov bx, STATUS_MSG 
    ; call print_string  
    call fs_load_tables
    jc fs_fail
    mov si, kernel_name
    call fs_find_entry
    jc fs_fail
    mov [kernel_cluster], ax
    mov ax, 0x2000
    mov es, ax
    mov ax, [kernel_cluster]
    xor bx, bx
    call fs_load_chain
    jc fs_fail


here:
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

    jmp  dword CODE32_SEL:KERNEL_ENTRY        ; far jump into kernel


%include "msg_inc.inc"
%include "load_kernel.inc"
%include "fs.inc"

fs_fail:
    hlt
    jmp $

kernel_name db "KERNEL  "
kernel_cluster dw 0
