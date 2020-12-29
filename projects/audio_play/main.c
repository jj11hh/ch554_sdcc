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

#include "osc.h"
#include "time.h"
#include "song.h"

volatile uint8_t snd_buf[BUFSIZE];
volatile uint8_t snd_in = 0;
volatile uint8_t snd_out = 0;
volatile uint8_t snd_size = 0;

volatile uint8_t led_pwm_cnt;

volatile uint8_t led_pwm_set[4];

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

void task_mix () {
    while (snd_size < BUFSIZE) {
        snd_buf[snd_in++] = sound_mix();
        snd_in %= BUFSIZE;

        snd_size ++;
    }
}

static const __code uint8_t led_pwm_tbl[] = {
    32,35,38,41,44,47,50,52,55,57,59,60,
    62,63,63,64,64,64,63,63,62,60,59,57,
    55,52,50,47,44,41,38,35,32,29,26,23,
    20,17,14,12,9,7,5,4,2,1,1,0,0,0,1,1,
    2,4,5,7,9,12,14,17,20,23,26,29
};

void task_led () {
    static time_t next_update = 0;
    if (time_after(jiffies, next_update)) {
        uint8_t tbl_idx = (jiffies >> 5) % 64;

        led_pwm_set[0] = led_pwm_tbl[tbl_idx];
        led_pwm_set[1] = led_pwm_tbl[(tbl_idx + 16) % 64];
        led_pwm_set[2] = led_pwm_tbl[(tbl_idx + 32) % 64];
        led_pwm_set[3] = led_pwm_tbl[(tbl_idx + 48) % 64];

        next_update = jiffies + 10;
    }
}

static inline void timer0_reload () {
    TH0 = (65535 - (FREQ_SYS / 12 / 8000)) / 256;
    TL0 = (65535 - (FREQ_SYS / 12 / 8000)) % 256;
}

/*
void TMR1_ISR (void) __interrupt ( INT_NO_TMR1 ) {
    // All LED off
    P1 = 1;
    P3 = 1;

    if (led_pwm_set[0] > led_pwm_cnt) {
        LED1 = 0;
        LED5 = 0;
        LED9 = 0;
    }
    if (led_pwm_set[1] > led_pwm_cnt) {
        LED2 = 0;
        LED6 = 0;
        LED10 = 0;
    }
    if (led_pwm_set[2] > led_pwm_cnt) {
        LED3 = 0;
        LED7 = 0;
        LED11 = 0;
    }
    if (led_pwm_set[3] > led_pwm_cnt) {
        LED4 = 0;
        LED8 = 0;
        LED12 = 0;
    }

    led_pwm_cnt ++;
    led_pwm_cnt %= 32;
}
*/

void TMR_1ms_ISR(void) __interrupt ( INT_NO_TMR0 ) {
    //reload
    timer0_reload();
    static uint8_t cnt = 0;
    uint8_t pwm_cnt;

// assmembly manually
#ifdef MS_DEBUG
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
    if (cnt % 8 == 0) {
        jiffies ++;
    }

    //P1 = 1;
    //P3 = 1;

    LED1 = 1;
    LED2 = 1;
    LED3 = 1;
    LED4 = 1;
    LED5 = 1;
    LED6 = 1;
    LED7 = 1;
    LED8 = 1;
    LED9 = 1;
    LED10 = 1;
    LED11 = 1;
    LED12 = 1;

    pwm_cnt = cnt % 32;
    if (pwm_cnt > led_pwm_set[0]){
        LED1 = 0;
        LED5 = 0;
        LED9 = 0;
    }
    if (pwm_cnt > led_pwm_set[1]){
        LED2 = 0;
        LED6 = 0;
        LED10 = 0;
    }
    if (pwm_cnt > led_pwm_set[2]){
        LED3 = 0;
        LED7 = 0;
        LED11 = 0;
    }
    if (pwm_cnt > led_pwm_set[3]){
        LED4 = 0;
        LED8 = 0;
        LED12 = 0;
    }

    cnt ++;

    /*
    if (jiffies & 0x80)
        TIN2 = 1;
    else
        TIN2 = 0;
    */
#endif
}



void init_channel (void) {
    int i;
    for (i = 0; i < 4; ++ i) {
        channel[i].phase = 0;
        channel[i].freq = 0;
        channel[i].vol = 0;
    }
}
// duration: x10ms
void play_note(uint8_t note, uint8_t duration) {
    channel[0].freq = octave_tbl[note];
    channel[0].vol = 0;
    uint8_t i;
    uint8_t t;
    uint8_t s;

    // 20ms Attack
    for (i = 1; i <= 5; ++ i) {
        channel[0].vol = i * 52;
        sleep_ms(4);
    }

    // 80ms Sustain
    for (i = 0; i < 16; ++ i) {
        channel[0].vol -= 2;
        sleep_ms(5);
    }

    // Remain Release
    s = duration - 10;
    t = channel[0].vol / s;
    for (i = 0; i < s / 4; ++ i) {
        channel[0].vol -= 3 * t;
        sleep_ms(10);
    }
    for (; i < s; ++ i) {
        if (channel[0].vol > t/3)
            channel[0].vol -= t/3;
        else
            channel[0].vol = 0;
        sleep_ms(10);
    }
    channel[0].vol = 0;
}

void main () {
    CfgFsys( );                                                           //CH559时钟选择配置
    mDelaymS(5);

    init_channel();

    // Piezo Buzzer
    P3_DIR_PU |= 1 << 4;
    P3_MOD_OC &= ~ (1 << 4);

    P3 = 0;
    P1 = 0;

    // PWM1 PED
    //P1_DIR_PU |= 1 << 5 | 1 << 4;
    //P1_MOD_OC &= ~ (1 << 5 | 1 << 4);

    PWM_CK_SE = 1;
    PWM_CTRL |= bPWM_CLR_ALL;
    PWM_CTRL &= ~bPWM_CLR_ALL;

    PWM_CTRL |= bPWM2_OUT_EN 
//                | bPWM1_OUT_EN
                | bPWM_IE_END
                | bPWM_IF_END;

    PWM_DATA2 = 127;

    IP_EX = bIP_PWMX;

    //TMOD = bT0_M0 | bT1_M1; // Timer 0 mode 1, Timer 1 mode 2
    TMOD = 0x01;

    timer0_reload();

    // timer1, 256 div, 7812.5 Hz
    // TH1 = 0x01;
    // TL1 = 0x01;

    ET0 = 1;
    TR0 = 1;

    //ET1 = 1;
    //TR1 = 1;

    IE_PWMX = 1;
    EA = 1;


    /*
    CHAN_SINE.freq = freq_2_acc(1000);
    CHAN_SINE.vol = 30;

    while (1) {
        CHAN_SAW.freq = freq_2_acc(400);
        CHAN_SQUARE.freq = freq_2_acc(660);
        uint8_t i = 0;
        for (; i < 10; ++ i) {
            uint8_t v = i * i;
            CHAN_SAW.vol = v;
            CHAN_SQUARE.vol = 100 - v;
            sleep_ms(50);
        }

        CHAN_SAW.vol = 0;
        CHAN_SQUARE.vol = 0;
        sleep_ms(800);
        //volume = (count_ms % 256) >> 2;
    }
    */

#define BEAT (60000/180)
#define DUR (BEAT/10)
#define DUR_2 (BEAT/2)/10
#define BLANK 0

    while(1) {
        play_note(G4, DUR); sleep_ms(BLANK);

        play_note(C5, DUR); sleep_ms(BLANK);
        play_note(C5, DUR_2); play_note(D5, DUR_2);
        play_note(C5, DUR_2); play_note(B5, DUR_2);

        play_note(A5, DUR); sleep_ms(BLANK);
        play_note(A5, DUR); sleep_ms(BLANK);
        play_note(A5, DUR); sleep_ms(BLANK);

        play_note(D5, DUR); sleep_ms(BLANK);
        play_note(D5, DUR_2); play_note(E5, DUR_2);
        play_note(D5, DUR_2); play_note(C5, DUR_2);

        play_note(B5, DUR); sleep_ms(BLANK);
        play_note(G4, DUR); sleep_ms(BLANK);
        play_note(G4, DUR); sleep_ms(BLANK);

        play_note(E5, DUR); sleep_ms(BLANK);
        play_note(E5, DUR_2); play_note(F5, DUR_2);
        play_note(E5, DUR_2); play_note(D5, DUR_2);

        play_note(C5, DUR); sleep_ms(BLANK);
        play_note(A5, DUR); sleep_ms(BLANK);
        play_note(G4, DUR_2); play_note(G4, DUR_2);

        play_note(A5, DUR); sleep_ms(BLANK);
        play_note(D5, DUR); sleep_ms(BLANK);
        play_note(B5, DUR); sleep_ms(BLANK);

        play_note(C5, BEAT/10);
        play_note(G4, DUR); sleep_ms(BLANK);

        play_note(C5, DUR); sleep_ms(BLANK);
        play_note(C5, DUR); sleep_ms(BLANK);
        play_note(C5, DUR); sleep_ms(BLANK);

        play_note(B5, BEAT/10);
        play_note(B5, DUR); sleep_ms(BLANK);

        play_note(C5, DUR); sleep_ms(BLANK);
        play_note(B5, DUR); sleep_ms(BLANK);
        play_note(A5, DUR); sleep_ms(BLANK);

        play_note(G4, BEAT/10);
        play_note(D5, DUR); sleep_ms(BLANK);

        play_note(E5, DUR); sleep_ms(BLANK);
        play_note(D5, DUR); sleep_ms(BLANK);
        play_note(C5, DUR); sleep_ms(BLANK);

        play_note(G5, DUR); sleep_ms(BLANK);
        play_note(G4, DUR); sleep_ms(BLANK);
        play_note(G4, DUR_2); play_note(G4, DUR_2);

        play_note(A5, DUR); sleep_ms(BLANK);
        play_note(D5, DUR); sleep_ms(BLANK);
        play_note(B5, DUR); sleep_ms(BLANK);

        play_note(C5, 3*BEAT/10);

        sleep_ms(3000);
    };
}