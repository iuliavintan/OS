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
    mov es, ax   ;ES=0 for FS buffers
    sti         ;Enable interrupts
    mov [BootDrive], dl ;Store boot drive for later use

; --- read FS header (LBA 1) ---
    mov word [dap+2], 1          ; read 1 sector
    mov word [dap+4], FS_HDR_BUF ; offset
    mov word [dap+6], 0x0000     ; segment
    mov dword [dap+8], FS_HDR_LBA
    mov dword [dap+12], 0
    mov si, dap                  ; point to disk address packet structure
    mov ah, 0x42                 ; read sectors using LBA
    mov dl, [BootDrive]
    int 0x13
    jc disk_error
    xor cx, cx
    mov es, cx

    ; --- validate FS magic ---
    mov si, FS_HDR_BUF
    cmp dword [es:si], FS_MAGIC
    jne disk_error

    ; --- read FAT (1 sector) ---
    mov eax, [es:si+FS_OFF_FAT_LBA]
    mov dword [dap+8], eax
    mov word [dap+2], 1
    mov word [dap+4], FAT_BUF
    mov word [dap+6], 0x0000
    mov si, dap
    mov ah, 0x42
    mov dl, [BootDrive]
    int 0x13
    jc disk_error
    xor cx, cx
    mov es, cx

    ; --- read root dir (1 sector) ---
    mov si, FS_HDR_BUF
    mov eax, [es:si+FS_OFF_ROOT_LBA]
    mov dword [dap+8], eax
    mov word [dap+2], 1
    mov word [dap+4], ROOT_BUF
    mov word [dap+6], 0x0000
    mov si, dap
    mov ah, 0x42
    mov dl, [BootDrive]
    int 0x13
    jc disk_error
    xor cx, cx
    mov es, cx

    ; --- find STAGE2 entry in root dir ---
    mov cx, FS_ROOT_ENTRIES
    mov di, ROOT_BUF
.find_entry:
    cmp cx, 0
    je disk_error
    push cx
    push di
    mov bx, stage2_name
    mov dx, FS_NAME_LEN
.cmp_loop:
    mov al, [es:di]
    cmp al, [bx]
    jne .no_match
    inc di
    inc bx
    dec dx
    jnz .cmp_loop
    pop di
    pop cx
    jmp read_success
.no_match:
    pop di
    pop cx
    add di, FS_ENTRY_SIZE
    dec cx
    jmp .find_entry

disk_error:
    mov bx, DISK_ERR_MSG 
    call print_string  
    jmp $


; ; --- jump to loaded code ---

read_success:
    ; --- read STAGE2 cluster chain into 0000:1000 ---
    mov ax, [es:di+FS_ENTRY_CLUS]
    cmp ax, 2
    jb disk_error
    mov bx, 0x1000
    xor cx, cx
    mov es, cx
.load_chain:
    cmp ax, 0xFFFF
    je .done_load
    cmp ax, 2
    jb disk_error
    push ax
    ; LBA = data_lba + (cluster-2)
    movzx eax, ax
    sub eax, 2
    add eax, [es:FS_HDR_BUF+FS_OFF_DATA_LBA]
    mov dword [dap+8], eax
    mov word [dap+2], 1
    mov word [dap+4], bx
    mov word [dap+6], es
    mov si, dap
    mov ah, 0x42
    mov dl, [BootDrive]
    int 0x13
    jc disk_error
    xor cx, cx
    mov es, cx
    pop ax
    add bx, 512
    ; next cluster = FAT[cluster]
    mov si, ax
    shl si, 1
    mov ax, [es:FAT_BUF+si]
    jmp .load_chain
.done_load:
    jmp 0x0000:0x1000 ; jump to loaded code (segment:offset)

;------ data ------------ ;
FS_MAGIC equ 0x5446534f ; "OSFT"
FS_HDR_LBA equ 1
FS_HDR_BUF equ 0x0500
FAT_BUF    equ 0x0700
ROOT_BUF   equ 0x0900
FS_ENTRY_SIZE equ 16
FS_NAME_LEN   equ 8
FS_ROOT_ENTRIES equ 32
FS_ENTRY_CLUS equ 8
FS_OFF_FAT_LBA  equ 8
FS_OFF_ROOT_LBA equ 16
FS_OFF_DATA_LBA equ 24
BootDrive db 0 ; to store boot drive (0x80 for first HDD, 0x00 for floppy)
DISK_ERR_MSG db "Disk/FS error!", 0
stage2_name db "STAGE2  "
dap:
    db 0x10     ; size of this structure (16 bytes)
    db 0x00     ; reserved
    dw 0        ; number of sectors to read
    dw 0        ; offset
    dw 0        ; segment
    dq 0        ; starting LBA


; ------------ includes ------------
%include "print_string.inc"

; ------------ boot sector padding ------------
times 510-($-$$) db 0
dw 0xAA55
