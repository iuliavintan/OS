BITS 16
[org 0x7c00]

; --- segment setup so data labels work ---
;cs = code segment; ds- data segment; es-extended data segment, ss- stack segment, sp - stack pointer, bp- base pointer, ip- instruction pointer
;si - index
;segment:offset
start:
    cli         ;Disable interrupts while setting up segments
    mov ax, cs
    mov ds, ax   ;Aligns DS with CS so that code and data are in the same segment
    xor ax, ax   
    mov ss, ax   
    mov sp, 0x7C00 ;Set stack pointer to top of bootloader memory
    sti         ;Enable interrupts
    mov [BootDrive], dl ;Store boot drive for later use


; --- disk read setup ---
disk_read:
    mov si, dap ; point to disk address packet structure
    mov ah, 0x42            ; read sectors using LBA
    mov dl, [BootDrive]            ; first hard disk
    int 0x13
    jc .disk_error          ; jump if error
    jmp read_success

.disk_error:
    mov bx, DISK_ERR_MSG 
    call print_string  
    jmp $


; ; --- jump to loaded code ---

read_success:
    jmp 0x0000:0x1000 ; jump to loaded code (segment:offset)

;------ data ------------ ;
STATUS_MSG db "Am ajuns aici!", 0
HEX_OUTPUT db '0x0000', 0 ; buffer for print_hex output
BootDrive db 0 ; to store boot drive (0x80 for first HDD, 0x00 for floppy)
DISK_ERR_MSG db "Disk read error!", 0
SECTORS equ 3  ; number of sectors to read
dap:
    db 0x10     ; size of this structure (16 bytes)
    db 0x00     ; reserved
    dw SECTORS        ; number of sectors to read
    dw 0x1000   ; memory address to load to; offset  (buffer = 0000:1000 => phys 0x1000)
    dw 0x0000   ; segment
    dq 1        ; starting LBA (here: sector right after MBR)


; ------------ includes ------------
%include "print_string.inc"
%include "print_hex.inc"

; ------------ boot sector padding ------------
times 510-($-$$) db 0
dw 0xAA55

