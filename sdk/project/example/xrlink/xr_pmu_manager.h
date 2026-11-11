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

#ifndef _CMD_XR_PMU_MANAGER_H_
#define _CMD_XR_MPU_MANAGER_H_

#ifdef __cplusplus
extern "C" {
#endif

#define PMU_MANAGER_DBG_ON		0
#define PMU_MANAGER_WRN_ON		0
#define PMU_MANAGER_ERR_ON		1
#define PMU_MANAGER_INF_ON		1

#define PMU_MANAGER_SYSLOG		printf

#define PMU_MANAGER_LOG(flags, fmt, arg...)	\
    do {									\
        if (flags)							\
           PMU_MANAGER_SYSLOG(fmt, ##arg);	\
    } while (0)

#define PMU_MANAGER_INF(fmt, arg...)	\
    PMU_MANAGER_LOG(PMU_MANAGER_INF_ON, "[PMU_MANAGER DBG] "fmt, ##arg)

#define PMU_MANAGER_DBG(fmt, arg...)	\
    PMU_MANAGER_LOG(PMU_MANAGER_DBG_ON, "[PMU_MANAGER DBG] "fmt, ##arg)

#define PMU_MANAGER_WRN(fmt, arg...)	\
    PMU_MANAGER_LOG(PMU_MANAGER_WRN_ON, "[PMU_MANAGER WRN] "fmt, ##arg)

#define PMU_MANAGER_ERR(fmt, arg...)										\
    do {																\
		PMU_MANAGER_LOG(PMU_MANAGER_ERR_ON, "[PMU_MANAGER ERR] %s():%d, "fmt,	\
               __func__, __LINE__, ##arg);								\
    } while (0)


typedef enum {
	POWER_ON_PMU,
	ENTER_STANDBY,
	ENTER_HIBERNATION,
} PMU_Manager;

void wlan_ring_wakeup_init(void);
void xrlink_pmu_mamger_init(void);
int xr_pmu_ctrl(unsigned char type);

#ifdef __cplusplus
}
#endif

#endif /* _CMD_ADC_H_ */
