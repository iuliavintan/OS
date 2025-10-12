[BITS 16]
[org 0x1000]               ; stage-1 will load us at 0000:1000



start:
    cli
    mov ax, cs
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7c00          ;setting up a temporary stack
    sti
 
    ; Print status message
    mov bx, MSG_ENTER_STAGE2
    call print_string


    call a20_check
    cmp ax,1 
    je a20_already_enabled


    call a20_enable_safe
    mov bx, MSG_RETURNED_ENABLE_A20
    call print_string


    
    cmp ax, 1
    jne a20_fail
    mov bx, MSG_A20_SUCCESS
    call print_string
    jmp e820_caller

e820_map:
    ;getting the memory map using E820

    xor ax, ax
    mov es, ax
    call do_e820
    ret

e820_caller:
    call e820_map
    test ax, ax
    jz .e820_fail
    mov bx, E820_SUCCESS_MSG
    call print_string
    jmp .after_e820
.e820_fail:
    mov bx, E820_FAIL_MSG
    call print_string

.after_e820:
    mov dx,ax 
    call print_hex
    mov bx, HEX_OUTPUT
    call print_string
    jmp load_gdt




; --- A20 enabling code (safe method with fallbacks) ---
a20_enable_safe:
    call a20_check
    cmp  ax, 1
    je   .ok                 ; already enabled -> don't touch anything

    ; Try fast A20 (port 0x92), READ-MODIFY-WRITE only
    in   al, 0x92
    test al, 00000010b       ; if bit1 already set, skip write
    jnz  .verify_fast
    or   al, 00000010b       ; set bit1 (A20); KEEP all other bits as-is!
    ; IMPORTANT: do NOT touch bit0 (reset) or write a literal!
    out  0x92, al
    call io_delay
.verify_fast:
    call a20_check
    cmp  ax, 1
    je   .ok

    ; BIOS fallback (if available in your env)
    mov  ax, 0x2401
    int  0x15
    call a20_check
    cmp  ax, 1
    je   .ok

    ; 8042 fallback (optional in emus; add timeouts to avoid hangs)
    cli
    call kbc_wait_in_clear
    mov  al, 0xD0
    out  0x64, al
    call kbc_wait_out_full
    in   al, 0x60            ; read current output port
    or   al, 00000010b       ; set A20 bit
    mov  ah, al

    call kbc_wait_in_clear
    mov  al, 0xD1            ; write output port
    out  0x64, al
    call kbc_wait_in_clear
    mov  al, ah
    out  0x60, al
    sti

    call a20_check
    cmp  ax, 1
    je   .ok

    xor  ax, ax
    ret
.ok:
    mov  ax, 1
    ret

; --- A20 check: safe bytes, safe offsets ---
a20_check:
    pushf
    cli
    push ds
    push es
    push di
    push si

    xor  ax, ax
    mov  ds, ax
    mov  si, 0x0500          ; safe low offset (avoid IVT/BDA)

    mov  ax, 0xFFFF
    mov  es, ax
    mov  di, si              ; ES:DI = 0xFFFF:0x0500 -> linear 0x100000+0x500

    ; save originals (low and high)
    mov  al, [ds:si]
    push ax                  ; save low byte in AL
    mov  al, [es:di]
    push ax                  ; save high byte in AL

    ; write test pattern
    mov  byte [ds:si], 0x00
    mov  byte [es:di], 0xFF

    ; check results
    mov  al, [ds:si]
    cmp  al, 0x00
    jne  .disabled           ; mismatch => aliasing => A20 disabled
    mov  al, [es:di]
    cmp  al, 0xFF
    jne  .disabled           ; mismatch => aliasing => A20 disabled

    ; enabled: restore bytes and return AX=1
    pop  ax
    mov  [es:di], al
    pop  ax
    mov  [ds:si], al
    mov  ax, 1
    jmp  .done

.disabled:
    ; restore bytes and return AX=0
    pop  ax
    mov  [es:di], al
    pop  ax
    mov  [ds:si], al
    xor  ax, ax

.done:
    pop  si
    pop  di
    pop  es
    pop  ds
    popf
    ret


; --- tiny helpers ---
kbc_wait_in_clear:
    mov cx, 0x4000
.w1: in al, 0x64
    test al, 00000010b
    jz   .ok
    loop .w1
.ok: ret

kbc_wait_out_full:
    mov cx, 0x4000
.w2: in al, 0x64
    test al, 00000001b
    jnz  .ok
    loop .w2
.ok: ret

io_delay:
    in al, 0x80
    in al, 0x80
    ret

a20_already_enabled:
    mov bx, MSG_A20_SUCCESS
    call print_string
    jmp e820_caller
a20_fail:
    mov bx, A20_ERR_MSG
    call print_string
    jmp $

; --- E820 memory map retrieval code ---

 mmap_ent equ 0x8000             ; the number of entries will be stored at 0x8000
do_e820:
        mov di, 0x8004          ; Set di to 0x8004. Otherwise this code will get stuck in `int 0x15` after some entries are fetched 
	xor ebx, ebx		; ebx must be 0 to start
	xor bp, bp		; keep an entry count in bp
	mov edx, 0x0534D4150	; Place "SMAP" into edx
	mov eax, 0xe820
	mov [es:di + 20], dword 1	; force a valid ACPI 3.X entry
	mov ecx, 24		; ask for 24 bytes
	int 0x15
	jc short .failed	; carry set on first call means "unsupported function"
	mov edx, 0x0534D4150	; Some BIOSes apparently trash this register?
	cmp eax, edx		; on success, eax must have been reset to "SMAP"
	jne short .failed
	test ebx, ebx		; ebx = 0 implies list is only 1 entry long (worthless)
	je short .failed
	jmp short .jmpin
.e820lp:
	mov eax, 0xe820		; eax, ecx get trashed on every int 0x15 call
	mov [es:di + 20], dword 1	; force a valid ACPI 3.X entry
	mov ecx, 24		; ask for 24 bytes again
	int 0x15
	jc short .e820f		; carry set means "end of list already reached"
	mov edx, 0x0534D4150	; repair potentially trashed register
.jmpin:
	jcxz .skipent		; skip any 0 length entries
	cmp cl, 20		; got a 24 byte ACPI 3.X response?
	jbe short .notext
	test byte [es:di + 20], 1	; if so: is the "ignore this data" bit clear?
	je short .skipent
.notext:
	mov ecx, [es:di + 8]	; get lower uint32_t of memory region length
	or ecx, [es:di + 12]	; "or" it with upper uint32_t to test for zero
	jz .skipent		; if length uint64_t is 0, skip entry
	inc bp			; got a good entry: ++count, move to next storage spot
	add di, 24
.skipent:
	test ebx, ebx		; if ebx resets to 0, list is complete
	jne short .e820lp
.e820f:
	mov [es:mmap_ent], bp	; store the entry count
	clc			; there is "jc" on end of list to this point, so the carry must be cleared
	ret
.failed:
	stc			; "function unsupported" error exit
	ret


; ---------------- GDT (flat) ----------------
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


load_gdt:
    cli
    lgdt [gdt_desc]

  
    mov eax, cr0
    or eax, 1
    mov cr0, eax      ; enter protected mode
    
    jmp CODE32_SEL:pm_entry  ; far jump -> load CS

[BITS 32]
pm_entry:
    mov ax, DATA_SEL  ; reload data segment registers
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    mov esp, 0x90000  ; setup a 32bit stack

    mov edi, 0xB8000
    mov dword [edi], 0x074B074F    ; 'O' 'K' (little endian: 0x..4B 0x..4F)
.halt:
    hlt
    jmp .halt


; ------------ messages ------------
HEX_OUTPUT db '0x0000', 0 
MSG_ENTER_STAGE2 db "Entered stage2",13, 10, 0
MSG_RETURNED_ENABLE_A20 db "Returned from enable_a20", 13,10, 0
MSG_A20_SUCCESS db "A20 enabled!",13, 10, 0
A20_ERR_MSG db "A20 enable failed!", 13,10,0
E820_SUCCESS_MSG db "E820 memory map obtained!",13,10,0
E820_FAIL_MSG db "E820 memory map failed!",13,10,0
DEBUG_MSG db "Debug: here",13,10,0

%include "print_string.asm"
%include "print_hex.asm"

