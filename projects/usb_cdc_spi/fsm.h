
#ifndef __FSM_H__
#define __FSM_H__

#include <stdint.h>

void fsm_init();
void fsm_feed(uint8_t code);
void fsm_poll(uint8_t *buffer, uint8_t *len);

#endif // __FSM_H__