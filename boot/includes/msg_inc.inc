HEX_OUTPUT db '0x0000', 0 
MSG_ENTER_STAGE2 db "Entered stage2",13, 10, 0
MSG_RETURNED_ENABLE_A20 db "Returned from enable_a20", 13,10, 0
MSG_A20_SUCCESS db "A20 enabled!",13, 10, 0
A20_ERR_MSG db "A20 enable failed!", 13,10,0
E820_SUCCESS_MSG db "E820 memory map obtained!",13,10,0
E820_FAIL_MSG db "E820 memory map failed!",13,10,0
DEBUG_MSG db "Debug: here",13,10,0
STATUS_MSG db "OK",13,10,0

%include "print_string.inc"
%include "print_hex.inc"