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
 * @file HAL_OS_linux.c
 * @brief Linux os api
 * @author fancyxu (fancyxu@tencent.com)
 * @version 1.0
 * @date 2021-05-31
 *
 * @par Change Log:
 * <table>
 * <tr><th>Date       <th>Version <th>Author    <th>Description
 * <tr><td>2021-05-31 <td>1.0     <td>fancyxu   <td>first commit
 * </table>
 */

#include <errno.h>
#include <memory.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "qcloud_iot_platform.h"

/**
 * @brief Mutex create.
 *
 * @return pointer to mutex
 */
void *HAL_MutexCreate(void)
{
#ifdef MULTITHREAD_ENABLED
    int              err_num;
    pthread_mutex_t *mutex = (pthread_mutex_t *)HAL_Malloc(sizeof(pthread_mutex_t));
    if (NULL == mutex) {
        return NULL;
    }

    if (0 != (err_num = pthread_mutex_init(mutex, NULL))) {
        HAL_Printf("%s: create mutex failed\n", __FUNCTION__);
        HAL_Free(mutex);
        return NULL;
    }

    return mutex;
#else
    return (void *)0xFFFFFFFF;
#endif
}

/**
 * @brief Mutex destroy.
 *
 * @param[in,out] mutex pointer to mutex
 */
void HAL_MutexDestroy(void *mutex)
{
    if (!mutex) {
        return;
    }

#ifdef MULTITHREAD_ENABLED
    int err_num;
    if (0 != (err_num = pthread_mutex_destroy((pthread_mutex_t *)mutex))) {
        HAL_Printf("%s: destroy mutex failed\n", __FUNCTION__);
    }

    HAL_Free(mutex);
#else
    return;
#endif
}

/**
 * @brief Mutex lock.
 *
 * @param[in,out] mutex pointer to mutex
 */
void HAL_MutexLock(void *mutex)
{
#ifdef MULTITHREAD_ENABLED
    int err_num;
    if (0 != (err_num = pthread_mutex_lock((pthread_mutex_t *)mutex))) {
        HAL_Printf("%s: lock mutex failed\n", __FUNCTION__);
    }
#else
    return;
#endif
}

/**
 * @brief Mutex try lock.
 *
 * @param[in,out] mutex pointer to mutex
 * @return 0 for success
 */
int HAL_MutexTryLock(void *mutex)
{
#ifdef MULTITHREAD_ENABLED
    return pthread_mutex_trylock((pthread_mutex_t *)mutex);
#else
    return 0;
#endif
}

/**
 * @brief Mutex unlock.
 *
 * @param[in,out] mutex pointer to mutex
 */
void HAL_MutexUnlock(void *mutex)
{
#ifdef MULTITHREAD_ENABLED
    int err_num;
    if (0 != (err_num = pthread_mutex_unlock((pthread_mutex_t *)mutex))) {
        HAL_Printf("%s: unlock mutex failed\n", __FUNCTION__);
    }
#else
    return;
#endif
}

/**
 * @brief Malloc from heap.
 *
 * @param[in] size size to malloc
 * @return pointer to buffer, NULL for failed.
 */
void *HAL_Malloc(size_t size)
{
    return malloc(size);
}

/**
 * @brief Free buffer malloced by HAL_Malloc.
 *
 * @param[in] ptr
 */
void HAL_Free(void *ptr)
{
    if (ptr)
        free(ptr);
}

/**
 * @brief Printf with format.
 *
 * @param[in] fmt format
 */
void HAL_Printf(const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);

    fflush(stdout);
}

/**
 * @brief Snprintf with format.
 *
 * @param[out] str buffer to save
 * @param[in] len buffer len
 * @param[in] fmt format
 * @return length of formatted string, >0 for success.
 */
int HAL_Snprintf(char *str, const int len, const char *fmt, ...)
{
    va_list args;
    int     rc;

    va_start(args, fmt);
    rc = vsnprintf(str, len, fmt, args);
    va_end(args);

    return rc;
}

/**
 * @brief Get utc time ms timestamp.
 *
 * @return timestamp
 */
uint32_t HAL_GetTimeMs(void)
{
    struct timeval time_val = {0};
    uint32_t       time_ms;

    gettimeofday(&time_val, NULL);
    time_ms = time_val.tv_sec * 1000 + time_val.tv_usec / 1000;

    return time_ms;
}

/**
 * @brief Sleep for ms
 *
 * @param[in] ms ms to sleep
 */
void HAL_SleepMs(uint32_t ms)
{
    usleep(1000 * ms);
}

#ifdef MULTITHREAD_ENABLED

/**
 * @brief platform-dependant thread routine/entry function
 *
 * @param[in,out] ptr
 * @return NULL
 */
static void *_HAL_thread_func_wrapper_(void *ptr)
{
    ThreadParams *params = (ThreadParams *)ptr;

    params->thread_func(params->user_arg);

    pthread_detach(pthread_self());
    pthread_exit(0);
    return NULL;
}

/**
 * @brief platform-dependant thread create function
 *
 * @param[in,out] params params to create thread @see ThreadParams
 * @return @see IoT_Return_Code
 */
int HAL_ThreadCreate(ThreadParams *params)
{
    if (params == NULL)
        return QCLOUD_ERR_INVAL;

    int ret = pthread_create((pthread_t *)&params->thread_id, NULL, _HAL_thread_func_wrapper_, (void *)params);
    if (ret) {
        HAL_Printf("%s: pthread_create failed: %d\n", __FUNCTION__, ret);
        return QCLOUD_ERR_FAILURE;
    }

    return QCLOUD_RET_SUCCESS;
}

/**
 * @brief No use.
 *
 */
void HAL_ThreadExit(void) {}

#endif
