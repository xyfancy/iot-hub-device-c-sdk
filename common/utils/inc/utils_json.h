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
 * @file utils_json.h
 * @brief
 * @author fancyxu (fancyxu@tencent.com)
 * @version 1.0
 * @date 2021-07-24
 *
 * @par Change Log:
 * <table>
 * <tr><th>Date       <th>Version <th>Author    <th>Description
 * <tr><td>2021-07-24 <td>1.0     <td>fancyxu   <td>first commit
 * </table>
 */

#ifndef IOT_HUB_DEVICE_C_SDK_COMMON_UTILS_INC_UTILS_JSON_H_
#define IOT_HUB_DEVICE_C_SDK_COMMON_UTILS_INC_UTILS_JSON_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

/**
 * @brief Json value
 *
 */
typedef struct {
    const char *value;
    int         value_len;
} UtilsJsonValue;

/**
 * @brief Get value from json string. Not strict, just for iot scene, we suppose all the string is valid json.
 *
 * @param[in] key key in json, support nesting with '.'
 * @param[in] key_len key len
 * @param[in] src json string
 * @param[in] src_len src length
 * @param[out] value value
 * @return 0 for success
 */
int utils_json_value_get(const char *key, int key_len, const char *src, int src_len, UtilsJsonValue *value);

#ifdef __cplusplus
}
#endif

#endif  // IOT_HUB_DEVICE_C_SDK_COMMON_UTILS_INC_UTILS_JSON_H_
