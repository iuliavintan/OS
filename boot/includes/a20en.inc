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
   ; mov bx, MSG_A20_SUCCESS
   ; call print_string
    jmp e820_caller
a20_fail:
   ; mov bx, A20_ERR_MSG
   ; call print_string
    jmp $
