/*
 * Copyright (C) 2022 XRADIO TECHNOLOGY CO., LTD. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the
 *       distribution.
 *    3. Neither the name of XRADIO TECHNOLOGY CO., LTD. nor the names of
 *       its contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <string.h>
#include <sys/endian.h>
#include "kernel/os/os.h"
#include "driver/chip/hal_gpio.h"
#include "driver/chip/hal_spi.h"
#include "kernel/os/os_queue.h"
#include "FreeRTOS.h"
#include "xrlink/xrlink_io.h"

#define GPIO_PORT_SPI_IRQ		    GPIO_PORT_B
#define GPIO_PIN_SPI_IRQ			GPIO_PIN_14

#define GPIO_PORT_SPI_RW			GPIO_PORT_B
#define GPIO_PIN_SPI_RW				GPIO_PIN_15

#define SLAVE_SPI_PORT				SPI0
#define SLAVE_SPI_FIRST_BIT			SPI_TCTRL_FBS_MSB
#define SLAVE_SPI_SCLK_MODE			SPI_SCLK_Mode0

#define SLAVE_SPI_MCLK				(50 * 1000 * 1000)
#define SLAVE_SPI_CS_LEVEL			(0)

#define SPI_DBG_ON		0
#define SPI_WRN_ON		0
#define SPI_ERR_ON		1
#define SPI_INF_ON		1

#define SPI_SYSLOG		printf

#define SPI_LOG(flags, fmt, arg...)	\
    do {									\
        if (flags)							\
           SPI_SYSLOG(fmt, ##arg);	\
    } while (0)

#define SPI_INF(fmt, arg...)	\
    SPI_LOG(SPI_INF_ON, "[SPI DBG] "fmt, ##arg)

#define SPI_DBG(fmt, arg...)	\
    SPI_LOG(SPI_DBG_ON, "[SPI DBG] "fmt, ##arg)

#define SPI_WRN(fmt, arg...)	\
    SPI_LOG(SPI_WRN_ON, "[SPI WRN] "fmt, ##arg)

#define SPI_ERR(fmt, arg...)										\
    do {																\
		SPI_LOG(SPI_ERR_ON, "[SPI ERR] %s():%d, "fmt,	\
               __func__, __LINE__, ##arg);								\
    } while (0)

static int xr_spi_gpio_init(void);
static int xr_spi_gpio_deinit(void);
#if SPI_MASTER
static int xr_spi_reg_irq_ind(irq_ind_func func);
static int xr_spi_read_irq_gpio(void);
static int xr_spi_read_rw_gpio(void);
#else
static int xr_spi_irq_gpio_ctl(uint8_t level);
static void xr_spi_rw_gpio_ctl(uint8_t level);
#endif
static int xr_spi_bus_init(void);
static void xr_spi_bus_deinit(void);
static int xr_spi_bus_write(uint8_t *tx_buf, uint16_t len);
static int xr_spi_bus_read(uint8_t *rx_buf, uint16_t len);


#if SPI_FULL_DUPLEX
static int xr_spi_write_read(uint8_t *tx, uint16_t tx_len,
		uint8_t *rx, uint16_t rx_len);
#endif

#if !SPI_MASTER
static OS_Semaphore_t slave_spi_cs_del_sem;
#endif

static SPI_Device g_spi_cfg_param = {

	.port            = SLAVE_SPI_PORT,
	.cs              = SPI_TCTRL_SS_SEL_SS0,
	.config.mode     = SPI_CTRL_MODE_SLAVE,
	.config.opMode   = SPI_OPERATION_MODE_DMA,
	.config.firstBit = SLAVE_SPI_FIRST_BIT,
	.config.sclk     = SLAVE_SPI_MCLK,
	.config.sclkMode = SLAVE_SPI_SCLK_MODE,
};

static xradio_bus_ops_t spi_bus_ops = {

	.gpio_init     = xr_spi_gpio_init,
	.gpio_deinit   = xr_spi_gpio_deinit,

#if SPI_MASTER
	.read_rw_gpio  = xr_spi_read_rw_gpio,
	.reg_irq_ind    = xr_spi_reg_irq_ind,
	.read_irq_gpio  = xr_spi_read_irq_gpio,
#else
	.irq_gpio_ctl   = xr_spi_irq_gpio_ctl,
#endif

#if SPI_FULL_DUPLEX
	.write_read    = xr_spi_write_read,
#endif
	.init	       = xr_spi_bus_init,
	.write         = xr_spi_bus_write,
	.read          = xr_spi_bus_read,
	.deinit	       = xr_spi_bus_deinit,
};

#if SPI_MASTER
static irq_ind_func irq_func = NULL;
void xr_gpio_irq_eint_callback(void *arg)
{
	if (irq_func)
		irq_func(NULL);
}

#endif

static int xr_spi_gpio_init(void)
{
	GPIO_InitParam param;

	SPI_INF("xradio spi gpio init.\n");

	param.driving	= GPIO_DRIVING_LEVEL_3;
#if SPI_MASTER
	param.mode		= GPIOx_Pn_F0_INPUT;
#else
	param.mode		= GPIOx_Pn_F1_OUTPUT;
#endif
	param.pull		= GPIO_PULL_UP;

	//GPIO RW INIT
	HAL_GPIO_Init(GPIO_PORT_SPI_RW, GPIO_PIN_SPI_RW, &param);
	//GPIO IRQ INIT
	HAL_GPIO_Init(GPIO_PORT_SPI_IRQ, GPIO_PIN_SPI_IRQ, &param);

	xr_spi_irq_gpio_ctl(1);
	xr_spi_rw_gpio_ctl(0);

#if SPI_MASTER
	GPIO_IrqParam irq_param;
	irq_param.arg = NULL;
	irq_param.callback = xr_gpio_irq_eint_callback;
	irq_param.event = GPIO_IRQ_EVT_RISING_EDGE; /*set pin irq rising edge trigger*/
	HAL_GPIO_EnableIRQ(GPIO_PORT_SPI_IRQ, GPIO_PIN_SPI_IRQ, &irq_param);
#endif

	return 0;
}

static int xr_spi_gpio_deinit(void)
{
#if SPI_MASTER
	HAL_GPIO_DisableIRQ(GPIO_PORT_SPI_IRQ, GPIO_PIN_SPI_IRQ);
#endif

	//GPIO IRQ DEINIT
	HAL_GPIO_DeInit(GPIO_PORT_SPI_IRQ, GPIO_PIN_SPI_IRQ);

	//GPIO RW DEINIT
	HAL_GPIO_DeInit(GPIO_PORT_SPI_RW, GPIO_PIN_SPI_RW);

	return 0;
}

#if SPI_MASTER
static int xr_spi_reg_irq_ind(irq_ind_func func)
{
	irq_func = func;
	return 0;
}

static int xr_spi_read_irq_gpio(void)
{
	return (uint8_t)HAL_GPIO_ReadPin(GPIO_PORT_SPI_IRQ, GPIO_PIN_SPI_IRQ);
}

static int xr_spi_read_rw_gpio(void)
{
	return (uint8_t)HAL_GPIO_ReadPin(GPIO_PORT_SPI_RW, GPIO_PIN_SPI_RW);
}

#else
static void xr_spi_rw_gpio_ctl(uint8_t level)
{
	HAL_GPIO_WritePin(GPIO_PORT_SPI_RW, GPIO_PIN_SPI_RW,
			level ? GPIO_PIN_HIGH : GPIO_PIN_LOW);
}

static int xr_spi_irq_gpio_ctl(uint8_t level)
{
	HAL_GPIO_WritePin(GPIO_PORT_SPI_IRQ, GPIO_PIN_SPI_IRQ,
			level ? GPIO_PIN_HIGH : GPIO_PIN_LOW);
	return 0;
}

static void slave_spi_cs_del_irq_cb(uint32_t irq, void *arg)
{
	OS_SemaphoreRelease(&slave_spi_cs_del_sem);
}
#endif

static int xr_spi_bus_init(void)
{
	int ret;

	static const SPI_Global_Config board_spi_param = {
		.mclk	  = SLAVE_SPI_MCLK,
		.cs_level = SLAVE_SPI_CS_LEVEL
	};

	SPI_INF("xradio spi init.\n");

	ret = HAL_SPI_Init(g_spi_cfg_param.port, &board_spi_param);
    if (ret != 0) {
        SPI_ERR("HAL_SPI_Init failed\r\n");
        return -1;
    }
#if SPI_MASTER
	;
#else
#if SPI_FULL_DUPLEX
	if (HAL_SPI_Open(g_spi_cfg_param.port, g_spi_cfg_param.cs,
				&g_spi_cfg_param.config, 5000) != HAL_OK) {
		HAL_SPI_Deinit(g_spi_cfg_param.port);
		SPI_ERR("HAL_SPI_Open() fail\n");
		return -1;
	}
#else
	/* init spi */
	if (HAL_SPI_Slave_Open(g_spi_cfg_param.port, g_spi_cfg_param.cs,
				&g_spi_cfg_param.config, 5000) != HAL_OK) {
		HAL_SPI_Deinit(g_spi_cfg_param.port);
		SPI_ERR("HAL_SPI_Slave_Open() fail\n");
		return -1;
	}

	DMA_ChannelInitParam dma_param;
	memset(&dma_param, 0, sizeof(dma_param));
	dma_param.irqType      = DMA_IRQ_TYPE_NONE;
	dma_param.endCallback  = NULL;

	if (HAL_SPI_Slave_InitTxDMA(g_spi_cfg_param.port, &dma_param) != HAL_OK) {
		SPI_ERR("HAL_SPI_Slave_InitTxDMA() fail\n");
	}

	if (HAL_SPI_Slave_InitRxDMA(g_spi_cfg_param.port, &dma_param) != HAL_OK) {
		SPI_ERR("HAL_SPI_Slave_InitRxDMA() fail\n");
	}
#endif

	{
		static uint8_t is_init_cs_del_sem = 0;
		if (is_init_cs_del_sem == 0) {
			is_init_cs_del_sem = 1;
			if (OS_SemaphoreCreateBinary(&slave_spi_cs_del_sem) != OS_OK) {
				SPI_ERR("create slave_spi_sem_tx fail\r\n");
			}
		}
	}

	SPI_IrqParam irq_param;
	irq_param.irqMask  = SPI_INT_CS_DESELECT;
	/* or SPI_INT_TRANSFER_COMPLETE/SPI_INT_CS_DESELECT */
	irq_param.callback = slave_spi_cs_del_irq_cb;
	irq_param.arg      = NULL;
	HAL_SPI_Slave_EnableIRQ(g_spi_cfg_param.port, &irq_param);
#endif
	return 0;
}

static void xr_spi_bus_deinit(void)
{
	SPI_INF("xradio spi deinit.\n");
#if SPI_MASTER
	;
#else

	HAL_SPI_Slave_StopReceive_DMA(g_spi_cfg_param.port);
	HAL_SPI_Slave_DisableIRQ(g_spi_cfg_param.port);
	HAL_SPI_Slave_DeInitRxDMA(g_spi_cfg_param.port);
	HAL_SPI_Slave_DeInitTxDMA(g_spi_cfg_param.port);
	HAL_SPI_Slave_Close(g_spi_cfg_param.port);
	HAL_SPI_Deinit(g_spi_cfg_param.port);

#endif
}

static int xr_spi_bus_write(uint8_t *tx_buf, uint16_t len)
{
#if SPI_MASTER
	;
#else
	if (HAL_SPI_Slave_StartTransmit_DMA(g_spi_cfg_param.port, tx_buf, len) != HAL_OK) {
		SPI_ERR("HAL_SPI_Slave_StartTransmit_DMA() fail\n");
	}

	xr_spi_rw_gpio_ctl(0);

	if (OS_SemaphoreWait(&slave_spi_cs_del_sem, 5000) != OS_OK) {
		SPI_ERR("%s(), wait timeout\n", __func__);
		HAL_SPI_Slave_StopTransmit_DMA(g_spi_cfg_param.port);
		return -1;
	}

	int32_t left = HAL_SPI_Slave_StopTransmit_DMA(g_spi_cfg_param.port);
	if (left != 0) {
		SPI_WRN("HAL_SPI_Slave_StopTransmit_DMA(), left %d\n", left);
	}
#endif
	return 0;
}

static int xr_spi_bus_read(uint8_t *rx_buf, uint16_t len)
{
#if SPI_MASTER
	;
#else

	if (HAL_SPI_Slave_StartReceive_DMA(g_spi_cfg_param.port, rx_buf, len) != HAL_OK) {
		SPI_ERR("HAL_SPI_Slave_StartReceive_DMA() fail\n");
	}

	xr_spi_rw_gpio_ctl(1);

	if (OS_SemaphoreWait(&slave_spi_cs_del_sem, OS_WAIT_FOREVER) != OS_OK) {
		HAL_SPI_Slave_StopReceive_DMA(g_spi_cfg_param.port);
		return 0;
	}

	int32_t ret = HAL_SPI_Slave_StopReceive_DMA(g_spi_cfg_param.port);
	if (ret != 0) {
		SPI_WRN("HAL_SPI_Slave_StopReceive_DMA(), port %d, left %d\n",
				SLAVE_SPI_PORT, ret);
	} else {
		ret = len;
	}
#endif
	return ret;
}

#if SPI_FULL_DUPLEX
static void xr_spi_write_read_ready(void)
{
	xr_spi_rw_gpio_ctl(1);
}

static int xr_spi_write_read(uint8_t *tx, uint16_t tx_len,
		uint8_t *rx, uint16_t rx_len)
{
	HAL_Status status;

	status = HAL_SPI_TransmitReceive(SLAVE_SPI_PORT, tx,
			rx, tx_len >= rx_len ? tx_len : rx_len,
			xr_spi_write_read_ready);

	xr_spi_rw_gpio_ctl(0);

	return status;
}
#endif

xradio_bus_ops_t *xradio_get_bus_ops(void)
{
	return &spi_bus_ops;
}
