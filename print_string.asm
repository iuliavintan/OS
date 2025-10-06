print_string:   
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
