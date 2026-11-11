#include <stdint.h>
#include "common/prj_conf_opt.h"
#include "common/board/board.h"
#include "driver/chip/hal_uart.h"

// UART0 initialisieren: 115200 Baud, 8N1
extern "C" int trv16_uart_init(void) {
    return board_uart_init(UART0_ID) == HAL_OK ? 0 : -1;
}

// Daten vom HC32L136 empfangen
extern "C" int trv16_uart_receive(
    uint8_t *buffer,
    int32_t length,
    uint32_t timeout_ms
) {
    if (buffer == nullptr || length <= 0) {
        return -1;
    }

    return HAL_UART_Receive_Poll(
        UART0_ID,
        buffer,
        length,
        timeout_ms
    );
}
