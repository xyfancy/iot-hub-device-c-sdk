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

#include <iostream>
#include <string>

#include "gtest/gtest.h"
#include "qcloud_iot_hub.h"

namespace mqtt_client_unittest {
/**
 * @brief test fixture of mqtt client
 *
 */
class MqttClientTest : public testing::Test {
 protected:
  void SetUp() override {
    LogHandleFunc func;
    func.log_malloc = HAL_Malloc;
    func.log_free = HAL_Free;
    func.log_get_current_time_str = HAL_Timer_current;
    func.log_printf = HAL_Printf;
    func.log_handle = NULL;
    utils_log_init(func, eLOG_DEBUG, 2048);

    ASSERT_EQ(HAL_GetDevInfo(reinterpret_cast<void *>(&device_info)), 0);

    HAL_Snprintf(topic_name, sizeof(topic_name), "%s/%s/data", device_info.product_id, device_info.device_name);

    MQTTInitParams init_params = DEFAULT_MQTT_INIT_PARAMS;
    init_params.device_info = &device_info;
#ifdef AUTH_WITH_NO_TLS
    init_params.host = "localhost";
#endif
    init_params.command_timeout = QCLOUD_IOT_MQTT_COMMAND_TIMEOUT;
    init_params.keep_alive_interval_ms = QCLOUD_IOT_MQTT_KEEP_ALIVE_INTERNAL;
    init_params.auto_connect_enable = 1;
    init_params.event_handle.h_fp = NULL;
    init_params.event_handle.context = NULL;

    HAL_SleepMs(5000);  // for iot hub can not connect twice in 5 s

    client = IOT_MQTT_Construct(&init_params);
    ASSERT_NE(client, nullptr);
  }

  void TearDown() override {
    IOT_MQTT_Destroy(&client);
    utils_log_deinit();
  }

  void *client = NULL;
  DeviceInfo device_info;
  char topic_name[128] = {0};
};

/**
 * @brief Test subscribe.
 *
 */
TEST_F(MqttClientTest, subscribe) {
  int wait_cnt;
  SubscribeParams sub_params = DEFAULT_SUB_PARAMS;

  /**
   * @brief QOS0
   *
   */
  wait_cnt = 10;
  sub_params.qos = QOS0;
  ASSERT_GE(IOT_MQTT_Subscribe(client, topic_name, &sub_params), 0);
  while (!IOT_MQTT_IsSubReady(client, topic_name) && (wait_cnt > 0)) {
    ASSERT_EQ(IOT_MQTT_Yield(client, 500), 0);
    wait_cnt--;
  }
  ASSERT_NE(wait_cnt, 0);

  ASSERT_GE(IOT_MQTT_Unsubscribe(client, topic_name), 0);
  ASSERT_EQ(IOT_MQTT_Yield(client, 500), 0);

  /**
   * @brief QOS1
   *
   */
  wait_cnt = 10;
  sub_params.qos = QOS1;
  ASSERT_GE(IOT_MQTT_Subscribe(client, topic_name, &sub_params), 0);
  while (!IOT_MQTT_IsSubReady(client, topic_name) && (wait_cnt > 0)) {
    ASSERT_EQ(IOT_MQTT_Yield(client, 500), 0);
    wait_cnt--;
  }
  ASSERT_NE(wait_cnt, 0);

  ASSERT_GE(IOT_MQTT_Unsubscribe(client, topic_name), 0);
  ASSERT_EQ(IOT_MQTT_Yield(client, 500), 0);
}

/**
 * @brief Test publish.
 *
 */
TEST_F(MqttClientTest, publish) {
  char topic_content[] = "{\"action\": \"publish_test\", \"count\": \"0\"}";
  PublishParams pub_params = DEFAULT_PUB_PARAMS;

  pub_params.payload = topic_content;
  pub_params.payload_len = strlen(topic_content);
  /**
   * @brief QOS0
   *
   */
  pub_params.qos = QOS0;
  ASSERT_GE(IOT_MQTT_Publish(client, topic_name, &pub_params), 0);

  /**
   * @brief QOS1
   *
   */
  pub_params.qos = QOS1;
  ASSERT_GE(IOT_MQTT_Publish(client, topic_name, &pub_params), 0);
}

}  // namespace mqtt_client_unittest
