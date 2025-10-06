BITS 16
[org 0x7c00]

; --- segment setup so data labels work ---
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
mov dl, 0x80            ; first hard disk
int 0x13
jc .disk_error          ; jump if error

.disk_error:
    mov bx, DISK_ERR_MSG
    call print_string  
    jmp $

; --- jump to loaded code ---

jmp 0x0000:1000 ;Jump to loaded code at 0000:1000 (physical 0x1000)

;------------ data ------------ ;
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

; ------------ boot sector padding ------------
times 510-($-$$) db 0
dw 0xAA55


