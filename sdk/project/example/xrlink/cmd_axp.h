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

#ifndef _CMD_AXP_H_
#define _CMD_AXP_H_

#ifdef __cplusplus
extern "C" {
#endif

#define CMD_AXP_DBG_ON		0
#define CMD_AXP_WRN_ON		0
#define CMD_AXP_ERR_ON		1
#define CMD_AXP_INF_ON		1

#define CMD_AXP_SYSLOG		printf

#define CMD_AXP_LOG(flags, fmt, arg...)	\
    do {									\
        if (flags)							\
           CMD_AXP_SYSLOG(fmt, ##arg);	\
    } while (0)

#define CMD_AXP_INF(fmt, arg...)	\
    CMD_AXP_LOG(CMD_AXP_INF_ON, "[CMD_AXP DBG] "fmt, ##arg)

#define CMD_AXP_DBG(fmt, arg...)	\
    CMD_AXP_LOG(CMD_AXP_DBG_ON, "[CMD_AXP DBG] "fmt, ##arg)

#define CMD_AXP_WRN(fmt, arg...)	\
    CMD_AXP_LOG(CMD_AXP_WRN_ON, "[CMD_AXP WRN] "fmt, ##arg)

#define CMD_AXP_ERR(fmt, arg...)										\
    do {																\
		CMD_AXP_LOG(CMD_AXP_ERR_ON, "[CMD_AXP ERR] %s():%d, "fmt,	\
               __func__, __LINE__, ##arg);								\
    } while (0)



enum cmd_status cmd_axp_exec(char *cmd);

#ifdef __cplusplus
}
#endif

#endif /* _CMD_ADC_H_ */
