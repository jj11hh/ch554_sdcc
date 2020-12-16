/********************************** (C) COPYRIGHT *******************************
* File Name          : CDC.C
* Author             : WCH
* Version            : V1.0
* Date               : 2017/03/01
* Description        : CH554 as CDC device to serial port, select serial port 1
*******************************************************************************/
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include <ch554.h>
#include <debug.h>

#ifndef SDCC

#define __using(x)
#define __code
#define __interrupt(x)
#define __xdata
#define __idata
#define __data
#define __used

#endif //SDCC

#ifndef FREQ_SYS
#define FREQ_SYS 24000000
#endif

#define BUFSIZE         8
#define SAMPLE_RATE_DIV 4

extern uint8_t sinewave_tbl[64];

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

typedef struct {
    uint16_t phase; // 0 - 255
    uint16_t freq;  // phase_acc = freq
    uint8_t vol;
} osc_t;

// chan 0: square wave
// chan 1: saw wave
// chan 2: sine wave
// chan 3: noise wave

osc_t channel[4];
volatile uint16_t jiffies = 0;


volatile uint8_t snd_buf[BUFSIZE];
volatile uint8_t snd_in = 0;
volatile uint8_t snd_out = 0;
volatile uint8_t snd_size = 0;

// freq = 16_000_000 / 256 ~= 62.5 kHz
void PWM_ISR(void) __interrupt ( INT_NO_PWMX ) __using (1) {
    static uint8_t count = 0;
    PWM_CTRL |= bPWM_IF_END;

    if (!count && snd_size) {
        PWM_DATA2 = snd_buf[snd_out ++];
        snd_out %= BUFSIZE;
        snd_size --;
    }

    count ++;
    count %= SAMPLE_RATE_DIV;
}

static inline uint8_t rand8() {
    static uint8_t rnd = 0x8B;
    // rnd = rnd * 5 + 17;
    rnd = (rnd << 2) + rnd + 17;
    return rnd;
}

/*
static int func (int8_t a) {
    static int16_t s = 12345;

    s += a;
    return s;    
}

static void func (uint8_t __code *p) {
    while (*p) P1 = *(p ++);
}
*/



uint8_t sound_mix () __using (2) {
    int16_t sample = 0;
    uint8_t a, b, c;

    // square wave
    b = channel[0].vol;

    if (b) {
        channel[0].phase += channel[0].freq;
        a = channel[0].phase >> 8;
        if (a & 0x80)
            sample += (uint8_t)(channel[0].vol >> 1u);
        else
            sample -= (uint8_t)(channel[0].vol >> 1u);
    }

    // saw wave

#if SDCC
    __asm 
    mov a,(_channel + 9) ; acc = channel[1].vol
    jz __mix_saw_end

    ; channel[1].phase += channel[1].freq;

    ; phase _channel + 5
    ; freq _channel + 7
    ; vol _channel + 9
	mov	a,(_channel + 0x0007)
	add	a,(_channel + 0x0005)
	mov	(_channel + 0x0005),a
	mov	a,((_channel + 0x0007) + 1)
	addc	a,((_channel + 0x0005) + 1)
	mov	((_channel + 0x0005) + 1),a

    ; now a == channel[1].phase >> 8

    mov b,(_channel + 9) ; b = channel[1].vol
    mul ab

    ; sample += ((vol * (phase / 255)) /255) - (vol/2)
    mov a,(_channel + 9) ; acc = vol
    clr c
    rrc a ; a = vol / 2
    xch a,b ; b = vol /2, a = (vol * (phase >> 8)) >> 8
    clr c
    subb a,b ; a = a - b, a is a int8_t

    ; sample -> r6 : r7

    jc __mix_a_is_neg
    add a,r6
    mov r6,a
    clr a
    addc a,r7
    mov r7,a
    sjmp __mix_saw_end
    __mix_a_is_neg:
    add a,r6
    mov r6,a
    mov a,#0xFF
    addc a,r7
    mov r7,a

    __mix_saw_end:
    __endasm;

#else

    b = channel[1].vol;

    if (b) {
        channel[1].phase += channel[1].freq;
        a = channel[1].phase >> 8;
        if (a & 0x80)
            sample += (uint8_t)(channel[1].vol >> 1u);
        else
            sample -= (uint8_t)(channel[1].vol >> 1u);
    }

#endif

    // (channel + 0x000a) = channel[2].phase
    // (channel + 0x000c) = channel[2].freq
    // (channel + 0x000e) = channel[2].vol

    // sine wave
    b = channel[2].vol;
    if (b) {
        channel[2].phase += channel[2].freq;
        a = channel[2].phase >> 8;

        c = sin_8bit(a);
        sample += (int8_t)(((uint16_t)c * (uint8_t)b) >> 8u) - (b >> 1);
    }

    // noise
    b = channel[3].vol;
    if (b) {
        channel[3].phase += channel[3].freq;

        a = channel[3].phase >> 8;

        c = rand8() % b;
        sample += (int8_t)c - (b >> 1);
    }

    sample += 0xFF;
    if (sample > 0xFF) 
        return 0xFF;
    else if (sample < 0)
        return 0;
    else
        return sample & 0xFF;
}

void background_task () {
    while (snd_size < BUFSIZE) {
        snd_buf[snd_in++] = sound_mix();
        snd_in %= BUFSIZE;

        snd_size ++;
    }
}


static inline void timer0_reload () {
    TH0 = (65535 - (FREQ_SYS / 12 / 1000)) / 256;
    TL0 = (65535 - (FREQ_SYS / 12 / 1000)) % 256;
}

void TMR_1ms_ISR(void) __interrupt ( INT_NO_TMR0 ) {
    //reload
    timer0_reload();

// assmembly manually
#ifdef SDCC
    __asm
        mov a,_jiffies
        inc a
        setb _TIN2
        jb acc.7,0001$
        clr _TIN2
        0001$:
        mov _jiffies,a
        jnz 0002$
        inc (_jiffies + 1)
        0002$:
    __endasm;
#else
    jiffies ++;
    if (jiffies & 0x80)
        TIN2 = 1;
    else
        TIN2 = 0;
#endif
}

#define time_after(unknown, known) ((int16_t)(known) - (int16_t)(unknown) < 0)
#define time_before(unknown, known) ((int16_t)(unknown) - (int16_t)(known) <= 0)
#define time_after_eq(unknown, known) ((int16_t)(unknown) - (int16_t)(known) >= 0)
#define time_before_eq(unknown, known) ((int16_t)(known) - (int16_t)(unknown) >= 0)

void sleep_ms(uint16_t timeout) {
    timeout += jiffies;
    while (time_before(jiffies, timeout)) {
        background_task();
    }
}

void init_channel (void) {
    int i;
    for (i = 0; i < 4; ++ i) {
        channel[i].phase = 0;
        channel[i].freq = 0;
        channel[i].vol = 0;
    }
}

void main () {
    CfgFsys( );                                                           //CH559时钟选择配置
    mDelaymS(5);

    init_channel();

    P3_DIR_PU |= 1 << 4;
    P3_MOD_OC &= ~ (1 << 4);

    // PWM1 PED
    P1_DIR_PU |= 1 << 5 | 1 << 4;
    P1_MOD_OC &= ~ (1 << 5 | 1 << 4);

    PWM_CK_SE = 1;
    PWM_CTRL |= bPWM_CLR_ALL;
    PWM_CTRL &= ~bPWM_CLR_ALL;

    PWM_CTRL |= bPWM2_OUT_EN 
                | bPWM1_OUT_EN
                | bPWM_IE_END
                | bPWM_IF_END;

    PWM_DATA2 = 127;

    TMOD = 0x01; // Timer 0 mode 1

    timer0_reload();

    ET0 = 1;
    TR0 = 1;

    IE_PWMX = 1;
    EA = 1;


    channel[0].freq = freq_2_acc(1000);
    channel[0].vol = 30;
    channel[1].vol = 10;
    channel[2].vol = 10;
    channel[3].vol = 10;

    while (1) {
        channel[2].freq = freq_2_acc(400);
        uint8_t i = 0;
        for (; i < 10; ++ i) {
            channel[2].vol = i * i;
            sleep_ms(50);
        }
        sleep_ms(800);
        //volume = (count_ms % 256) >> 2;
    }
}