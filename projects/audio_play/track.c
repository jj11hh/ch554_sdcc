#include "osc.h"
#include "time.h"
#include "track.h"
#include <stdbool.h>
#include <string.h>

uint16_t track_addr[4];

uint8_t tempo;
time_t next_update_time;

// REG: ACC, SP, LFO_AV, LFO_PHASE, LFO_INIT_PHASE
// BITS: LFO_ON, LFO_RETRIG, 

// COMMANDS
// JUMP_ON_CHANNEL(channel_x)
// CALL_ON_CHANNEL(channel_x)
// DELAY(ticks)

__xdata channel_info_t channel_ex[4];

void play_sound (__code int8_t *song) {
    memset(channel_ex, 0, sizeof(channel_ex));

    channel_info_t current;

    uint8_t halt_cnt = 0;
    uint8_t op;
    uint8_t i = 0;
    while ( 1 ) {
        halt_cnt = 0;
        for (i = 0; i < 4; ++ i) {

            if (channel_ex[i].halt) {
                halt_cnt ++;
                break;
            }

            memcpy(&current, channel_ex + i, sizeof(channel_info_t));

#define GET_U8(n) *((uint8_t*)(&song[n]))
#define GET_S8(n) *((int8_t*)(&song[n]))
#define GET_U16(n) *((uint16_t*)(&song[n]))
#define GET_S16(n) *((int16_t*)(&song[n]))

            while (! current.halt) {
                op = song[current.pc];
                switch (op) {
                case NOP:
                    current.pc ++;
                    break;
                case LDI:
                    current.reg_a = GET_U8(current.pc + 1);
                    current.pc += 2;
                    break;
                case LDIW:
                    current.reg_a = GET_U16(current.pc + 1);
                    current.pc += 3;
                    break;
                case STA:
                    *((uint8_t*)&current + GET_U8(current.pc + 1)) = current.reg_a & 0xFF;
                    current.pc += 2;
                    break;
                case STW:
                    *((uint16_t*)&current + GET_U8(current.pc + 1)) = current.reg_a;
                    current.pc += 2;
                    break;
                case SETVOL:
                    channel[i].vol = current.reg_a;
                    current.pc ++;
                    break;
                case SETACC:
                    channel[i].freq = current.reg_a;
                    current.pc ++;
                    break;
                case SETPHA:
                    channel[i].phase = current.reg_a;
                    current.pc ++;
                    break;
                case SETTEMPO:
                    tempo = current.reg_a;
                    current.pc ++;
                    break;
                case LFOON:
                    current.lfo_en = 1;
                    current.pc ++;
                    break;
                case LFOOFF:
                    current.lfo_en = 0;
                    current.pc ++;
                    break;
                case SETLFODEPTH:
                    current.lfo_depth = current.reg_a;
                    current.pc ++;
                    break;
                case SETLFOACC:
                    current.lfo_acc = current.reg_a;
                    current.pc ++;
                    break;
                case PUSHW:
                    current.stack[current.sp ++] = current.reg_a;
                    current.pc ++;
                    break;
                case POPW:
                    current.reg_a = current.stack[-- current.sp];
                    current.pc ++;
                    break;
                case LDA:
                    current.reg_a = *((uint8_t*)&current + GET_U8(current.pc + 1));
                    current.pc += 2;
                    break;
                case LDW:
                    current.reg_a = *((uint16_t*)&current + GET_U16(current.pc + 1));
                    current.pc += 3;
                    break;
                case ADDI:
                    current.reg_a += GET_U8(current.pc + 1);
                    current.pc += 2;
                    break;
                case ADDIW:
                    current.reg_a += GET_U16(current.pc + 1);
                    current.pc += 3;
                    break;
                case ADD:
                    current.reg_a += *((uint8_t*)&current + GET_U8(current.pc + 1));
                    current.pc += 2;
                    break;
                case ADDW:
                    current.reg_a += *((uint8_t*)&current + GET_U16(current.pc + 1));
                    current.pc += 3;
                    break;
                case SUBI:
                    current.reg_a -= GET_U8(current.pc + 1);
                    current.pc += 2;
                    break;
                case SUBIW:
                    current.reg_a -= GET_U16(current.pc + 1);
                    current.pc += 3;
                    break;
                case SUB:
                    current.reg_a -= *((uint8_t*)&current + GET_U8(current.pc + 1));
                    current.pc += 2;
                    break;
                case SUBW:
                    current.reg_a -= *((uint8_t*)&current + GET_U16(current.pc + 1));
                    current.pc += 3;
                    break;
                case ORI:
                    current.reg_a |= GET_U8(current.pc + 1);
                    current.pc += 2;
                    break;
                case ANDI:
                    current.reg_a |= GET_U8(current.pc + 1);
                    current.pc += 2;
                    break;
                case YIELD:
                    current.pc ++;
                    goto handle_yield;
                case DJNZ:
                    if (-- current.reg_a) {
                        current.pc += GET_S8(current.pc + 1);
                    }
                    else {
                        current.pc += 2;
                    }
                    break;
                case JNZ:
                    if (current.reg_a) {
                        current.pc += GET_S8(current.pc + 1);
                    }
                    else {
                        current.pc += 2;
                    }
                    break;
                case JZ:
                    if (! current.reg_a) {
                        current.pc += GET_S8(current.pc + 1);
                    }
                    else {
                        current.pc += 2;
                    }
                    break;
                case JOCH1:
                    if (i == 1) {
                        current.pc += GET_S8(current.pc + 1);
                    }
                    else {
                        current.pc += 2;
                    }
                    break;
                case JOCH2:
                    if (i == 2) {
                        current.pc += GET_S8(current.pc + 1);
                    }
                    else {
                        current.pc += 2;
                    }
                    break;
                case JOCH3:
                    if (i == 3) {
                        current.pc += GET_S8(current.pc + 1);
                    }
                    else {
                        current.pc += 2;
                    }
                    break;
                case LJMP:
                    current.pc = GET_U16(current.pc + 1);
                    break;
                case AJMP:
                    current.pc += current.reg_a;
                    break;
                case LJMPA:
                    current.pc = current.reg_a;
                    break;
                case LCALL:
                    current.stack[current.sp ++] = current.pc + 3;
                    current.pc = GET_U16(current.pc + 1);
                    break;
                case RET:
                    current.pc = current.stack[-- current.sp];
                    break;
                case HALT:
                    current.halt = 1;
                    halt_cnt ++;
                    break;
                case KILL:
                    current.halt = 1;
                    goto kill;
                }
            }

            handle_yield:
            memcpy(channel_ex + i, &current, sizeof(channel_info_t));
        }

        if (halt_cnt == 4) 
            goto kill;

        
    }

    kill:
    memset(channel_ex, 0, sizeof(channel_ex));
    channel[0].vol = 0;
    channel[1].vol = 0;
    channel[2].vol = 0;
    channel[3].vol = 0;
}
