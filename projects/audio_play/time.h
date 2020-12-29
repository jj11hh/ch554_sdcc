#ifndef __TIME_H__
#include <stdint.h>

typedef uint16_t time_t;

extern volatile uint16_t jiffies;

#define time_after(unknown, known) ((int16_t)(known) - (int16_t)(unknown) < 0)
#define time_before(unknown, known) ((int16_t)(unknown) - (int16_t)(known) <= 0)
#define time_after_eq(unknown, known) ((int16_t)(unknown) - (int16_t)(known) >= 0)
#define time_before_eq(unknown, known) ((int16_t)(known) - (int16_t)(unknown) >= 0)

void sleep_ms(uint16_t);

#endif // __TIME_H__