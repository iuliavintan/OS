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

;---data----
    STATUS_MSG db "Stage 2 alive!", 0
    HEX_OUTPUT db '0x0000', 0 ; buffer for print_hex output  

; ------------ includes ------------
    %include "print_string.asm"
    %include "print_hex.asm"  
times 512-($-$$) db 0
