#include <stdio.h>
#include <string.h>

#include "axp210x_drv.h"
#include "driver/chip/hal_i2c.h"
#include "driver/chip/hal_gpio.h"

#define AXP_I2C_ID	1
#define AXP_PWR_ON_GPIO_PORT		GPIO_PORT_A
#define AXP_PWR_ON_GPIO_PIN			GPIO_PIN_18

#define AXP_PWR_IN_GPIO_PORT		GPIO_PORT_A
#define AXP_PWR_IN_GPIO_PIN			GPIO_PIN_19

#if 0
xradio_bus_ops_t axp210x_ops = {
	.init	    = axp210x_init,
//	.write_read = axp210x_reg_write_read,
	.write      = axp210x_reg_write,
	.read       = axp210x_reg_read,
//	.irq_trigger= xr_spi_irq_trigger,
	.deinit	    = axp210x_deinit,
};

xradio_bus_ops_t *axp210x_get_bus_ops(void)
{
	return &axp210x_ops;
}
#endif

int axp210x_init(void)
{
	HAL_Status status = HAL_ERROR;

	I2C_InitParam initParam;
	initParam.addrMode = I2C_ADDR_MODE_7BIT;
	initParam.clockFreq = 400000;

	status = HAL_I2C_Init(AXP_I2C_ID, &initParam);
	if (status != HAL_OK) {
		printf("IIC init error %d\n", status);
		return -1;
	}

	return 0;
}

void axp210x_gpio_init(irq_pwr_in_func func)
{
	GPIO_InitParam param;
	param.driving = GPIO_DRIVING_LEVEL_1;
	param.mode = GPIOx_Pn_F1_OUTPUT;
	param.pull = GPIO_PULL_NONE;
	HAL_GPIO_Init(AXP_PWR_ON_GPIO_PORT, AXP_PWR_ON_GPIO_PIN, &param);

	param.driving = GPIO_DRIVING_LEVEL_1;
	param.mode = GPIOx_Pn_F6_EINT;
	param.pull = GPIO_PULL_NONE;
	HAL_GPIO_Init(AXP_PWR_IN_GPIO_PORT, AXP_PWR_IN_GPIO_PIN, &param);

	GPIO_IrqParam irq_param;
	irq_param.arg = NULL;
	irq_param.callback = func;
	irq_param.event = GPIO_IRQ_EVT_BOTH_EDGE; /*set pin irq rising edge trigger*/
	HAL_GPIO_EnableIRQ(AXP_PWR_IN_GPIO_PORT, AXP_PWR_IN_GPIO_PIN, &irq_param);
}

void axp210x_deinit(void)
{
	HAL_Status status = HAL_ERROR;

	status = HAL_I2C_DeInit(AXP_I2C_ID);
	if (status != HAL_OK)
		printf("IIC deinit error %d\n", status);

}

void axp210x_pwr_on_ctl(uint8_t level)
{
	HAL_GPIO_WritePin(AXP_PWR_ON_GPIO_PORT, AXP_PWR_ON_GPIO_PIN, level ? GPIO_PIN_HIGH : GPIO_PIN_LOW);
}

uint8_t axp210x_pwr_in_read(void)
{
	return (uint8_t)HAL_GPIO_ReadPin(AXP_PWR_IN_GPIO_PORT, AXP_PWR_IN_GPIO_PIN);
}

int axp210x_reg_read(uint8_t addr, uint8_t reg, uint8_t *data)
{
	int ret;
	int len = 1;
	ret = HAL_I2C_Master_Transmit_IT(AXP_I2C_ID, addr, &reg, 1);
	if (ret != len) {
		printf("IIC transmit error %d\n", ret);
		return -1;
	}

	ret = HAL_I2C_Master_Receive_IT(AXP_I2C_ID, addr, data, len);
	if (ret != len) {
		printf("IIC receive error %d\n", ret);
		return -1;
	}
	return 0;
}

int axp210x_reg_write(uint8_t addr, uint8_t reg, uint8_t data)
{
	int ret;
	int len = 2;
	uint8_t send_data[2];
	send_data[0] = reg;
	send_data[1] = data;
	ret = HAL_I2C_Master_Transmit_IT(AXP_I2C_ID, addr, send_data, len);
	if (ret != len) {
		printf("IIC transmit error %d\n", ret);
		return -1;
	}

	return 0;
}
