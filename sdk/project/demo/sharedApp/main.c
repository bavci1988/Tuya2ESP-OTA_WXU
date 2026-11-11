#include "common/framework/platform_init.h"
#include "serial.h"

void user_main(void);

int main(void)
{
	platform_init();

	serial_init(SERIAL_UART_ID, 115200, UART_DATA_BITS_8, UART_PARITY_NONE, UART_STOP_BITS_1, 1);

	serial_start();

	user_main();

	return 0;
}
