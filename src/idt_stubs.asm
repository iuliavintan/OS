; section .text
; bits 32

; global idt_flush
; idt_flush:
;     mov eax, [esp + 4]   ; Get the pointer to the IDT
;     lidt [eax]           ; Load the IDT
;     sti                 ; Enable interrupts
;     ret
    
; %macro ISR_NOERR 1
;     global isr%1
;     isr%1:
;         cli
;         push dword 0      ; Push a dummy error code
;         push dword %1     ; Push the interrupt number
;         jmp isr_common_stub
; %endmacro


; %macro ISR_ERR 1
;     global isr%1
;     isr%1:
;         cli
;         push dword %1     ; Push the interrupt number
;         jmp isr_common_stub
; %endmacro

; %macro IRQ 2
;     global irq%1
;     irq%1:
;         cli
;         push dword 0
;         push dword %2
;         jmp irq_common_stub
; %endmacro





; ISR_NOERR 0
; ISR_NOERR 1
; ISR_NOERR 2
; ISR_NOERR 3
; ISR_NOERR 4
; ISR_NOERR 5
; ISR_NOERR 6
; ISR_NOERR 7

; ISR_ERR 8
; ISR_NOERR 9
; ISR_ERR 10
; ISR_ERR 11
; ISR_ERR 12
; ISR_ERR 13
; ISR_ERR 14
; ISR_NOERR 15
; ISR_NOERR 16
; ISR_NOERR 17
; ISR_NOERR 18
; ISR_NOERR 19
; ISR_NOERR 20
; ISR_NOERR 21
; ISR_NOERR 22
; ISR_NOERR 23
; ISR_NOERR 24
; ISR_NOERR 25
; ISR_NOERR 26
; ISR_NOERR 27
; ISR_NOERR 28
; ISR_NOERR 29
; ISR_NOERR 30
; ISR_NOERR 31
; ISR_NOERR 128
; ISR_NOERR 177

; IRQ 0,32
; IRQ 1,33
; IRQ 2,34
; IRQ 3,35
; IRQ 4,36
; IRQ 5,37
; IRQ 6,38
; IRQ 7,39
; IRQ 8,40
; IRQ 9,41
; IRQ 10,42
; IRQ 11,43
; IRQ 12,44
; IRQ 13,45
; IRQ 14,46
; IRQ 15,47


; extern isr_handler
; isr_common_stub:
;     pusha                     ; Push all general-purpose registers
;     mov eax, ds               ; Return from interrupt
;     push eax
;     mov eax, cr2              ; Get faulting address (for page faults)
;     push eax

;     mov ax, 0x10
;     mov ds, ax                ; Load data segment selectors
;     mov es, ax          
;     mov fs, ax          
;     mov gs, ax

;     push esp                  ; Push pointer to stack frame
;     call isr_handler          ; Call the C handler

;     add esp, 8              ; Clean up the stack (error code + interrupt number)
;     pop ebx             ; Restore registers
;     mov ds, bx
;     mov es, bx
;     mov fs, bx
;     mov gs, bx
    
;     popa
;     add esp, 8
;     sti
;     iret


; extern irq_handler
; irq_common_stub:
;     pusha                     ; Push all general-purpose registers
;     mov eax, ds               ; Return from interrupt
;     push eax
;     mov eax, cr2              ; Get faulting address (for page faults)
;     push eax

;     mov ax, 0x10
;     mov ds, ax                ; Load data segment selectors
;     mov es, ax          
;     mov fs, ax          
;     mov gs, ax

;     push esp                  ; Push pointer to stack frame
;     call irq_handler          ; Call the C handler

;     add esp, 8              ; Clean up the stack (error code + interrupt number)
;     pop ebx             ; Restore registers
;     mov ds, bx
;     mov es, bx
;     mov fs, bx
;     mov gs, bx
    
;     popa
;     add esp, 8
;     sti
;     iret


; idt.asm


section .text
[BITS 32]

; ---- lidt helper ----
global idt_flush
idt_flush:
    mov eax, [esp + 4]     ; arg: pointer to idt_ptr (6 bytes)
    lidt [eax]
    ret

; ---- macros to define stubs ----
%macro ISR_NOERR 1
global isr%1
isr%1:
    push dword 0           ; dummy err_code
    push dword %1          ; int_no
    jmp  isr_common_stub
%endmacro

%macro ISR_ERR 1
global isr%1
isr%1:
    push dword %1          ; int_no (CPU already pushed err_code)
    jmp  isr_common_stub
%endmacro

%macro IRQ 2
global irq%1
irq%1:
    push dword 0           ; dummy err_code
    push dword %2          ; int_no = remapped vector
    jmp  irq_common_stub
%endmacro

; ---- declare C handlers ----
extern isr_handler
extern irq_handler

; ---- common ISR stub ----
isr_common_stub:
    pusha
    push ds
    push es
    push fs
    push gs

    mov  eax, cr2
    push eax            ; extra dword lives ABOVE the segs

    push esp
    call isr_handler
    add  esp, 4

    add  esp, 4         ; discard cr2 we added
    pop  gs
    pop  fs
    pop  es
    pop  ds
    popa
    add  esp, 8         ; int_no + err_code
    iretd

; isr_common_stub:
;     ; save segment registers (individually)
;     push ds
;     push es
;     push fs
;     push gs

;     pusha

;     ; (optional) capture CR2 for page-fault diagnostics
;     ; mov  eax, cr2
;     ; push eax               ; cr2


;     ; switch to kernel data segments
;     mov  ax, 0x10          ; your kernel data selector
;     mov  ds, ax
;     mov  es, ax
;     mov  fs, ax
;     mov  gs, ax

;     ; pass pointer to our frame to C
;     push esp               ; arg: &frame (top = cr2)
;     call isr_handler
;     add  esp, 4            ; pop arg

;     ; discard cr2
;     ; add  esp, 4

;     popa

;     ; restore segment registers (reverse order!)
;     pop  gs
;     pop  fs
;     pop  es
;     pop  ds

;     ; drop int_no + err_code we pushed/received
;     add  esp, 8
;     iretd

; ---- common IRQ stub ----
irq_common_stub:
    push ds
    push es
    push fs
    push gs

    pusha

   

    mov  ax, 0x10
    mov  ds, ax
    mov  es, ax
    mov  fs, ax
    mov  gs, ax

    push esp
    call irq_handler
    add  esp, 4

    popa
    pop  gs
    pop  fs
    pop  es
    pop  ds

    add  esp, 8
    iretd

; ---- exceptions 0..31 ----
ISR_NOERR 0
ISR_NOERR 1
ISR_NOERR 2
ISR_NOERR 3
ISR_NOERR 4
ISR_NOERR 5
ISR_NOERR 6
ISR_NOERR 7
ISR_ERR   8
ISR_NOERR 9
ISR_ERR   10
ISR_ERR   11
ISR_ERR   12
ISR_ERR   13
ISR_ERR   14
ISR_NOERR 15
ISR_NOERR 16
ISR_NOERR 17
ISR_NOERR 18
ISR_NOERR 19
ISR_NOERR 20
ISR_NOERR 21
ISR_NOERR 22
ISR_NOERR 23
ISR_NOERR 24
ISR_NOERR 25
ISR_NOERR 26
ISR_NOERR 27
ISR_NOERR 28
ISR_NOERR 29
ISR_NOERR 30
ISR_NOERR 31

; optional extras
ISR_NOERR 128
ISR_NOERR 177

; ---- PIC IRQs (after remap to 0x20..0x2F) ----
IRQ 0,32
IRQ 1,33
IRQ 2,34
IRQ 3,35
IRQ 4,36
IRQ 5,37
IRQ 6,38
IRQ 7,39
IRQ 8,40
IRQ 9,41
IRQ 10,42
IRQ 11,43
IRQ 12,44
IRQ 13,45
IRQ 14,46
IRQ 15,47
