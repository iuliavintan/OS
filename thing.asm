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



;stiva+stage2 512B, |>----.....---<|, 16 bits, 0x0000, segment:offset -> 0x00000 , setup GDT -> protected mode A20, CR0.PR=1, far jump 

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

;-------reading from disk using LBA---------

mov ah, 0x42       ; read sectors using LBA
mov dl, 0x80      ; first hard disk
mov si, dap       ; pointer to disk address packet
int 0x13
jc .disk_error    ; jump if error

.disk_error:
    mov bx, DISK_ERR_MSG
    call print_string  

jmp $

; ------------ includes ------------
%include "print_string.asm"
%include "print_hex.asm"

; ------------ data ------------ ;
HELLO_MSG    db "Hello, World!", 0
GOODBYE_MSG  db "Goodbye, World!", 0  
HEX_OUTPUT   db "0x0000", 0
DISK_ERR_MSG db "Disk read error!", 0

dap: 
    db 0x10     ; size of this structure (16 bytes)
    db 0x00   ; reserved
    dw 1       ; number of sectors to read
    dw 0x7DFF   ; memory address to load to; offset  (buffer = 0000:1000 => phys 0x1000)
    dw 0x0000    ; segment
    dq 1        ; starting LBA (here: sector right after MBR)


; ------------ boot sector padding ------------
times 510-($-$$) db 0
dw 0xAA55
times 512 dw 0x4321

