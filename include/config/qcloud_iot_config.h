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
 * @file qcloud_iot_config.h
 * @brief sdk config define
 * @author fancyxu (fancyxu@tencent.com)
 * @version 1.0
 * @date 2021-06-01
 *
 * @par Change Log:
 * <table>
 * <tr><th>Date       <th>Version <th>Author    <th>Description
 * <tr><td>2021-06-01 <td>1.0     <td>fancyxu   <td>first commit
 * <tr><td>2021-07-12 <td>1.1     <td>fancyxu   <td>rename AUTH_WITH_NOTLS to AUTH_WITH_NO_TLS
 * </table>
 */

#ifndef IOT_HUB_DEVICE_C_SDK_INCLUDE_CONFIG_QCLOUD_IOT_CONFIG_H_
#define IOT_HUB_DEVICE_C_SDK_INCLUDE_CONFIG_QCLOUD_IOT_CONFIG_H_

#ifdef __cplusplus
extern "C" {
#endif

/* #undef AUTH_MODE_CERT */
#define AUTH_MODE_KEY
/* #undef AUTH_WITH_NO_TLS */
/* #undef GATEWAY_ENABLED */
/* #undef COAP_COMM_ENABLED */
/* #undef OTA_MQTT_CHANNEL */
/* #undef SYSTEM_COMM */
/* #undef DEV_DYN_REG_ENABLED */
/* #undef LOG_UPLOAD */
/* #undef IOT_DEBUG */
#define DEBUG_DEV_INFO_USED
/* #undef AT_TCP_ENABLED */
/* #undef AT_UART_RECV_IRQ */
/* #undef AT_OS_USED */
/* #undef AT_DEBUG */
/* #undef OTA_USE_HTTPS */
/* #undef MULTITHREAD_ENABLED */
/* #undef BROADCAST_ENABLED */
/* #undef RRPC_ENABLED */
/* #undef REMOTE_CONFIG_MQTT */

#ifdef __cplusplus
}
#endif

#endif  // IOT_HUB_DEVICE_C_SDK_INCLUDE_CONFIG_QCLOUD_IOT_CONFIG_H_
