/**
 * @copyright
 *
 * Tencent is pleased to support the open source community by making IoT Hub available.
 * Copyright(C) 2018 - 2021 THL A29 Limited, a Tencent company.All rights reserved.
 *
 * Licensed under the MIT License(the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://opensource.org/licenses/MIT
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the License is
 * distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * @file qcloud_iot_platform.h
 * @brief hal interface
 * @author fancyxu (fancyxu@tencent.com)
 * @version 1.0
 * @date 2021-05-28
 *
 * @par Change Log:
 * <table>
 * <tr><th>Date       <th>Version <th>Author    <th>Description
 * <tr><td>2021-05-28 <td>1.0     <td>fancyxu   <td>first commit
 * </table>
 */

#ifndef QCLOUD_IOT_HUB_SDK_EMBEDDED_C_INCLUDE_COMMON_QCLOUD_IOT_PLATFORM_H_
#define QCLOUD_IOT_HUB_SDK_EMBEDDED_C_INCLUDE_COMMON_QCLOUD_IOT_PLATFORM_H_

#if defined(__cplusplus)
extern "C" {
#endif

#include <inttypes.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "qcloud_iot_hub.h"

/**********************************************************************
 * QCloud IoT C-SDK Hardware Abstraction Layer
 * Platform/OS/IP stack/SSL dependant functions
 * Check platform folder for reference implementation
 * Require porting when adapt SDK to new platform/OS
 *********************************************************************/

/**************************************************************************************
 * os
 **************************************************************************************/

/**
 * @brief Mutex create.
 *
 * @return pointer to mutex
 */
void *HAL_MutexCreate(void);

/**
 * @brief Mutex destroy.
 *
 * @param[in,out] mutex pointer to mutex
 */
void HAL_MutexDestroy(void *mutex);

/**
 * @brief Mutex lock.
 *
 * @param[in,out] mutex pointer to mutex
 */
void HAL_MutexLock(void *mutex);

/**
 * @brief Mutex try lock.
 *
 * @param[in,out] mutex pointer to mutex
 * @return 0 for success
 */
int HAL_MutexTryLock(void *mutex);

/**
 * @brief Mutex unlock.
 *
 * @param[in,out] mutex pointer to mutex
 */
void HAL_MutexUnlock(void *mutex);

/**
 * @brief Malloc from heap.
 *
 * @param[in] size size to malloc
 * @return pointer to buffer, NULL for failed.
 */
void *HAL_Malloc(size_t size);

/**
 * @brief Free buffer malloced by HAL_Malloc.
 *
 * @param[in] ptr
 */
void HAL_Free(void *ptr);

/**
 * @brief Printf with format.
 *
 * @param[in] fmt format
 */
void HAL_Printf(const char *fmt, ...);

/**
 * @brief Snprintf with format.
 *
 * @param[out] str buffer to save
 * @param[in] len buffer len
 * @param[in] fmt format
 * @return length of formatted string, >0 for success.
 */
int HAL_Snprintf(char *str, const int len, const char *fmt, ...);

/**
 * @brief Get utc time ms timestamp.
 *
 * @return timestamp
 */
uint32_t HAL_GetTimeMs(void);

/**
 * @brief Sleep for ms.
 *
 * @param[in] ms ms to sleep
 */
void HAL_SleepMs(uint32_t ms);

#ifdef MULTITHREAD_ENABLED

/**
 * @brief Theard entry function.
 *
 */
typedef void (*ThreadRunFunc)(void *arg);

/**
 * @brief Thread params to create.
 *
 */
typedef struct {
    char *        thread_name; /**< thread name */
    uint32_t      thread_id;   /**< thread handle */
    ThreadRunFunc thread_func; /**< thread entry function */
    void *        user_arg;    /**< thread entry arg */
    uint16_t      priority;    /**< thread priority */
    uint32_t      stack_size;  /**< thread stack size */
} ThreadParams;

/**
 * @brief platform-dependant thread create function
 *
 * @param[in,out] params params to create thread @see ThreadParams
 * @return @see IoT_Return_Code
 */
int HAL_ThreadCreate(ThreadParams *params);

/**
 * @brief No use.
 *
 */
void HAL_ThreadExit(void);

#endif

/**************************************************************************************
 * device info
 **************************************************************************************/

/**
 * @brief Save device info.
 *
 * @param[in] dev_info device info to be saved
 * @return @see IoT_Return_Code
 */
int HAL_SetDevInfo(void *dev_info);

/**
 * @brief Get device info.
 *
 * @param[in] dev_info buffer to save device info
 * @return @see IoT_Return_Code
 */
int HAL_GetDevInfo(void *dev_info);

/**************************************************************************************
 * timer
 **************************************************************************************/

/**
 * @brief Define timer structure, platform dependant.
 *
 */
typedef struct {
#if defined(__linux__) && defined(__GLIBC__)
    struct timeval end_time;
#else
    uintptr_t end_time;
#endif
} Timer;

/**
 * @brief Return if timer expired.
 *
 * @param[in] timer @see Timer
 * @return true expired
 * @return false no expired
 */
bool HAL_Timer_expired(Timer *timer);

/**
 * @brief Countdown ms.
 *
 * @param[in,out] timer @see Timer
 * @param[in] timeout_ms ms to count down
 */
void HAL_Timer_countdown_ms(Timer *timer, unsigned int timeout_ms);

/**
 * @brief Countdown second
 *
 * @param[in,out] timer @see Timer
 * @param[in] timeout second to count down
 */
void HAL_Timer_countdown(Timer *timer, unsigned int timeout);

/**
 * @brief Timer remain ms.
 *
 * @param[in] timer @see Timer
 * @return ms
 */
int HAL_Timer_remain(Timer *timer);

/**
 * @brief time format string
 *
 * @return time format string, such as "2021-05-31 15:58:46"
 */
char *HAL_Timer_current(void);

/**
 * @brief Get current utf timestamp of second
 *
 * @return timestamp
 */
int HAL_Timer_current_sec(void);

/**
 * @brief Set system time using ms timestamp
 *
 * @param[in] timestamp_sec timestamp to set
 * @return 0 for success
 */
int HAL_Timer_set_systime_sec(size_t timestamp_sec);

/**
 * @brief Set system time using second timestamp
 *
 * @param[in] timestamp_ms
 * @return 0 for success
 */
int HAL_Timer_set_systime_ms(size_t timestamp_ms);

/**************************************************************************************
 * network
 **************************************************************************************/

/**
 * @brief TCP connect in linux
 *
 * @param[in] host host to connect
 * @param[out] port port to connect
 * @return socket fd
 */
uintptr_t HAL_TCP_Connect(const char *host, uint16_t port);

/**
 * @brief TCP disconnect
 *
 * @param[in] fd socket fd
 * @return 0 for success
 */
int HAL_TCP_Disconnect(uintptr_t fd);

/**
 * @brief TCP write
 *
 * @param[in] fd socket fd
 * @param[in] buf buf to write
 * @param[in] len buf len
 * @param[in] timeout_ms timeout
 * @param[out] written_len data written length
 * @return @see IoT_Return_Code
 */
int HAL_TCP_Write(uintptr_t fd, const unsigned char *data, uint32_t len, uint32_t timeout_ms, size_t *written_len);

/**
 * @brief TCP read.
 *
 * @param[in] fd socket fd
 * @param[out] buf buffer to save read data
 * @param[in] len buffer len
 * @param[in] timeout_ms timeout
 * @param[out] read_len length of data read
 * @return @see IoT_Return_Code
 */
int HAL_TCP_Read(uintptr_t fd, unsigned char *data, uint32_t len, uint32_t timeout_ms, size_t *read_len);

#if defined(__cplusplus)
}
#endif

#endif  // QCLOUD_IOT_HUB_SDK_EMBEDDED_C_INCLUDE_COMMON_QCLOUD_IOT_PLATFORM_H_
