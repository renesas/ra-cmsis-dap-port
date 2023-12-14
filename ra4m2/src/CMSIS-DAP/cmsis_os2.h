/*
 * Copyright (c) 2013-2020 Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * ----------------------------------------------------------------------
 *
 * $Date:        12. June 2020
 * $Revision:    V2.1.3
 *
 * Project:      CMSIS-RTOS2 API
 * Title:        cmsis_os2.h header file
 *
 * Version 2.1.3
 *    Additional functions allowed to be called from Interrupt Service Routines:
 *    - osThreadGetId
 * Version 2.1.2
 *    Additional functions allowed to be called from Interrupt Service Routines:
 *    - osKernelGetInfo, osKernelGetState
 * Version 2.1.1
 *    Additional functions allowed to be called from Interrupt Service Routines:
 *    - osKernelGetTickCount, osKernelGetTickFreq
 *    Changed Kernel Tick type to uint32_t:
 *    - updated: osKernelGetTickCount, osDelayUntil
 * Version 2.1.0
 *    Support for critical and uncritical sections (nesting safe):
 *    - updated: osKernelLock, osKernelUnlock
 *    - added: osKernelRestoreLock
 *    Updated Thread and Event Flags:
 *    - changed flags parameter and return type from int32_t to uint32_t
 * Version 2.0.0
 *    Initial Release
 *---------------------------------------------------------------------------*/

/* Reduced to a bare minimum required to get SWO.c to compile */

#define osThreadId_t int
#define osWaitForever 0xFFFFFFFFU
#define osFlagsWaitAny 0x00000000U

// Flags errors (returned by osThreadFlagsXxxx and osEventFlagsXxxx).
#define osFlagsError          0x80000000U ///< Error indicator.
#define osFlagsErrorUnknown   0xFFFFFFFFU ///< osError (-1).
#define osFlagsErrorTimeout   0xFFFFFFFEU ///< osErrorTimeout (-2).
#define osFlagsErrorResource  0xFFFFFFFDU ///< osErrorResource (-3).
#define osFlagsErrorParameter 0xFFFFFFFCU ///< osErrorParameter (-4).
#define osFlagsErrorISR       0xFFFFFFFAU ///< osErrorISR (-6).

enum osStatus_t
{
  osOK = 0,
  osError = -1,
  osErrorTimeout = -2,
  osErrorResource = -3,
  osErrorParameter = -4,
  osErrorNoMemory = -5,
  osErrorISR = -6,
  osStatusReserved = 0x7FFFFFFF
};

uint32_t osThreadFlagsSet(osThreadId_t thread_id, uint32_t flags);
uint32_t osThreadFlagsWait(uint32_t flags,
                           uint32_t options,
                           uint32_t timeout);
