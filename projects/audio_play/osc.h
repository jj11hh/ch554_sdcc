#ifndef __OSC_H__
#define __OSC_H__

#include <stdint.h>
#include "common.h"


typedef struct {
    uint16_t phase; // 0 - 255
    uint16_t freq;  // phase_acc = freq
    uint8_t vol;
} osc_t;

// chan 0: square wave
// chan 1: saw wave
// chan 2: sine wave
// chan 3: noise wave

extern osc_t channel[4];
extern uint8_t duty_of_channels[3];
extern __code const uint8_t sinewave_tbl[64];
extern __code const uint16_t octave_tbl[96];

#define CHAN_SQUARE   channel[0]
#define CHAN_SAW      channel[1]
#define CHAN_SINE     channel[2]
#define CHAN_NOISE    channel[3]

static inline uint8_t sin_8bit(uint8_t t) {
    if (t < 64u) return sinewave_tbl[t];
    if (t < 128u) return sinewave_tbl[127u-t];
    if (t < 192u) return 255u - sinewave_tbl[t-128u];
    return 255 - sinewave_tbl[255u-t];
}

static inline uint16_t freq_2_acc(uint16_t f) {
    uint16_t t = (FREQ_SYS / 256 / SAMPLE_RATE_DIV) / f;
    return 65535 / t;
} 

#define FREQ_2_ACC(f) ((uint16_t)(65535/((FREQ_SYS/256/SAMPLE_RATE_DIV)/(f))))

static inline uint8_t rand8() {
    static uint8_t rnd = 0x8B;
    // rnd = rnd * 5 + 17;
    rnd = (rnd << 2) + rnd + 17;
    return rnd;
}

uint8_t sound_mix() __using(2);


#define C0      0x00
#define CS0     0x01
#define D0      0x02
#define DS0     0x03
#define E0      0x04
#define F0      0x05
#define FS0     0x06
#define G0      0x07
#define GS0     0x08
#define A0      0x09
#define AS0     0x0A
#define B0      0x0B
#define C1      0x0C
#define CS1     0x0D
#define D1      0x0E
#define DS1     0x0F
#define E1      0x10
#define F1      0x11
#define FS1     0x12
#define G1      0x13
#define GS1     0x14
#define A1      0x15
#define AS1     0x16
#define B1      0x17
#define C2      0x18
#define CS2     0x19
#define D2      0x1A
#define DS2     0x1B
#define E2      0x1C
#define F2      0x1D
#define FS2     0x1E
#define G2      0x1F
#define GS2     0x20
#define A2      0x21
#define AS2     0x22
#define B2      0x23
#define C3      0x24
#define CS3     0x25
#define D3      0x26
#define DS3     0x27
#define E3      0x28
#define F3      0x29
#define FS3     0x2A
#define G3      0x2B
#define GS3     0x2C
#define A3      0x2D
#define AS3     0x2E
#define B3      0x2F
#define C4      0x30
#define CS4     0x31
#define D4      0x32
#define DS4     0x33
#define E4      0x34
#define F4      0x35
#define FS4     0x36
#define G4      0x37
#define GS4     0x38
#define A4      0x39
#define AS4     0x3A
#define B4      0x3B
#define C5      0x3C
#define CS5     0x3D
#define D5      0x3E
#define DS5     0x3F
#define E5      0x40
#define F5      0x41
#define FS5     0x42
#define G5      0x43
#define GS5     0x44
#define A5      0x45
#define AS5     0x46
#define B5      0x47
#define C6      0x48
#define CS6     0x49
#define D6      0x4A
#define DS6     0x4B
#define E6      0x4C
#define F6      0x4D
#define FS6     0x4E
#define G6      0x4F
#define GS6     0x50
#define A6      0x51
#define AS6     0x52
#define B6      0x53
#define C7      0x54
#define CS7     0x55
#define D7      0x56
#define DS7     0x57
#define E7      0x58
#define F7      0x59
#define FS7     0x5A
#define G7      0x5B
#define GS7     0x5C
#define A7      0x5D
#define AS7     0x5E
#define B7      0x5F

#endif //__OSC_H__