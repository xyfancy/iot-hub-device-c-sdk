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
 * <tr><td>2021-07-18 <td>1.2     <td>fancyxu   <td>remove MqttClientTest for common use
 * </table>
 */

#include <iostream>
#include <string>

#include "mqtt_client_test.h"

namespace mqtt_client_unittest {

/**
 * @brief Test subscribe.
 *
 */
TEST_F(MqttClientTest, subscribe) {
  int wait_cnt;
  char topic_name[MAX_SIZE_OF_CLOUD_TOPIC] = {0};
  HAL_Snprintf(topic_name, sizeof(topic_name), "%s/%s/data", device_info.product_id, device_info.device_name);

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
  char topic_name[MAX_SIZE_OF_CLOUD_TOPIC] = {0};
  HAL_Snprintf(topic_name, sizeof(topic_name), "%s/%s/data", device_info.product_id, device_info.device_name);

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
