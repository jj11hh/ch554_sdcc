#ifndef __TRACK_H__
#define __TRACK_H__

#include <stdint.h>
#include <stdbool.h>

#define CHANNEL_STACK_SIZE 4

typedef struct {
    bool halt;

    uint16_t reg_a;

    uint16_t stack[CHANNEL_STACK_SIZE];
    uint8_t sp;

    uint16_t pc;

    bool lfo_en;
    uint8_t lfo_depth;
    uint16_t lfo_acc;
    uint16_t lfo_phase;

} channel_info_t;

#define IW(W) ((W)%255), ((W)/255)

enum opcode {
    NOP = 0,
    LDI,
    LDIW,
    STA,
    STW,
    SETVOL,
    SETACC,
    SETPHA,
    SETTEMPO,
    LFOON,
    LFOOFF,
    SETLFODEPTH,
    SETLFOACC,
    PUSHW,
    POPW,
    LDA,
    LDW,
    ADDI,
    ADDIW,
    ADD,
    ADDW,
    SUBI,
    SUBIW,
    SUB,
    SUBW,
    ORI,
    ANDI,
    YIELD,
    DJNZ,
    JNZ,
    JZ,
    JOCH0,
    JOCH1,
    JOCH2,
    JOCH3,
    LJMP,
    AJMP,
    LJMPA,
    LCALL,
    RET,
    HALT,
    KILL,
};

#endif // __TRACK_H__