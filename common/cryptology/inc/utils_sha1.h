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
 * @file utils_sha1.h
 * @brief header file for utils-sha1
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

#ifndef IOT_HUB_DEVICE_C_SDK_COMMON_CRYPTOLOGY_INC_UTILS_SHA1_H_
#define IOT_HUB_DEVICE_C_SDK_COMMON_CRYPTOLOGY_INC_UTILS_SHA1_H_

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief SHA-1 context structure.
 *
 */
typedef struct {
    uint32_t      total[2];   /**< number of bytes processed  */
    uint32_t      state[5];   /**< intermediate digest state  */
    unsigned char buffer[64]; /**< data block being processed */
} iot_sha1_context;

/**
 * @brief Initialize SHA-1 context.
 *
 * @param[in,out] ctx SHA-1 context to be initialized
 */
void utils_sha1_init(iot_sha1_context *ctx);

/**
 * @brief Clear SHA-1 context.
 *
 * @param[in,out] ctx SHA-1 context to be cleared
 */
void utils_sha1_free(iot_sha1_context *ctx);

/**
 * @brief Clone (the state of) a SHA-1 context.
 *
 * @param[out] dst The destination context
 * @param[in] src The context to be cloned
 */
void utils_sha1_clone(iot_sha1_context *dst, const iot_sha1_context *src);

/**
 * @brief SHA-1 context setup
 *
 * @param[in,out] ctx context to be initialized
 */
void utils_sha1_starts(iot_sha1_context *ctx);

/**
 * @brief SHA-1 process buffer.
 *
 * @param[in,out] ctx SHA-1 context
 * @param[in] input buffer holding the data
 * @param[in] ilen length of the input data
 */
void utils_sha1_update(iot_sha1_context *ctx, const unsigned char *input, size_t ilen);

/**
 * @brief SHA-1 final digest
 *
 * @param[in,out] ctx SHA-1 context
 * @param[out] output SHA-1 checksum result
 */
void utils_sha1_finish(iot_sha1_context *ctx, unsigned char output[20]);

/**
 * @brief Output = SHA-1( input buffer )
 *
 * @param[in] input buffer holding the data
 * @param[in] ilen length of the input data
 * @param[out] output SHA-1 checksum result
 */
void utils_sha1(const unsigned char *input, size_t ilen, unsigned char output[20]);

#endif  // IOT_HUB_DEVICE_C_SDK_COMMON_CRYPTOLOGY_INC_UTILS_SHA1_H_
