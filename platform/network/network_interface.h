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
 * @file network_interface.h
 * @brief header for network interface
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

#ifndef IOT_HUB_DEVICE_C_SDK_PLATFORM_NETWORK_NETWORK_INTERFACE_H_
#define IOT_HUB_DEVICE_C_SDK_PLATFORM_NETWORK_NETWORK_INTERFACE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

#include "qcloud_iot_hub.h"

/**
 * @brief Type of network interface.
 *
 */
typedef enum { NETWORK_TCP = 0, NETWORK_UDP = 1, NETWORK_TLS = 2, NETWORK_DTLS = 3 } NETWORK_TYPE;

/**
 * @brief Define structure for network stack.
 *
 */
typedef struct Network Network;

/**
 * @brief Define structure for network stack.
 *
 * @note init/connect/read/write/disconnect/state
 *
 */
struct Network {
    int (*init)(Network *);

    int (*connect)(Network *);

    int (*read)(Network *, unsigned char *, size_t, uint32_t, size_t *);

    int (*write)(Network *, unsigned char *, size_t, uint32_t, size_t *);

    void (*disconnect)(Network *);

    int (*is_connected)(Network *);

    uintptr_t handle; /**< for non-AT: 0 = not connected, non-zero = connected for AT: 0 = valid connection,
                         MAX_UNSIGNED_INT = invalid*/

#ifndef AUTH_WITH_NOTLS
    SSLConnectParams ssl_connect_params;
#endif

    const char * host; /**< server address */
    int          port; /**< server port */
    NETWORK_TYPE type;
};

/**
 * @brief Init network, support tcp, tls(if AUTH_WITH_NOTLS defined).
 *
 * @param network pointer to network
 * @return @see IoT_Return_Code
 */
int network_init(Network *network);

#ifdef __cplusplus
}
#endif

#endif  // IOT_HUB_DEVICE_C_SDK_PLATFORM_NETWORK_NETWORK_INTERFACE_H_
