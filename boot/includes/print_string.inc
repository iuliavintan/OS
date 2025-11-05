
print_string:   
    cli
    pusha
    mov ah, 0x0E
.next:
    mov al,[bx]
    test al, al
    jz .hang 
    int 0x10
    inc bx
    jmp .next
.hang:
    popa
    ret


print_crlf:
    mov ah,0x0E
    mov al,13
    xor bx,bx
    int 0x10
    mov al,10
    int 0x10
    ret



