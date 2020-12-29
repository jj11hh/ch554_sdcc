#include "track.h"
#include "common.h"

#include <stddef.h>

__code int8_t marry_xmas[] = {

// .org 0
    JOCH0, 8, 
    JOCH1, 9,
    JOCH2, 10,
    JOCH3, 11,

// .org 8
    LJMP, IW(20),
    HALT, NOP, NOP,
    HALT, NOP, NOP,
    HALT, NOP, NOP,

// .org 20

    LDI, 1,
    STA, offsetof(channel_info_t, halt),


    HALT
};