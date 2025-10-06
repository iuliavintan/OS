print_hex:
    pusha
    mov si,0

.next_digit:
    mov ax, dx
    mov cx, 12
    sub cx, si       ; both are 16-bit → valid operands
    shr ax, cl       ; shift count must be in CL
    and al, 0x0F

    cmp al,10
    jb .digit_is_0_9
    add al,'A'-10
    jmp .store
.digit_is_0_9: 
    add al, '0'
.store:
    mov di, si
    shr di,2
    add di,2
    mov [HEX_OUTPUT+di], al

    add si,4
    cmp si,16
    jb .next_digit

    popa 
    ret