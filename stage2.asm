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

    ; Print hex value of current segment (for verification)
    ; mov ax, cs
    ; call print_hex

    mov bx, MSG_CALL_ENABLE_A20
    call print_string

    call enable_a20_fast
    mov bx, MSG_RETURNED_ENABLE_A20
    call print_string


    
    cmp ax, 1
    jne a20_fail
    mov bx, MSG_A20_SUCCESS
    call print_string

enable_a20_fast:
    in al, 0x92
    or al, 2         ; set A20 enable bit
    out 0x92, al

; returns AX = 1 if A20 enabled, AX = 0 if disabled
a20_check:
    pushf
    cli
    push ds
    push es
    push di
    push si

    xor  ax, ax
    mov  ds, ax         ; DS = 0x0000
    mov  si, 0x0500     ; some safe low memory offset (avoid IVT/BDA!)

    mov  ax, 0xFFFF
    mov  es, ax
    mov  di, si         ; ES:DI = 0xFFFF:0x0510 = linear 0x100000+0x510

    ; save original bytes
    mov  al, [ds:si]
    push ax
    mov  al, [es:di]
    push ax

    ; write test pattern
    mov  byte [ds:si], 0x00
    mov  byte [es:di], 0xFF

    ; check if they mirror
    cmp  byte [ds:si], 0x00
    jne  .enabled
    cmp  byte [es:di], 0xFF
    jne  .enabled

    ; restore originals
    pop  ax
    mov  [es:di], al
    pop  ax
    mov  [ds:si], al
    xor  ax, ax         ; disabled
    jmp  .done

.enabled:
    ; restore originals
    pop  ax
    mov  [es:di], al
    pop  ax
    mov  [ds:si], al
    mov  ax, 1          ; enabled

.done:
    pop  si
    pop  di
    pop  es
    pop  ds
    popf
    ret

a20_fail:
    mov bx, A20_ERR_MSG
    call print_string
    jmp $
    
    ; Disable NMI
    ; in al, 0x70       ; citește valoarea curentă din portul RTC Index
    ; or al, 0x80       ; setează bitul 7 (disable NMI)
    ; out 0x70, al

    ;Enabling NMI back
    ; in al, 0x70
    ; and al, 0x7F      ; șterge bitul 7
    ; out 0x70, al

    ; Infinite loop to halt execution
jmp $

a20_already_enabled:
    mov bx, MSG_A20_SUCCESS
    call print_string
    jmp $

MSG_ENTER_STAGE2 db "Entered stage2", 13, 10, 0
MSG_CALL_ENABLE_A20 db "Calling enable_a20", 13, 10, 0
MSG_RETURNED_ENABLE_A20 db "Returned from enable_a20", 13, 10, 0
MSG_A20_SUCCESS db "A20 enabled!", 13, 10, 0
A20_ERR_MSG db "A20 enable failed!", 13, 10, 0

%include "print_string.asm"







; [BITS 16]

; [ORG 0x1000]

; start_stage2:
;     mov bx, MSG_ENTER_STAGE2
;     call print_string
;     cli
;     mov ax, cs
;     mov ds, ax
;     mov es, ax
;     xor ax, ax
;     mov ss, ax
;     mov sp, 0x7C00

;     mov bx, MSG_CALL_ENABLE_A20
;     call print_string

;     call get_a20_state
;     mov dx,ax
;     call print_hex

;     call enable_a20
;     mov bx, MSG_RETURNED_ENABLE_A20
;     call print_string

;     cmp ax, 1
;     jne a20_fail
;     mov bx, MSG_A20_SUCCESS
;     call print_string


;     ; Dezactivează NMI
;     ; mov al, 0x80
;     ; out 0x70, al
;     ; in al, 0x71

;     ; Acum poți trece la GDT / Protected Mode
;     ;jmp setup_gdt



; MSG_ENTER_STAGE2 db "Entered stage2", 13, 10, 0
; MSG_CALL_ENABLE_A20 db "Calling enable_a20", 13, 10, 0
; MSG_RETURNED_ENABLE_A20 db "Returned from enable_a20", 13, 10, 0
; MSG_A20_SUCCESS db "A20 enabled!", 13, 10, 0
; A20_ERR_MSG db "A20 enable failed!", 13, 10, 0
; DEBUG_MSG db "Im here!", 0
; HEX_OUTPUT db "0x0000", 0



; ;---------------------------------------------------------------------------;
; ; get_a20_state - test A20 by comparing byte at 0x0000:0x0500 and 0xFFFF:0x0510
; ; Returns: AX = 1 if A20 on (no wrap), AX = 0 if A20 off (wrap)
; ;---------------------------------------------------------------------------;



; a20_fail:
;     mov bx, A20_ERR_MSG
;     call print_string
;     jmp $

; get_a20_state:
;     pushf
;     push ax
;     push bx
;     push cx
;     push dx
;     push si
;     push di
;     push ds
;     push es

;     cli

;     xor ax, ax
;     mov ds, ax        ; ds = 0x0000
;     mov si, 0x0500    ; ds:si -> physical 0x0500

;     mov ax, 0FFFFh
;     mov es, ax        ; es = 0xFFFF
;     mov di, 0x0510    ; es:di -> physical 0xFFFF0 + 0x0510 = 0x100500

;     ; save originals
;     mov al, [ds:si]
;     mov [temp_below], al
;     mov al, [es:di]
;     mov [temp_over], al

;     ; write test pattern
;     mov ah, 1
;     mov [ds:si], byte 0
;     mov [es:di], byte 1

;     mov al, [ds:si]
;     cmp al, [es:di]
;     jne .a20_on
;     mov ax, 0
;     jmp .restore

; .a20_on:
;     mov ax, 1

; .restore:
;     ; restore bytes
;     mov al, [temp_below]
;     mov [ds:si], al
;     mov al, [temp_over]
;     mov [es:di], al

;     sti

;     pop es
;     pop ds
;     pop di
;     pop si
;     pop dx
;     pop cx
;     pop bx
;     pop ax
;     popf
;     ret

; temp_below: db 0
; temp_over:  db 0

; ;---------------------------------------------------------------------------;
; ; wait_input_clear - wait until keyboard controller input buffer clear (port 0x64 bit1=0)
; ; returns ZF=0 when cleared.  Has a timeout in CX (caller sets). Preserves AL.
; ;---------------------------------------------------------------------------;
; wait_input_clear:
;     ; expects CX = timeout loop counter (decrement)
; .wait1:
;     in al, 0x64
;     test al, 2       ; input buffer full? (1 = data available from KBC, 2 = input buffer full)
;     jz .done
;     loop .wait1
;     stc              ; set carry to signal timeout
;     ret
; .done:
;     clc
;     ret

; ;---------------------------------------------------------------------------;
; ; wait_output_available - wait until keyboard controller output available (port 0x64 bit0=1)
; ; expects CX = timeout
; ;---------------------------------------------------------------------------;
; wait_output_available:
; .wait2:
;     in al, 0x64
;     test al, 1       ; output buffer full?
;     jnz .ok
;     loop .wait2
;     stc
;     ret
; .ok:
;     clc
;     ret

; ;---------------------------------------------------------------------------;
; ; enable_a20_kbd - use keyboard controller (8042) method
; ; returns CF=0 on success check via get_a20_state afterwards
; ;---------------------------------------------------------------------------;
; enable_a20_kbd:
;     pushf
;     push ax
;     push bx
;     push cx
;     push dx

;     cli

;     mov cx, 30000    ; timeout count for waits (caller can adjust)
;     call wait_input_clear
;     jc .kbd_fail
;     mov al, 0xAD     ; disable keyboard
;     out 0x64, al

;     mov cx, 30000
;     call wait_input_clear
;     jc .kbd_fail
;     mov al, 0xD0     ; read output port
;     out 0x64, al

;     mov cx, 30000
;     call wait_output_available
;     jc .kbd_fail
;     in al, 0x60      ; read output port state
;     push ax

;     mov cx, 30000
;     call wait_input_clear
;     jc .kbd_fail
;     mov al, 0xD1     ; write output port
;     out 0x64, al

;     mov cx, 30000
;     call wait_input_clear
;     jc .kbd_fail
;     pop ax
;     or al, 2         ; set bit1 -> enable A20 via KBC
;     out 0x60, al

;     mov cx, 30000
;     call wait_input_clear
;     jc .kbd_fail
;     mov al, 0xAE     ; enable keyboard
;     out 0x64, al

;     sti
;     pop dx
;     pop cx
;     pop bx
;     pop ax
;     popf
;     ret

; .kbd_fail:
;     sti
;     pop dx
;     pop cx
;     pop bx
;     pop ax
;     popf
;     stc             ; signal fail via CF=1
;     ret

; ;---------------------------------------------------------------------------;
; ; enable_a20_bios - call INT 15h function 0x2401 (A20 gate activate)
; ; returns CF clear on no error (but BIOS may still not have enabled A20)
; ;---------------------------------------------------------------------------;
; enable_a20_bios:
;     push ax
;     push bx
;     push cx
;     push dx

;     xor ax, ax
;     mov ax, 0x2401
;     int 0x15
;     jc .err
;     ; AH == 0 => supported (but ignore)
;     clc
;     jmp .done
; .err:
;     stc
; .done:
;     pop dx
;     pop cx
;     pop bx
;     pop ax
;     ret

; ;---------------------------------------------------------------------------;
; ; enable_a20_fast - try port 0x92 fast A20 gate
; ;---------------------------------------------------------------------------;
; enable_a20_fast:
;     pushf
;     push ax
;     push dx

;     in al, 0x92
;     test al, 2
;     jnz .already
;     or al, 2
;     and al, 0FEh    ; clear bit0 safe reset bit as recommended
;     out 0x92, al

; .already:
;     pop dx
;     pop ax
;     popf
;     ret

; ;---------------------------------------------------------------------------;
; ; enable_a20 - orchestrator: try methods in order until success or give up
; ; After return: AX = 1 if A20 on, AX = 0 if off
; ;---------------------------------------------------------------------------;
; enable_a20:
;     push ax
;     push bx
;     push cx
;     push dx
;     push si
;     push di

   
;     ; 1) quick test: already on?
;     call get_a20_state
;     cmp ax, 1
;     je .done_ok
;     ; 2) try BIOS int 15h
;     call enable_a20_bios
    
;     ; test
;     call get_a20_state
;     cmp ax, 1
;     je .done_ok
    
;     ; 3) try keyboard controller (may be slow) with a few retries
;     mov cx, 3        ; try up to 3 times
; .kbd_loop:
;     call enable_a20_kbd
;     ; test
;     call get_a20_state
;     cmp ax, 1
;     je .done_ok
;     loop .kbd_loop

;     ; 4) try fast gate (port 0x92)
;     call enable_a20_fast
;     ; test
;     call get_a20_state
;     cmp ax, 1
;     je .done_ok

;     ; failed
;     mov ax, 0
;     jmp .cleanup

; .done_ok:
;     mov ax, 1

; .cleanup:
;     pop di
;     pop si
;     pop dx
;     pop cx
;     pop bx
;     pop ax
;     ret



; %include "print_string.asm"
; %include "print_hex.asm"