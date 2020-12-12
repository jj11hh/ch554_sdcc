#include <stdint.h>
#include <ch554.h>
#include "vscode.h"
#include "gpio.h"

static __xdata uint8_t outputbuffer[32];
static uint8_t output_in;
static uint8_t output_out;
static uint8_t output_size;

static uint8_t opcode;
static uint8_t oprand;
static uint8_t state;

#define STATE_OPCODE (uint8_t)0
#define STATE_OPRAND (uint8_t)1

#define OP_READ_PORT    0
#define OP_WRITE_PORT   1
#define OP_READ_PIN     2
#define OP_WRITE_PIN    3
#define OP_SPI_RW       4

// INSTRU:  7 6 5  |  4 3 2 1 0 | 7 6 5 4 3 2 1 0
//         OPCODE      OPRAND      EXTRA_OPRAND

// WRITE_PIN:    4   |  3 2 1 0
//            state       PIN


static inline uint8_t read_port(uint8_t port) {
    switch (port) {
        case 0: return 0xFF;
        case 1: return P1;
        case 2: return 0xFF;
        case 3: return P3;
        default: return 0xFF;
    }
}

static inline void write_port (uint8_t port, uint8_t data) {
    switch (port) {
        case 0: return;
        case 1: P1 = data; return;
        case 2: return;
        case 3: P3 = data; return;
        default: return;
    }
}

static inline uint8_t read_pin(uint8_t pin) {
    switch (pin) {
        case 0: return LED;
        case 1: return IRQ;
        case 2: return CE;
        case 3: return CSN;
        case 4: return MOSI;
        case 5: return MISO;
        case 6: return SCK;
        default: return 1;
    }
}

static inline void write_pin(uint8_t pin, uint8_t state) {
    switch (pin) {
        case 0: LED = state; return;
        case 1: IRQ = state; return;
        case 2: CE = state; return;
        case 3: CSN = state; return;
        case 4: MOSI = state; return;
        case 5: MISO = state; return;
        case 6: SCK = state; return;
        default: return;
    }
}

static uint8_t spi_rw(uint8_t byte)
{
    uint8_t i;
    for(i = 0; i < 8; ++ i)          // 循环8次
    {
        MOSI = (byte & 0x80);   // byte最高位输出到MOSI
        byte <<= 1;             // 低一位移位到最高位
        SCK = 1;                // 拉高SCK，nRF24L01从MOSI读入1位数据，同时从MISO输出1位数据
        byte |= MISO;           // 读MISO到byte最低位
        SCK = 0;                // SCK置低
    }
    return byte;               // 返回读出的一字节
}

void fsm_init() {
    output_in = 0;
    output_out = 0;
    output_size = 0;
    state = 0;
}

static uint8_t port_or_pin;

void fsm_feed(uint8_t code) {
    if (state == 0) {
        opcode = code >> 5;
        switch (opcode) {
        case OP_READ_PORT:
            port_or_pin = code & (uint8_t)0b00011111;
            outputbuffer[output_in ++] = read_port(port_or_pin);
            if (output_in == 32) output_in = 0;
            output_size ++;
            break;
        case OP_WRITE_PORT:
            port_or_pin = code & (uint8_t)0b00011111;
            state = 1;
            break;
        case OP_READ_PIN:
            port_or_pin = code & (uint8_t)0b00011111;
            outputbuffer[output_in ++] = read_pin(port_or_pin);
            if (output_in == 32) output_in = 0;
            output_size ++;
            break;
        case OP_WRITE_PIN:
            port_or_pin = code & (uint8_t)0b00001111;
            write_pin(port_or_pin, (code & (uint8_t)0b00010000) >> 4);
            break;
        case OP_SPI_RW:
            state = 1;
            break;
        }
    }
    else {
        oprand = code;
        switch (opcode) {
        case OP_WRITE_PORT:
            write_port(port_or_pin, oprand);
            state = 0;
            break;
        case OP_SPI_RW:
            outputbuffer[output_in ++] = spi_rw(oprand);
            if (output_in == 32) output_in = 0;
            output_size ++;
            state = 0;
            break;

        default: // Never 
            state = 0;
        }
    }
}

void fsm_poll (uint8_t *buffer, uint8_t *len) {
    *len = output_size;
    while (output_size) {
        *buffer = outputbuffer[output_out ++];
        if (output_out == 32)
            output_out = 0;
        output_size --;
        buffer ++;
    }
}