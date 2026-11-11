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

#include "common/cmd/cmd_util.h"
#include "common/framework/net_ctrl.h"
#include "net/wlan/wlan_ext_req.h"
#include "net/wlan/wlan_defs.h"
#include "net/wlan/wlan.h"
#include "cmd_axp.h"
#include "axp2101_api.h"
#include "xrlink/xrlink_io.h"
#include "pm/pm.h"
#include "axp210x_drv.h"

static enum cmd_status cmd_axp_init_exec(char *cmd)
{
	if (axp2101_axp_init()) {
		return CMD_STATUS_FAIL;
	} else {
		return CMD_STATUS_OK;
	};

}

static enum cmd_status cmd_axp_probe_power_key_exec(char *cmd)
{
	int ret = axp2101_probe_power_key();
	if (ret) {
		CMD_AXP_ERR("probe_power_key ret =%d \n", ret);
		return CMD_STATUS_FAIL;
	} else{
		return CMD_STATUS_OK;
	};

}

static enum cmd_status cmd_axp_open_all_power_exec(char *cmd)
{
	axp2101_poweron();
	return CMD_STATUS_OK;
}

static enum cmd_status cmd_axp_ps_off_exec(char *cmd)
{
	axp2101_poweroff();
	return CMD_STATUS_OK;
}


static enum cmd_status cmd_axp_close_all_power_exec(char *cmd)
{
	//axp2101_set_vin_voltage(0);
	axp2101_poweroff();
	return CMD_STATUS_OK;
}

#if 0
static enum cmd_status cmd_axp_ctrl_any_exec(char *cmd)
{
	axp2101_axp_init();
}
#endif

static enum cmd_status cmd_axp_ctrl_ddr_exec(char *cmd)
{
	if (axp2101_set_ddr_voltage(900, 1)) {
		return CMD_STATUS_FAIL;
	} else{
		return CMD_STATUS_OK;
	};
}

static enum cmd_status cmd_axp_ctrl_pll_exec(char *cmd)
{
	if (axp2101_set_pll_voltage(900, 1)) {
		return CMD_STATUS_FAIL;
	} else{
		return CMD_STATUS_OK;
	};
}

static enum cmd_status cmd_axp_ctrl_efuse_exec(char *cmd)
{
	if (axp2101_set_efuse_voltage(900, 1)) {
		return CMD_STATUS_FAIL;
	} else{
		return CMD_STATUS_OK;
	};
}

static enum cmd_status cmd_axp_ctrl_sys_exec(char *cmd)
{
	if (axp2101_set_sys_voltage(900, 1)) {
		return CMD_STATUS_FAIL;
	} else{
		return CMD_STATUS_OK;
	};
}

static enum cmd_status cmd_axp_initi2c_exec(char *cmd)
{
	if (axp210x_init()) {
		CMD_AXP_ERR("bus init error\n");
		return CMD_STATUS_FAIL;
	}
	return CMD_STATUS_OK;

}

static enum cmd_status cmd_axp_deiniti2c_exec(char *cmd)
{
	axp210x_deinit();
	return CMD_STATUS_OK;
}

static enum cmd_status cmd_axp_write_reg_exec(char *cmd)
{
	uint32_t cnt, reg, val;

	cnt = cmd_sscanf(cmd, "r=%x v=%x", &reg, &val);
	if (cnt != 2) {
		CMD_AXP_ERR("invalid param number %d\n", cnt);
		return CMD_STATUS_INVALID_ARG;
	}
	CMD_AXP_DBG("r=%d v=%d,\n", reg, val);
	if (axp210x_reg_write(AXP2101_RUNTIME_ADDR, reg, val)) {
		CMD_AXP_ERR("write error\n");
		return CMD_STATUS_FAIL;
	}

	return CMD_STATUS_OK;
}

static enum cmd_status cmd_axp_read_reg_exec(char *cmd)
{
	uint32_t cnt, reg;
	uint8_t val;

	cnt = cmd_sscanf(cmd, "r=%x", &reg);
	if (cnt != 1) {
		CMD_AXP_ERR("invalid param number %d\n", cnt);
		return CMD_STATUS_INVALID_ARG;
	}
	if (axp210x_reg_read(AXP2101_RUNTIME_ADDR, (uint8_t)reg, &val)) {
		CMD_AXP_ERR("bus read error\n");
		return CMD_STATUS_FAIL;
	}
	CMD_AXP_DBG("val = 0x%x\n", val);
	return CMD_STATUS_OK;
}

static const struct cmd_data g_axp_cmds[] = {
	{ "init",				cmd_axp_init_exec},
	{ "probe_power_key",	cmd_axp_probe_power_key_exec},
	{ "poweron",			cmd_axp_open_all_power_exec},
	{ "psoff",				cmd_axp_ps_off_exec},
	{ "poweroff",			cmd_axp_close_all_power_exec},
	{ "ctrl_ddr",			cmd_axp_ctrl_ddr_exec},
	{ "ctrl_pll",			cmd_axp_ctrl_pll_exec},
	{ "ctrl_efuse",			cmd_axp_ctrl_efuse_exec},
	{ "ctrl_sys",			cmd_axp_ctrl_sys_exec},
	{ "initi2c",			cmd_axp_initi2c_exec},
	{ "deiniti2c",			cmd_axp_deiniti2c_exec},
	{ "write_reg",			cmd_axp_write_reg_exec},
	{ "read_reg",			cmd_axp_read_reg_exec},

};

enum cmd_status cmd_axp_exec(char *cmd)
{
	return cmd_exec(cmd, g_axp_cmds, cmd_nitems(g_axp_cmds));
}
