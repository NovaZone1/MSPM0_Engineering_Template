#include "vofa.hpp"

static uint8_t vofa_buffer[44];
static uint16_t cnt = 0;

void vofa_add_data(float data) {
    vofa_buffer[cnt++] = *((uint8_t *) (&data));
    vofa_buffer[cnt++] = *((uint8_t *) (&data) + 1);
    vofa_buffer[cnt++] = *((uint8_t *) (&data) + 2);
    vofa_buffer[cnt++] = *((uint8_t *) (&data) + 3);
}

void vofa_send(float data) {
    vofa_add_data(data);
    vofa_buffer[cnt++] = 0x00;
    vofa_buffer[cnt++] = 0x00;
    vofa_buffer[cnt++] = 0x80;
    vofa_buffer[cnt++] = 0x7F;

    for (uint8_t i = 0; i < cnt; i++) {
        // 等待TX FIFO非满再发送（关键修复）
        while (DL_UART_isTXFIFOFull(UART1))
            ;
        DL_UART_transmitData(UART1, vofa_buffer[i]);
    }
    cnt = 0;
}