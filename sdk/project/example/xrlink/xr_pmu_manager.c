/*
 * Copyright (C) 2017 XRADIO TECHNOLOGY CO., LTD. All rights reserved.
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

#include "driver/chip/hal_wakeup.h"
#include <stdio.h>
#include "axp210x_drv.h"
#include "axp2101_api.h"
#include "kernel/os/os.h"
#include "FreeRTOS.h"
#include "xrlink/xrlink_io.h"
#include "pm/pm.h"
#include "driver/chip/hal_prcm.h"
#include "xr_pmu_manager.h"

#define HIBERNATION_MAGIC_CODE_VALUE	0XAA
#define HIBERNATION_MAGIC_CODE_ADDRESS	3

#define WLAN_WAKEUP_IO_NUM			(7)
#define WLAN_WAKEUP_IO_MODE_DEF		(WKUPIO_WK_MODE_FALLING_EDGE)
#define WLAN_WAKEUP_IO_PULL_DEF		(GPIO_PULL_DOWN)
#define WLAN_WAKEUP_IO_EVT_BVT_EDGE GPIO_IRQ_EVT_FALLING_EDGE

#define WLAN_RING_WAKEUP_PORT_DEF	GPIO_PORT_A
#define WLAN_RING_WAKEUP_PIN_DEF	WakeIo_To_Gpio(WLAN_WAKEUP_IO_NUM)

#define PMU_MANAGER_THREAD_STACK_SIZE		(1024 * 2)
#define PMU_MANAGER_QUEUE_SIZE	(16)

#define PMU_EVENT_THREAD_STACK_SIZE		(1024 * 2)
#define PMU_EVENT_QUEUE_SIZE	(16)

static OS_Thread_t g_thread_pmu_manager;
static OS_Queue_t pmu_manager_queue;

static OS_Thread_t g_thread_pmu_event;
static OS_Queue_t pmu_event_queue;

typedef enum {
	POWER_IN_IRQ,
	RING_IN_IRQ,
} PMU_Event;

typedef enum {
	POWERON_KEY		= 0X01,
	IRQ_PIN			= 0X02,
	VBUS_INS		= 0X04,
} PMU_Poweron_Source;


typedef struct {
	uint8_t type;
	//uint8_t *payload;
	//uint16_t payload_len;
} xr_pmu_manager_buf_handle_t;

typedef struct {
	uint8_t type;
} xr_pmu_event_buf_handle_t;


__nonxip_text
static void xr_pmu_ring_IrqCb(void *arg)
{
	xr_pmu_event_buf_handle_t item;

	item.type = RING_IN_IRQ;
	OS_QueueSend(&pmu_event_queue, &item, OS_WAIT_FOREVER);
}

void xr_pmu_ring_wakeup_init(void)
{
	GPIO_InitParam param;
	GPIO_IrqParam Irq_param;

	param.driving = GPIO_DRIVING_LEVEL_0;
	param.pull = WLAN_WAKEUP_IO_PULL_DEF;
	param.mode = GPIOx_Pn_F6_EINT;
	HAL_GPIO_Init(WLAN_RING_WAKEUP_PORT_DEF, WLAN_RING_WAKEUP_PIN_DEF, &param);

	Irq_param.event = WLAN_WAKEUP_IO_EVT_BVT_EDGE;
	Irq_param.callback = xr_pmu_ring_IrqCb;
	Irq_param.arg = (void *)0;
	HAL_GPIO_EnableIRQ(WLAN_RING_WAKEUP_PORT_DEF, WLAN_RING_WAKEUP_PIN_DEF, &Irq_param);
	HAL_Wakeup_SetIO(WLAN_WAKEUP_IO_NUM, WLAN_WAKEUP_IO_MODE_DEF, WLAN_WAKEUP_IO_PULL_DEF);

	/*Sakeup io debounce clock source 0  freq is LFCLK 32K*/
	HAL_PRCM_SetWakeupDebClk0(0);
	/*Wakeup IO 5 debounce clock select source 0*/
	HAL_PRCM_SetWakeupIOxDebSrc(WLAN_WAKEUP_IO_NUM, 0);
	/*Wakeup IO 5 input debounce clock cycles is 16+1*/
	HAL_PRCM_SetWakeupIOxDebounce(WLAN_WAKEUP_IO_NUM, 1);
	/*Wakeup IO 5 enable, negative edge,  */
	HAL_Wakeup_SetIO(WLAN_WAKEUP_IO_NUM, WLAN_WAKEUP_IO_MODE_DEF, WLAN_WAKEUP_IO_PULL_DEF);
}

__nonxip_text
static void xr_pmu_prw_in_IrqCb(void *arg)
{
	xr_pmu_event_buf_handle_t item;
	item.type = POWER_IN_IRQ;
	OS_QueueSend(&pmu_event_queue, &item, OS_WAIT_FOREVER);
}

int xr_pmu_ctrl(uint8_t type)
{
	if (type > ENTER_HIBERNATION) {
		PMU_MANAGER_ERR("Invalid control type!\n");
		return -1;
	}

	xr_pmu_manager_buf_handle_t item;
	item.type = type;
	OS_QueueSend(&pmu_manager_queue, &item, OS_WAIT_FOREVER);

	return 0;
}

static void xr_pmu_manager_task(void *arg)
{
	int ret, last_poweron_status;
	xr_pmu_manager_buf_handle_t item;
	xradio_bus_ops_t *spi_bus_ops = xradio_get_bus_ops();

	while (1) {

		ret = OS_QueueReceive(&pmu_manager_queue, &item, OS_WAIT_FOREVER);
		if (ret != OS_OK) {
			PMU_MANAGER_ERR("pmu_manager_queue receive err=%d\n", ret);
		}
		switch (item.type) {
		case POWER_ON_PMU:
			axp2101_poweron();
			break;
		case ENTER_STANDBY:
			if (axp210x_pwr_in_read() == 0) {
				last_poweron_status = 1;
				axp2101_poweroff();
			} else
				last_poweron_status = 0;
			spi_bus_ops->gpio_deinit();
			spi_bus_ops->deinit();
			pm_enter_mode(PM_MODE_STANDBY);
			spi_bus_ops->gpio_init();
			spi_bus_ops->init();
			if (last_poweron_status == 1)
				axp2101_poweron();
			break;

		case ENTER_HIBERNATION:
			if (axp210x_pwr_in_read() == 0) {
				HAL_PRCM_SetCPUAPrivateData(HIBERNATION_MAGIC_CODE_ADDRESS, HIBERNATION_MAGIC_CODE_VALUE);
				axp2101_poweroff();
			} else
				HAL_PRCM_SetCPUAPrivateData(HIBERNATION_MAGIC_CODE_VALUE, 0);
			pm_enter_mode(PM_MODE_HIBERNATION);
			break;

		default:
			PMU_MANAGER_ERR("Invalid pmu manager msg-[0x%08x]!\n", item.type);
		}
	}
}

static void xr_pmu_power_on_handler(void)
{
	uint8_t source;
	source = axp2101_get_poweron_source();
	switch (source) {
	case POWERON_KEY:
		PMU_MANAGER_INF("poweron from poweron key\n");
		break;
	case IRQ_PIN:
		PMU_MANAGER_INF("poweron from 806'irq\n");
		break;
	case VBUS_INS:
		PMU_MANAGER_INF("poweron from vbus\n");
		break;
	default:
		PMU_MANAGER_ERR("Invalid poweron event-[0x%08x]!\n", source);
	}
}

static void xr_pmu_event_task(void *arg)
{
	int ret;
	xr_pmu_event_buf_handle_t item;

	while (1) {

		ret = OS_QueueReceive(&pmu_event_queue, &item, OS_WAIT_FOREVER);
		if (ret != OS_OK) {
			PMU_MANAGER_ERR("pmu_event_queue receive err=%d\n", ret);
		}
		switch (item.type) {
		case POWER_IN_IRQ:
			if (axp210x_pwr_in_read() == 0) {
				xr_pmu_power_on_handler();
			} else{
				PMU_MANAGER_INF("pmu poweroff\n");
			}
			break;
		case RING_IN_IRQ:
			PMU_MANAGER_INF("ring pressed\n");
			break;

		default:
			PMU_MANAGER_ERR("Invalid pmu event-[0x%08x]!\n", item.type);
		}
	}
}
void xrlink_pmu_mamger_init(void)
{
	axp210x_init();
	axp210x_gpio_init(xr_pmu_prw_in_IrqCb);
	xr_pmu_ring_wakeup_init();
	if (HAL_PRCM_GetCPUAPrivateData(HIBERNATION_MAGIC_CODE_ADDRESS) == HIBERNATION_MAGIC_CODE_VALUE) {
		axp2101_poweron();
	}

	if (OS_QueueCreate(&pmu_manager_queue, PMU_MANAGER_QUEUE_SIZE,
				 sizeof(xr_pmu_manager_buf_handle_t)) != OS_OK) {
		PMU_MANAGER_ERR("pmu manger queue create error \r\n");
	}

	OS_ThreadCreate(&g_thread_pmu_manager,
					"pmu_manager",
					xr_pmu_manager_task,
					NULL,
					OS_PRIORITY_HIGH,
					PMU_MANAGER_THREAD_STACK_SIZE);

	if (OS_QueueCreate(&pmu_event_queue, PMU_EVENT_QUEUE_SIZE,
				 sizeof(xr_pmu_event_buf_handle_t)) != OS_OK) {
		PMU_MANAGER_ERR("pmu event queue create error \r\n");
	}

	OS_ThreadCreate(&g_thread_pmu_event,
					"pmu_event",
					xr_pmu_event_task,
					NULL,
					OS_PRIORITY_HIGH,
					PMU_EVENT_THREAD_STACK_SIZE);
}


