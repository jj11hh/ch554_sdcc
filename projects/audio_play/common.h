#ifndef __COMMON_H__
#define __COMMON_H__

#include "ch554.h"

#ifndef SDCC

#define __using(x)
#define __code
#define __interrupt(x)
#define __xdata
#define __idata
#define __data
#define __used
#define __bit _Bool

#endif //SDCC

#ifndef FREQ_SYS
#define FREQ_SYS 24000000
#endif

#define BUFSIZE         8
#define SAMPLE_RATE_DIV 2

SBIT(LED1, 0x90, 4);
SBIT(LED2, 0x90, 5);
SBIT(LED3, 0x90, 6);
SBIT(LED4, 0x90, 7);

SBIT(LED5, 0x90, 1);
SBIT(LED6, 0x90, 0);
SBIT(LED7, 0xB0, 1);
SBIT(LED8, 0xB0, 0);

SBIT(LED9, 0xB0, 3);
SBIT(LED10, 0xB0, 5);
SBIT(LED11, 0x90, 2);
SBIT(LED12, 0x90, 3);

#endif // __COMMON_H__