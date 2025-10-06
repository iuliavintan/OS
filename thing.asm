; BITS 16
; org 0x7c00

; mov bx,HELLO_MSG
; call printstring

; mov bx,GOODBYE_MSG
; call printstring



; %include "print_string.asm"

; HELLO_MSG db "Hello, World!",0
; GOODBYE_MSG db "Goodbye, World!",0

; %include "print_hex.asm"

; mov bx, HEX_OUTPUT      ; should print the template
; call printstring       ; expect: 0x0000
; mov dx, 0x7754
; call print_hex
; mov bx, HEX_OUTPUT
; call printstring
; jmp $

; HEX_OUTPUT db '0x0000',0
; times 510-($-$$) db 0
; dw 0xAA55

;----------------------------------------------------------------

BITS 16
org 0x7C00

; --- segment setup so data labels work ---
; mov ax, cs
; mov ds, ax

; --- Hello ---
mov bx, HELLO_MSG               ;ax = 0x4444
call print_string

; --- show template, then converted value ---
mov bx, HEX_OUTPUT       ; expect: 0x0000
call print_string

mov dx, 0x7754
call print_hex           ; must preserve regs internally (pusha/popa)

mov bx, HEX_OUTPUT       ; expect: 0x7754
call print_string

; --- Goodbye ---
mov bx, GOODBYE_MSG
call print_string

jmp $

; ------------ includes ------------
%include "print_string.asm"
%include "print_hex.asm"

; ------------ data ------------ ;db 1B; dw 2B, dd- 4B, dq- 8B
HELLO_MSG    db "Hello, Worldasjdkasjhfkjashfkdjhflaksjhflkajsdhflkasjhfkalsjdhf!", 0
GOODBYE_MSG  db "Goodbye, World!", 0
HEX_OUTPUT   db "0x0000", 0

; ------------ boot sector padding ------------
times 510-($-$$) db 0
dw 0xAA55

