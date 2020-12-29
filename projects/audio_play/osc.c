#include "osc.h"

osc_t channel[4];
uint8_t duty_of_channels[3] = { 32, 64, 128 };

uint8_t sound_mix () __using (2) {
    int16_t sample = 0;
    uint8_t a, b, c;

#ifdef USE_ALL_SQUARE_WAVE

    // three pulse wave
    for (uint8_t i = 0; i < 3; ++ i) {
        if (channel[i].vol) {
            channel[i].phase += channel[i].freq;
            a = channel[i].phase >> 8;
            if (a < duty_of_channels[i]) {
                sample += (uint8_t) (channel[i].vol >> 1u);
            }
            else {
                sample -= (uint8_t) (channel[i].vol >> 1u);
            }
        }
    }

#else

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

#ifdef SDCC
__asm

    mov a,(_channel + 0x000e)
    jz __mix_sine_end

    ; channel[2].phase += channel[2].freq;

    ; phase _channel + 0x0a
    ; freq _channel + 0x0c
    ; vol _channel + 0x0e
	mov	a,(_channel + 0x000c)
	add	a,(_channel + 0x000a)
	mov	(_channel + 0x000a),a
	mov	a,((_channel + 0x000c) + 1)
	addc	a,((_channel + 0x000a) + 1)
	mov	((_channel + 0x000a) + 1),a   

    mov r4,a ; r4 <- phase << 8

;	main.c:38: if (t < 64u) return sinewave_tbl[t];
	cjne	a,#0x40,__mix_cjne1
__mix_cjne1:
	jnc __mix_jnc1

    ; unneeded
	; mov	a,r4

	add	a,#_sinewave_tbl
	mov	r1,a
	mov	ar3,@r1
	sjmp __mix_sine_end_lookup
__mix_jnc1:
;	main.c:39: if (t < 128u) return sinewave_tbl[127u-t];
	cjne	r4,#0x80,__mix_cjne2
__mix_cjne2:
	jnc	__mix_jnc2
	mov	a,#0x7f
	clr	c
	subb	a,r4
	add	a,#_sinewave_tbl
	mov	r1,a
	mov	ar3,@r1
	sjmp __mix_sine_end_lookup
__mix_jnc2:
;	main.c:40: if (t < 192u) return 255u - sinewave_tbl[t-128u];
	cjne	r4,#0xc0,__mix_cjne3
__mix_cjne3:
	jnc	__mix_jnc3

    ; unneeded
	; mov	a,r4

	add	a,#0x80
	add	a,#_sinewave_tbl
	mov	r1,a
	mov	ar2,@r1
	mov	a,#0xff
	clr	c
	subb	a,r2
	mov	r3,a
	sjmp __mix_sine_end_lookup
__mix_jnc3:
;	main.c:41: return 255 - sinewave_tbl[255u-t];
	mov	a,#0xff
	clr	c
	subb	a,r4
	add	a,#_sinewave_tbl
	mov	r1,a
	mov	ar4,@r1
	mov	a,#0xff
	clr	c
	subb	a,r4
	mov	r3,a
;	main.c:222: c = sin_8bit(a);
__mix_sine_end_lookup:

    mov a,r3 ; acc <- sin_8bit(a)
    mov b,(_channel + 0x0e) ; b <- vol

    mul ab ; b <- (a*b) >> 8

    ; sample += ((vol * sin_8bit(a)) /255) - (vol/2)
    mov a,(_channel + 0x0e) ; acc = vol
    clr c
    rrc a ; a = vol / 2
    xch a,b ; b = vol /2, a = (vol * sin_8bit(a)) / 255
    clr c
    subb a,b ; a = a - b, a is a int8_t

    jc __mix_a_is_neg_sine
    add a,r6
    mov r6,a
    clr a
    addc a,r7
    mov r7,a
    sjmp __mix_sine_end
    __mix_a_is_neg_sine:
    add a,r6
    mov r6,a
    mov a,#0xFF
    addc a,r7
    mov r7,a

__mix_sine_end:

__endasm;

#else

    // sine wave
    b = channel[2].vol;
    if (b) {
        channel[2].phase += channel[2].freq;
        a = channel[2].phase >> 8;
        c = sin_8bit(a);
        sample += (int8_t)(((uint16_t)c * (uint8_t)b) >> 8u) - (b >> 1);
    }


#endif // SDCC

#endif // USE_ALL_SQUARE_WAVE

    // noise
    b = channel[3].vol;
    if (b) {
        channel[3].phase += channel[3].freq;

        a = channel[3].phase >> 8;

        c = rand8() % b;
        // sample = (int8_t)c - (b / 2)
        b = b >> 1;
        sample += c;
        sample -= b;
    }

    sample += 0xFF;
    if (sample > 0xFF) 
        return 0xFF;
    else if (sample < 0)
        return 0;
    else
        return sample & 0xFF;
}


