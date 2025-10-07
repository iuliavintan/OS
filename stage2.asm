[BITS 16]
[org 0x1000]               ; stage-1 will load us at 0000:1000

start:
    cli
    mov ax, cs
    mov ds, ax
    sti

    ; Print status message
    mov bx, STATUS_MSG
    call print_string

    ; Print hex value of current segment (for verification)
    mov ax, cs
    call print_hex

    ; Infinite loop to halt execution
    jmp $

;---data----
    STATUS_MSG db "Stage 2 alive!", 0
    HEX_OUTPUT db '0x0000', 0 ; buffer for print_hex output  

; ------------ includes ------------
    %include "print_string.asm"
    %include "print_hex.asm"  
times 512-($-$$) db 0
