BITS 16
ORG 0x7C00

mov si, msg
mov ah, 0x0E          ; teletype output
.print:
    lodsb              ;AL <- [DS:SI]; SI++/--
    test al, al
    jz .hang
    int 0x10
    jmp .print
.hang:
    jmp $

msg db "Booted!", 0

times 510-($-$$) db 0
dw 0xAA55
