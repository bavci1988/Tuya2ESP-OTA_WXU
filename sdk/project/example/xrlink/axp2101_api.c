/*
 * (C) Copyright 2018-2020
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * wangwei <wangwei@allwinnertech.com>
 *
 */
#include <stdio.h>
#include <string.h>

#include "axp2101_api.h"
#include "axp.h"
#include "axp210x_drv.h"
#include "kernel/os/os.h"

static int pmu_set_vol(char *name, int set_vol, int onoff);
#define VDD_SYS_VOL (920)
#define VOL_ON (1)
#define ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))

/*
name,min_volta,max_volta,contrl_reg,电压控制有效的位,开关寄存器,开关位,寄存器偏移值
*/

static axp_contrl_info axp_ctrl_tbl[] = {
	{ "dcdc1", 1500, 3400, AXP2101_DC1OUT_VOL, 0x7f, AXP2101_OUTPUT_CTL0, 0, 0,
	{ {1500, 3400, 100}, } },

	{ "dcdc2", 500, 1540, AXP2101_DC2OUT_VOL, 0x7f, AXP2101_OUTPUT_CTL0, 1, 0,
	{ {500, 1200, 10}, {1220, 1540, 20}, } },

	{ "dcdc3", 500, 3400, AXP2101_DC3OUT_VOL, 0x7f, AXP2101_OUTPUT_CTL0, 2, 0,
	{ {500, 1200, 10}, {1220, 1540, 20}, {1600, 3400, 100}, } },

	{ "dcdc4", 500, 1840, AXP2101_DC4OUT_VOL, 0x7f, AXP2101_OUTPUT_CTL0, 3, 0,
	{ {500, 1200, 10}, {1220, 1840, 20}, } },

	{ "dcdc5", 1200, 3700, AXP2101_DC5OUT_VOL, 0x7f, AXP2101_OUTPUT_CTL0, 4, 0,
	{ {1400, 3700, 100}, } },

	{ "aldo1", 500, 3500, AXP2101_ALDO1OUT_VOL, 0x1f, AXP2101_OUTPUT_CTL2, 0, 0,
	{ {500, 3500, 100}, } },

	{ "aldo2", 500, 3500, AXP2101_ALDO2OUT_VOL, 0x1f, AXP2101_OUTPUT_CTL2, 1, 0,
	{ {500, 3500, 100}, } },

	{ "aldo3", 500, 3500, AXP2101_ALDO3OUT_VOL, 0x1f, AXP2101_OUTPUT_CTL2, 2, 0,
	{ {500, 3500, 100}, } },

	{ "aldo4", 500, 3500, AXP2101_ALDO4OUT_VOL, 0x1f, AXP2101_OUTPUT_CTL2, 3, 0,
	{ {500, 3500, 100}, } },

	{ "bldo1", 500, 3500, AXP2101_BLDO1OUT_VOL, 0x1f, AXP2101_OUTPUT_CTL2, 4, 0,
	{ {500, 3500, 100}, } },

	{ "bldo2", 500, 3500, AXP2101_BLDO2OUT_VOL, 0x1f, AXP2101_OUTPUT_CTL2, 5, 0,
	{ {500, 3500, 100}, } },

	{ "dldo1", 500, 3300, AXP2101_DLDO1OUT_VOL, 0x1f, AXP2101_OUTPUT_CTL2, 7, 0,
	{ {500, 3300, 100}, } },

	{ "dldo2", 500, 1400, AXP2101_DLDO2OUT_VOL, 0x1f, AXP2101_OUTPUT_CTL3, 0, 0,
	{ {500, 1400, 50}, } },


};
#define PMU_POWER_KEY_STATUS AXP2101_INTSTS1
#define PMU_POWER_KEY_OFFSET 0x2

static inline void disable_dcdc_pfm_mode(void)
{
	uint8_t val;

	axp210x_reg_read(AXP2101_RUNTIME_ADDR, 0x80, &val);
	val |= (0x01 << 3); /*dcdc4 for gpu pwm mode*/
	val |= (0x01 << 4); /*dcdc5 for dram pwm mode*/
	axp210x_reg_write(AXP2101_RUNTIME_ADDR, 0x80, val);

	/* disable dcm mode for GPU stability Vdrop issue*/
	axp210x_reg_write(AXP2101_RUNTIME_ADDR, 0xff, 0x0);
	axp210x_reg_write(AXP2101_RUNTIME_ADDR, 0xf4, 0x6);
	axp210x_reg_write(AXP2101_RUNTIME_ADDR, 0xf2, 0x4);
	axp210x_reg_write(AXP2101_RUNTIME_ADDR, 0xf5, 0x4);
	axp210x_reg_write(AXP2101_RUNTIME_ADDR, 0xff, 0x1);
	axp210x_reg_write(AXP2101_RUNTIME_ADDR, 0x12, 0x40);
	axp210x_reg_write(AXP2101_RUNTIME_ADDR, 0xff, 0x0);
}

int axp2101_probe_power_key(void)
{
	uint8_t reg_value;

	if (axp210x_reg_read(AXP2101_RUNTIME_ADDR, PMU_POWER_KEY_STATUS,
			  &reg_value)) {
		return -1;
	}
	/* POKLIRQ,POKSIRQ */
	reg_value &= (0x03 << PMU_POWER_KEY_OFFSET);
	if (reg_value) {
		if (axp210x_reg_write(AXP2101_RUNTIME_ADDR, PMU_POWER_KEY_STATUS,
				   reg_value)) {
			return -1;
		}
	}

	return (reg_value >> PMU_POWER_KEY_OFFSET) & 3;
}

static axp_contrl_info *get_ctrl_info_from_tbl(char *name)
{
	int i    = 0;
	int size = ARRAY_SIZE(axp_ctrl_tbl);
	for (i = 0; i < size; i++) {
		if (!strncmp(name, axp_ctrl_tbl[i].name,
			     strlen(axp_ctrl_tbl[i].name))) {
			break;
		}
	}
	if (i >= size) {
		return NULL;
	}
	return (axp_ctrl_tbl + i);
}

static int pmu_set_vol(char *name, int set_vol, int onoff)
{
	uint8_t reg_value, i;
	axp_contrl_info *p_item = NULL;
	uint8_t base_step		= 0;

	p_item = get_ctrl_info_from_tbl(name);
	if (!p_item) {
		return -1;
	}

	if ((set_vol > 0) && (p_item->min_vol)) {
		if (set_vol < p_item->min_vol) {
			set_vol = p_item->min_vol;
		} else if (set_vol > p_item->max_vol) {
			set_vol = p_item->max_vol;
		}
		if (axp210x_reg_read(AXP2101_RUNTIME_ADDR, p_item->cfg_reg_addr,
				  &reg_value)) {
			return -1;
		}

		reg_value &= ~p_item->cfg_reg_mask;

		for (i = 0; p_item->axp_step_tbl[i].step_max_vol != 0; i++) {
			if ((set_vol > p_item->axp_step_tbl[i].step_max_vol) &&
				(set_vol < p_item->axp_step_tbl[i+1].step_min_vol)) {
				set_vol = p_item->axp_step_tbl[i].step_max_vol;
			}
			if (p_item->axp_step_tbl[i].step_max_vol >= set_vol) {
				reg_value |= ((base_step + ((set_vol - p_item->axp_step_tbl[i].step_min_vol)/
					p_item->axp_step_tbl[i].step_val)) << p_item->reg_addr_offest);
				if (p_item->axp_step_tbl[i].regation) {
					uint8_t reg_value_temp = (~reg_value & p_item->cfg_reg_mask);
					reg_value &= ~p_item->cfg_reg_mask;
					reg_value |= reg_value_temp;
				}
				break;
			} else {
				base_step += ((p_item->axp_step_tbl[i].step_max_vol -
					p_item->axp_step_tbl[i].step_min_vol + p_item->axp_step_tbl[i].step_val) /
					p_item->axp_step_tbl[i].step_val);
			}
		}

		if (axp210x_reg_write(AXP2101_RUNTIME_ADDR, p_item->cfg_reg_addr,
				   reg_value)) {
			return -1;
		}
	}

	if (onoff < 0) {
		return 0;
	}
	if (axp210x_reg_read(AXP2101_RUNTIME_ADDR, p_item->ctrl_reg_addr,
			  &reg_value)) {
		return -1;
	}
	if (onoff == 0) {
		reg_value &= ~(1 << p_item->ctrl_bit_ofs);
	} else {
		reg_value |= (1 << p_item->ctrl_bit_ofs);
	}
	if (axp210x_reg_write(AXP2101_RUNTIME_ADDR, p_item->ctrl_reg_addr,
			   reg_value)) {
		return -1;
	}
	return 0;

}

#ifndef CONFIG_DDR_VOL_NAME_EXT
#define CONFIG_DDR_VOL_NAME_EXT "dcdc4"
#endif
int axp2101_set_ddr_voltage(int set_vol, int onoff)
{
	return pmu_set_vol(CONFIG_DDR_VOL_NAME_EXT, set_vol, onoff);
}

int axp2101_set_efuse_voltage(int set_vol, int onoff)
{
	return pmu_set_vol("bldo1", set_vol, onoff);
}

int axp2101_set_pll_voltage(int set_vol, int onoff)
{
	return pmu_set_vol("dcdc2", set_vol, onoff);
}

int axp2101_set_sys_voltage(int set_vol, int onoff)
{
	return pmu_set_vol("dcdc3", set_vol, onoff);
}

int axp2101_set_vin_voltage(int onoff)
{
#if 0
	pmu_set_vol("aldo2", 1800, 1);
	pmu_set_vol("bldo2", 2800, 1);
	pmu_set_vol("dldo2", 1200, 1);
#else
	pmu_set_vol("dcdc1", 3300, onoff);
	pmu_set_vol("dcdc2", 900, onoff);
	pmu_set_vol("dcdc3", 900, onoff);
	pmu_set_vol("dcdc4", 1500, onoff);
	pmu_set_vol("dcdc5", 3700, onoff);
	//pmu_set_vol("dc4ldo", 1200, onoff);
	pmu_set_vol("bldo1", 1800, onoff);
	pmu_set_vol("bldo2", 2800, onoff);
	pmu_set_vol("aldo1", 1800, onoff);
	pmu_set_vol("aldo3", 3300, onoff);
	pmu_set_vol("dldo1", 3300, onoff);
	pmu_set_vol("dldo2", 1200, onoff);
#endif

	return 0;
}

int axp2101_ctrl_voltage(char *name, int set_vol, int onoff)
{
	return pmu_set_vol(name, set_vol, onoff);
}

int axp2101_get_poweron_source(void)
{
	uint8_t reg_value;
	axp210x_reg_read(AXP2101_RUNTIME_ADDR, AXP2101_PWRON_STATUS, &reg_value);
	return reg_value;
}


/*
 * pmu_type : 0x47 is the first version
 *            0x4a is the second version
 */
static int axp2101_set_necessary_reg(int pmu_type)
{
	uint8_t reg_value;

	/* limit charge current to 300mA */
	reg_value = 0x9;
	axp210x_reg_write(AXP2101_RUNTIME_ADDR, AXP2101_CHARGE1, reg_value);

	/* limit run current to 2A */
	reg_value = 0x5;
	axp210x_reg_write(AXP2101_RUNTIME_ADDR, AXP2101_VBUS_CUR_SET, reg_value);

	/*enable vbus adc channel*/
	if (pmu_type != AXP2101_CHIP_ID_B) {
		reg_value = 0x40;
		axp210x_reg_write(AXP2101_RUNTIME_ADDR, AXP2101_BAT_AVERVOL_H6, reg_value);
	}

	/*set dcdc1 & dcdc3 pwm mode*/
	axp210x_reg_read(AXP2101_RUNTIME_ADDR, AXP2101_OUTPUT_CTL1, &reg_value);
	reg_value |= ((1 << 2) | (1 << 4));
	axp210x_reg_write(AXP2101_RUNTIME_ADDR, AXP2101_OUTPUT_CTL1, reg_value);

	/*pmu disable soften3 signal*/
	if (pmu_type != AXP2101_CHIP_ID_B) {
		reg_value = 0x00;
		axp210x_reg_write(AXP2101_RUNTIME_ADDR, AXP2101_TWI_ADDR_EXT, reg_value);
		reg_value = 0x06;
		axp210x_reg_write(AXP2101_RUNTIME_ADDR, AXP2101_EFUS_OP_CFG, reg_value);
		reg_value = 0x04;
		axp210x_reg_write(AXP2101_RUNTIME_ADDR, AXP2101_EFREQ_CTRL, reg_value);
		reg_value = 0x01;
		axp210x_reg_write(AXP2101_RUNTIME_ADDR, AXP2101_TWI_ADDR_EXT, reg_value);
		reg_value = 0x30;
		axp210x_reg_write(AXP2101_RUNTIME_ADDR, AXP2101_SELLP_CFG, reg_value);
		reg_value = 0x00;
		axp210x_reg_write(AXP2101_RUNTIME_ADDR, AXP2101_TWI_ADDR_EXT, reg_value);
		axp210x_reg_write(AXP2101_RUNTIME_ADDR, AXP2101_EFREQ_CTRL, reg_value);
		axp210x_reg_write(AXP2101_RUNTIME_ADDR, AXP2101_EFUS_OP_CFG, reg_value);
	}

	/*pmu set vsys min*/
	axp210x_reg_read(AXP2101_RUNTIME_ADDR, AXP2101_VSYS_MIN, &reg_value);
	reg_value &= ~(0x7 << 4);
	axp210x_reg_write(AXP2101_RUNTIME_ADDR, AXP2101_VSYS_MIN, reg_value);

	/*pmu set vimdpm cfg*/
	axp210x_reg_read(AXP2101_RUNTIME_ADDR, AXP2101_VBUS_VOL_SET, &reg_value);
	reg_value &= ~(0xf << 0);
	axp210x_reg_write(AXP2101_RUNTIME_ADDR, AXP2101_VBUS_VOL_SET, reg_value);

	/*pmu reset enable*/
	axp210x_reg_read(AXP2101_RUNTIME_ADDR, AXP2101_OFF_CTL, &reg_value);
	reg_value |= (3 << 2);
	axp210x_reg_write(AXP2101_RUNTIME_ADDR, AXP2101_OFF_CTL, reg_value);

	/*pmu pwroff enable*/
	axp210x_reg_read(AXP2101_RUNTIME_ADDR, AXP2101_PWEON_PWEOFF_EN, &reg_value);
	reg_value |= (1 << 1);
	axp210x_reg_write(AXP2101_RUNTIME_ADDR, AXP2101_PWEON_PWEOFF_EN, reg_value);

	/*pmu dcdc1 pwroff enable*/
	axp210x_reg_read(AXP2101_RUNTIME_ADDR, AXP2101_DCDC_PWEOFF_EN, &reg_value);
	reg_value &= ~(1 << 0);
	axp210x_reg_write(AXP2101_RUNTIME_ADDR, AXP2101_DCDC_PWEOFF_EN, reg_value);

	return 0;
}

int axp2101_poweroff(void)
{
	uint8_t reg_value;
	axp210x_reg_read(AXP2101_RUNTIME_ADDR, AXP2101_OFF_CTL, &reg_value);
	reg_value |= (1 << 0);
	axp210x_reg_write(AXP2101_RUNTIME_ADDR, AXP2101_OFF_CTL, reg_value);
	return 0;
}

int axp2101_poweron(void)
{
	printf("poweron the pmu\n");
	axp210x_pwr_on_ctl(1);
	OS_MSleep(20);
	axp210x_pwr_on_ctl(0);
	return 0;
}


int axp2101_reg_read(uint8_t addr, uint8_t *val)
{
	return axp210x_reg_read(AXP2101_RUNTIME_ADDR, addr, val);
}

int axp2101_reg_write(uint8_t addr, uint8_t val)
{
	return axp210x_reg_write(AXP2101_RUNTIME_ADDR, addr, val);
}

int axp2101_axp_init(void)
{
	uint8_t pmu_type;

	if (axp210x_reg_read(AXP2101_RUNTIME_ADDR, AXP2101_VERSION, &pmu_type)) {
		pmu_err("bus read error\n");
		return -1;
	}
	printf("pmu id is 0x%x\n", pmu_type);

	pmu_type &= 0xCF;
	if (pmu_type == AXP2101_CHIP_ID || pmu_type == AXP2101_CHIP_ID_B) {
		/* pmu type AXP21 */
		printf("PMU: AXP21\n");
		axp2101_set_necessary_reg(pmu_type);
		//axp2101_set_vin_voltage();
		return AXP2101_CHIP_ID;
	}
	printf("unknow PMU\n");
	return -1;
}
