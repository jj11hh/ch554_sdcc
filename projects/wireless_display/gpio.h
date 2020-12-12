#ifndef __GPIO_H__
#define __GPIO_H__

#include <ch554.h>

#define PORT_LED PORT_P3
#define PIN_LED 4
SBIT(LED, PORT_LED, PIN_LED);

#define PORT_IRQ PORT_P3
#define PIN_IRQ 2
SBIT(IRQ, PORT_IRQ, PIN_IRQ);

#define PORT_CE PORT_P3
#define PIN_CE 1
SBIT(CE, PORT_CE, PIN_CE);

#define PORT_CSN PORT_P3
#define PIN_CSN 0
SBIT(CSN, PORT_CSN, PIN_CSN);

SBIT(MOSI, PORT_P1, 5);
SBIT(MISO, PORT_P1, 6);
SBIT(SCK, PORT_P1, 7);

#endif