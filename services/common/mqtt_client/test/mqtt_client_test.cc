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
 * @file test_mqtt_client.cc
 * @brief unittest for mqtt client
 * @author fancyxu (fancyxu@tencent.com)
 * @version 1.0
 * @date 2021-07-07
 *
 * @par Change Log:
 * <table>
 * <tr><th>Date       <th>Version <th>Author    <th>Description
 * <tr><td>2021-07-07 <td>1.0     <td>fancyxu   <td>first commit
 * <tr><td>2021-07-08 <td>1.1     <td>fancyxu   <td>support tls test
 * <tr><td>2021-07-12 <td>1.1     <td>fancyxu   <td>fix connect twice in 5s error
 * </table>
 */

#include "mqtt_client_test.h"

namespace mqtt_client_unittest {

void MqttClientTest::SetUp() {
  LogHandleFunc func;
  func.log_malloc = HAL_Malloc;
  func.log_free = HAL_Free;
  func.log_get_current_time_str = HAL_Timer_Current;
  func.log_printf = HAL_Printf;
  func.log_handle = NULL;
  utils_log_init(func, LOG_LEVEL_DEBUG, 2048);

  ASSERT_EQ(HAL_GetDevInfo(reinterpret_cast<void *>(&device_info)), 0);

  MQTTInitParams init_params = DEFAULT_MQTT_INIT_PARAMS;
  init_params.device_info = &device_info;
  init_params.command_timeout = QCLOUD_IOT_MQTT_COMMAND_TIMEOUT;
  init_params.keep_alive_interval_ms = QCLOUD_IOT_MQTT_KEEP_ALIVE_INTERNAL;
  init_params.auto_connect_enable = 1;
  init_params.event_handle.h_fp = NULL;
  init_params.event_handle.context = NULL;

  HAL_SleepMs(5000);  // for iot hub can not connect twice in 5 s

  client = IOT_MQTT_Construct(&init_params);
  ASSERT_NE(client, nullptr);
}

void MqttClientTest::TearDown() {
  IOT_MQTT_Destroy(&client);
  utils_log_deinit();
}

}  // namespace mqtt_client_unittest
