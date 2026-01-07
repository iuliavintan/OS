; ============================================================================
; fs.inc — minimal FAT-style reader (16-bit FAT, 1-sector FAT/root)
; ============================================================================

%ifndef FS_INC
%define FS_INC

FS_MAGIC        equ 0x5446534f ; "OSFT"
FS_HDR_LBA      equ 1
FS_HDR_BUF      equ 0x2000
FAT_BUF         equ 0x2200
ROOT_BUF        equ 0x2400
FS_ENTRY_SIZE   equ 16
FS_NAME_LEN     equ 8
FS_ROOT_ENTRIES equ 32

FS_OFF_FAT_LBA   equ 8
FS_OFF_ROOT_LBA  equ 16
FS_OFF_DATA_LBA  equ 24


; Reads FS header, FAT, and root dir into buffers.
fs_load_tables:
    push ax
    push bx
    push es
    mov ax, ds
    mov es, ax
    mov bx, FS_HDR_BUF
    mov dword [LBA_LO], FS_HDR_LBA
    mov dword [LBA_HI], 0
    mov cx, 1
    call int13_ext_read
    jc .fail
    cmp dword [FS_HDR_BUF], FS_MAGIC
    jne .fail

    mov eax, [FS_HDR_BUF+FS_OFF_FAT_LBA]
    mov dword [LBA_LO], eax
    mov bx, FAT_BUF
    mov cx, 1
    call int13_ext_read
    jc .fail

    mov eax, [FS_HDR_BUF+FS_OFF_ROOT_LBA]
    mov dword [LBA_LO], eax
    mov bx, ROOT_BUF
    mov cx, 1
    call int13_ext_read
    jc .fail

    clc
    jmp .done
.fail:
    stc
.done:
    pop es
    pop bx
    pop ax
    ret

; Finds entry by 8-byte name (DS:SI). On success: AX=start_cluster, CF=0.
fs_find_entry:
    push bx
    push dx
    push di
    push si
    mov di, ROOT_BUF
    mov cx, FS_ROOT_ENTRIES
.find_loop:
    cmp cx, 0
    je .not_found
    push cx
    push di
    push si
    mov bx, si
    mov dx, FS_NAME_LEN
.cmp_loop:
    mov al, [di]
    cmp al, [bx]
    jne .no_match
    inc di
    inc bx
    dec dx
    jnz .cmp_loop
    pop si
    pop di
    pop cx
    mov ax, [di+8]               ; start cluster
    clc
    jmp .done
.no_match:
    pop si
    pop di
    pop cx
    add di, FS_ENTRY_SIZE
    dec cx
    jmp .find_loop
.not_found:
    stc
.done:
    pop si
    pop di
    pop dx
    pop bx
    ret

; Loads a cluster chain starting at AX into ES:BX.
fs_load_chain:
    push cx
    push dx
    push si
    push di
.next_cluster:
    cmp ax, 0xFFFF
    je .done
    cmp ax, 2
    jb .fail_invalid
    push ax
    movzx eax, ax
    sub eax, 2
    add eax, [FS_HDR_BUF+FS_OFF_DATA_LBA]
    mov dword [LBA_LO], eax
    mov dword [LBA_HI], 0
    mov cx, 1
    call int13_ext_read
    jc .fail_read
    add bx, 512
    pop ax
    mov si, ax
    shl si, 1
    mov ax, [FAT_BUF+si]
    jmp .next_cluster
.fail_invalid:
    stc
    jmp .done2
.fail_read:
    pop ax
    stc
    jmp .done2
.done:
    clc
.done2:
    pop di
    pop si
    pop dx
    pop cx
    ret

%endif ; FS_INC
