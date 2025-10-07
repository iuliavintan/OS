BITS 16
[org 0x7c00]

; --- segment setup so data labels work ---
;cs = code segment; ds- data segment; es-extended data segment, ss- stack segment, sp - stack pointer, bp- base pointer, ip- instruction pointer
;si - index
;segment:offset
cli         ;Disable interrupts while setting up segments
mov ax, cs
mov ds, ax   ;Aligns DS with CS so that code and data are in the same segment
xor ax, ax   
mov ss, ax   
mov sp, 0x7C00 ;Set stack pointer to top of bootloader memory
sti         ;Enable interrupts
mov [BootDrive], dl ;Store boot drive for later use


; --- disk read setup ---
mov si, dap ; point to disk address packet structure
mov ah, 0x42            ; read sectors using LBA
mov dl, [BootDrive]            ; first hard disk
int 0x13

jc .disk_error          ; jump if error
jmp read_success


; ; CF=1 → error: AH has status
; push ax               ; save AX
; push dx
; push ds
; push cs               ; DS=CS so strings/HEX_OUTPUT work
; pop ds
; mov dx, ax            ; AH=error code (high), AL undefined
; call print_hex        ; show 0x00AH
; mov bx, DISK_ERR_MSG
; call print_string
; pop ds
; pop dx
; pop ax
; jmp $
.disk_error:
    mov bx, DISK_ERR_MSG 
    call print_string  
    jmp $

read_success:
; --- dump memory content ---
;dump 16 bytes at 0x1000 for debugging
mov ax, 0x0000
mov ds, ax
mov si, 0x1000
mov cx, 16


; assumes DS points to the segment of the memory you want to dump
; SI = starting offset (e.g. 0x1000)
; CX = number of bytes to dump

dump_loop:
    lodsb                 ; AL = [DS:SI], SI++
    xor ah, ah
    mov dx, ax            ; DX = 00AL
    call print_hex        ; fills HEX_OUTPUT with "0x??"
    mov bx, HEX_OUTPUT
    call print_string     ; print the buffer

    ; print a space
    mov ah, 0x0E
    mov al, ' '
    int 0x10

    loop dump_loop
    ret


; --- jump to loaded code ---

jmp 0x0000:1000 ;Jump to loaded code at 0000:1000 (physical 0x1000)


;------------ data ------------ ;
STATUS_MSG db "Am ajuns aici!", 0
HEX_OUTPUT db 0
BootDrive db 0 ; to store boot drive (0x80 for first HDD, 0x00 for floppy)
DISK_ERR_MSG db "Disk read error!", 0
SECTORS equ 1  ; number of sectors to read
dap:
    db 0x10     ; size of this structure (16 bytes)
    db 0x00     ; reserved
    dw SECTORS        ; number of sectors to read
    dw 0x1000   ; memory address to load to; offset  (buffer = 0000:7DFF => phys 0x7DFF)
    dw 0x0000   ; segment
    dq 1        ; starting LBA (here: sector right after MBR)


; ------------ includes ------------
%include "print_string.asm"
%include "print_hex.asm"

; ------------ boot sector padding ------------
times 510-($-$$) db 0
dw 0xAA55


