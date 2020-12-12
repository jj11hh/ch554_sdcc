// Blink an LED connected to pin 1.7

#include <ch554.h>
#include <debug.h>

#define LED_PIN 4
SBIT(LED, PORT_P3, LED_PIN);

void main() {
    CfgFsys();

    // Configure pin 1.7 as GPIO output
    P3_DIR_PU &= 0x0C;
    P3_MOD_OC = P3_MOD_OC & ~(4<<LED_PIN);
    P3_DIR_PU = P3_DIR_PU |	(4<<LED_PIN);

    while (1) {
    	mDelaymS(100);
        LED = !LED;
    }
}
