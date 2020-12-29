#include "common.h"
#include "time.h"

volatile uint16_t jiffies = 0;

void task_mix(void);
void task_led(void);

void sleep_ms(uint16_t timeout) {
    timeout += jiffies;
    while (time_before(jiffies, timeout)) {
        task_mix();
        task_led();
    }
}