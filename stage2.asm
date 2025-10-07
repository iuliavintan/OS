[BITS 16]
org 0x1000               ; stage-1 will load us at 0000:1000

signature db "S20K!",0
times 512-($-$$) db 0
