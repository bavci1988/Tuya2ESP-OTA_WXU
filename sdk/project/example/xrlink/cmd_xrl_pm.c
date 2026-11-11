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
#include "cmd_xrl_pm.h"
#include "axp2101_api.h"
#include "xrlink/xrlink_io.h"
#include "pm/pm.h"
#include "xr_pmu_manager.h"

static enum cmd_status cmd_xrl_pm_standby_exec(char *cmd)
{
	xr_pmu_ctrl(ENTER_STANDBY);
	return CMD_STATUS_OK;
}

static enum cmd_status cmd_xrl_pm_hibernation_exec(char *cmd)
{
	xr_pmu_ctrl(ENTER_HIBERNATION);
	return CMD_STATUS_OK;
}

static enum cmd_status cmd_xrl_pm_lowpower_exec(char *cmd)
{
	uint32_t dtim;
	int ret;
	uint32_t cnt;

	cnt = cmd_sscanf(cmd, "d=%d", &dtim);
	if (cnt != 1) {
		printf("invalid param number %d\n", cnt);
		return CMD_STATUS_INVALID_ARG;
	}
	printf("dtim = %d\n", dtim);
	if (dtim > 30) {
		printf("dtim=%d parameters are out of rang\n", dtim);
	}
	ret = wlan_ext_low_power_param_set_default(dtim);
	if (ret) {
	  printf("fail to set pre_rx_bcn!\n");
	  return CMD_STATUS_INVALID_ARG;
	}
	return CMD_STATUS_OK;

}

static const struct cmd_data g_xrl_pm_cmds[] = {
	{ "standby",		cmd_xrl_pm_standby_exec},
	{ "hibernation",	cmd_xrl_pm_hibernation_exec},
	{ "set_lowpower",	cmd_xrl_pm_lowpower_exec},

};

enum cmd_status cmd_xrl_pm_exec(char *cmd)
{
	return cmd_exec(cmd, g_xrl_pm_cmds, cmd_nitems(g_xrl_pm_cmds));
}
