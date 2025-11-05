    ; ============================================================================
    ; load_kernel.inc — BIOS EDD (AH=42h) chunked LBA reader
    ;   - include from 16-bit code (no [BITS] here)
    ;   - assumes DS points to this data (you said DS=0; Stage2 linked at 0x1000)
    ;   - before calling load_kernel_after_stage2: set ES = 0x1000, BX = 0
    ; ============================================================================

    %ifndef LOAD_KERNEL_INC
    %define LOAD_KERNEL_INC

    ; --------- Tunables (override before %include if desired) ---------
    %ifndef STAGE2_LBA
    %define STAGE2_LBA      1
    %endif

    %ifndef STAGE2_SECT
    %define STAGE2_SECT     5          ; your padded Stage2 size (2268B → 5)
    %endif

    ; Kernel starts right after Stage2:
    %ifndef KERNEL_LBA_LO
    %define KERNEL_LBA_LO   (STAGE2_LBA + STAGE2_SECT)   ; e.g., 6
    %endif
    %ifndef KERNEL_LBA_HI
    %define KERNEL_LBA_HI   0
    %endif

    ; Kernel size in sectors (set to real number at build time)
    %ifndef KERNEL_SECT
    %define KERNEL_SECT     200
    %endif

    ; Per-call cap (EDD recommends ≤127)
    %ifndef MAX_PER_CALL
    %define MAX_PER_CALL    127
    %endif

    ; ---------------------- Data (below 1 MiB) ------------------------
    BootDrive:      db 0                      ; set at Stage2 entry: mov [BootDrive], dl

    align 4
    dap:            times 16 db 0             ; Disk Address Packet (16 bytes)

    align 4
    LBA_LO:         dd 0
    LBA_HI:         dd 0
; --- constants & data assumed already defined above ---
; BootDrive db 0
; dap: times 16 db 0
; LBA_LO dd 0
; LBA_HI dd 0
; MAX_PER_CALL equ 127

; ---------- EDD LBA reader (fixed boundary calc) ----------
int13_ext_read:
    pusha
    push ds

.read_loop:
    cmp cx, 0
    je  .done

    ; ---- boundary sectors before ES:BX crosses 64KiB ----
    ; bytes_left = 0x10000 - BX  (but 16-bit arithmetic wraps at 0)
    mov ax, bx
    neg ax                       ; ax = (-bx) mod 65536
    jnz .no_wrap
    ; if BX==0, bytes_left is actually 65536 → sectors = 128
    mov si, 128
    jmp .have_boundary
.no_wrap:
    shr ax, 9                    ; /512 → sectors
    mov si, ax
.have_boundary:

    ; chunk = min(remaining(CX), boundary(SI), 127)
    mov ax, cx                   ; remaining
    cmp ax, si
    jbe .ok1
    mov ax, si
.ok1:
%define MAX_PER_CALL 127
    cmp ax, MAX_PER_CALL
    jbe .have_chunk
    mov ax, MAX_PER_CALL
.have_chunk:
    mov di, ax                   ; DI = chunk (16-bit)

    ; ---- build DAP ----
    mov      byte  [dap+0], 0x10
    mov      byte  [dap+1], 0x00
    mov      word  [dap+2], di          ; sectors
    mov      word  [dap+4], bx          ; offset
    mov      word  [dap+6], es          ; segment
    mov      eax,  [LBA_LO]
    mov      dword [dap+8],  eax        ; LBA low dword
    mov      eax,  [LBA_HI]
    mov      dword [dap+12], eax        ; LBA high dword

    ; ---- BIOS AH=42h ----
    mov si, dap
    mov ah, 0x42
    mov dl, [BootDrive]                 ; ensure you set this at Stage-2 entry!
    int 0x13
    jc  .bios_err

    ; ---- advance ES:BX by (chunk * 512) ----
    mov   ax, 512
    mul   di                             ; DX:AX = chunk*512
    add   bx, ax
    jnc   .no_carry
    mov   ax, es
    inc   ax
    mov   es, ax
.no_carry:

    ; ---- LBA += chunk (64-bit) ----
    xor   ecx, ecx
    mov   cx, di
    mov   eax, [LBA_LO]
    add   eax, ecx
    mov   [LBA_LO], eax
    mov   eax, [LBA_HI]
    adc   eax, 0
    mov   [LBA_HI], eax

    ; ---- remaining -= chunk ----
    sub   cx, di
    jmp   .read_loop

.bios_err:
    pop ds
    popa
    stc
    ret

.done:
    pop ds
    popa
    clc
    ret

    ; ------------------------------------------------------------------
    ; load_kernel_after_stage2
    ;   Loads kernel placed immediately after Stage2 to ES:BX.
    ;   Call with ES preset (e.g., ES=0x1000) and BX=0.
    ; ------------------------------------------------------------------
    load_kernel_after_stage2:
        xor bx, bx
        mov dword [LBA_LO], KERNEL_LBA_LO
        mov dword [LBA_HI], KERNEL_LBA_HI
        mov cx, KERNEL_SECT
        call int13_ext_read
        ret

    %endif ; LOAD_KERNEL_INC
