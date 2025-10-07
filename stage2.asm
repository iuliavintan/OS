[BITS 16]
[org 0x1000]               ; stage-1 will load us at 0000:1000


;initialise real mode
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
    jmp a20_already_enabled


    mov bx, MSG_CALL_ENABLE_A20
    call print_string


    call a20_enable_safe
    mov bx, MSG_RETURNED_ENABLE_A20
    call print_string


    
    cmp ax, 1
    jne a20_fail
    mov bx, MSG_A20_SUCCESS
    call print_string

;  AX=1 if A20 enabled/now OK, AX=0 on failure
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
    mov  si, 0x0500          ; avoid IVT (0x0000–0x03FF) & BDA (0x0400–0x04FF)

    mov  ax, 0xFFFF
    mov  es, ax
    mov  di, si              ; ES:DI = 0xFFFF:0x0500 -> linear 0x100000+0x500

    mov  al, [ds:si]         ; save originals
    push ax
    mov  al, [es:di]
    push ax

    mov  byte [ds:si], 0x00
    mov  byte [es:di], 0xFF

    cmp  byte [ds:si], 0x00
    jne  .enabled
    cmp  byte [es:di], 0xFF
    jne  .enabled

    pop  ax                  ; restore
    mov  [es:di], al
    pop  ax
    mov  [ds:si], al
    xor  ax, ax
    jmp  .done

.enabled:
    pop  ax
    mov  [es:di], al
    pop  ax
    mov  [ds:si], al
    mov  ax, 1
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
    jmp $
a20_fail:
    mov bx, A20_ERR_MSG
    call print_string
    jmp $


; ------------ messages ------------
MSG_ENTER_STAGE2 db "Entered stage2", 13, 10, 0
MSG_CALL_ENABLE_A20 db "Calling enable_a20", 13, 10, 0
MSG_RETURNED_ENABLE_A20 db "Returned from enable_a20", 13, 10, 0
MSG_A20_SUCCESS db "A20 enabled!", 13, 10, 0
A20_ERR_MSG db "A20 enable failed!", 13, 10, 0

%include "print_string.asm"

