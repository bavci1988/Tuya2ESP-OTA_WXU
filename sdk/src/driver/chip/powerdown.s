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

  .syntax unified
#ifdef CONFIG_CPU_CM33F
  .cpu cortex-m33
  .fpu softvfp
#elif (defined CONFIG_CPU_CM4F)
  .cpu cortex-m4
  .fpu softvfp
#else
  .cpu cortex-m3
#endif
  .thumb

  .thumb_func
  .section .text
  .type   __powerdown, %function
  .global __powerdown

__powerdown:
  .fnstart
  .cantunwind

  MRS R0, MSP
  ISB
  STR R0, [R1]

  MRS R0, PSP
  ISB
  STR R0, [R1, #4]

  MRS R0, PRIMASK
  STR R0, [R1, #12]

  MRS R0, FAULTMASK
  STR R0, [R1, #16]

  MRS R0, BASEPRI
  STR R0, [R1, #20]

  MRS R0, CONTROL
  STR R0, [R1, #24]

  LDR R0, =0x14
  LDR R1, =0xe000ed10
  STR R0, [R1]

  LDR R0, =0x429b0000
  LDR R1, =0x40040100
  ISB
  STR R0, [R1]

  LDR R1, =0x40040024
  ISB
  LDR R0, [R1]
  BIC R0, R0, #0x30000
  STR R0, [R1]

  LDR R1, =0x40040020
  ISB
  LDR R0, [R1]
  BIC R0, R0, #0x80000000
  STR R0, [R1]

  LDR R1, =0x40040034
  ISB
  LDR R0, =0x00030001
  STR R0, [R1]

  LDR R0, =0xe000e100
  LDR R1, [R0]
  LDR R0, =0xe000e180
  STR R1, [R0]

  LDR R0, =0xe000e104
  LDR R1, [R0]
  LDR R0, =0xe000e184
  STR R1, [R0]

__clr_nvic:

  LDR R0, =0xe000e200
  LDR R1, [R0]
  LDR R0, =0xe000e280
  STR R1, [R0]

  LDR R0, =0xe000e204
  LDR R1, [R0]
  LDR R0, =0xe000e284
  STR R1, [R0]

  DSB
  ISB
  NOP
  NOP
  NOP
  WFE

  LDR R0, =0xe000e200
  LDR R1, [R0]
  LDR R0, =0xe000e204
  LDR R2, [R0]
  ORR R1, R2
  CMP R1, #0
  BNE __clr_nvic
  ISB
  NOP
  NOP
  NOP

  WFE
  NOP


  .fnend
  .size __powerdown, .-__powerdown
