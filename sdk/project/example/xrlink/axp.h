/*
 * (C) Copyright 2018-2020
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * wangwei <wangwei@allwinnertech.com>
 * axp.h
 */

#ifndef _AXP_H_
#define _AXP_H_


#define pmu_err(format, arg...) printf("[pmu]: " format, ##arg)
#define pmu_info(format, arg...) /*printf("[pmu]: "format,##arg)*/

typedef struct _axp_step_info {
	unsigned int step_min_vol;
	unsigned int step_max_vol;
	unsigned int step_val;
	unsigned int regation;
} _axp_step_info;

typedef struct _axp_contrl_info {
	char name[16];

	unsigned int min_vol;
	unsigned int max_vol;
	unsigned int cfg_reg_addr;
	unsigned int cfg_reg_mask;
	unsigned int ctrl_reg_addr;
	unsigned int ctrl_bit_ofs;
	unsigned int reg_addr_offest;
	_axp_step_info axp_step_tbl[4];

} axp_contrl_info;


#define AXP_BOOT_SOURCE_BUTTON         0
#define AXP_BOOT_SOURCE_IRQ_LOW                1
#define AXP_BOOT_SOURCE_VBUS_USB       2
#define AXP_BOOT_SOURCE_CHARGER                3
#define AXP_BOOT_SOURCE_BATTERY                4

#if defined(CFG_qg3101) && (CFG_LICHEE_BOARD == CFG_ft)
#define CONFIG_DDR_VOL_NAME_EXT		"aldo1"
#endif

#endif
