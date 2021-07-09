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
 * @file mqtt_sample.c
 * @brief a simple sample for mqtt client
 * @author fancyxu (fancyxu@tencent.com)
 * @version 1.0
 * @date 2021-05-31
 *
 * @par Change Log:
 * <table>
 * <tr><th>Date       <th>Version <th>Author    <th>Description
 * <tr><td>2021-05-31 <td>1.0     <td>fancyxu   <td>first commit
 * <tr><td>2021-07-08 <td>1.1     <td>fancyxu   <td>fix code standard of IotReturnCode and QcloudIotClient
 * </table>
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "qcloud_iot_hub.h"

#include "utils_log.h"

/**
 * @brief MQTT event callback, @see MQTTEventHandleFun
 *
 * @param[in] client pointer to mqtt client
 * @param[in] handle_context context
 * @param[in] msg msg
 */
static void _mqtt_event_handler(void *client, void *handle_context, MQTTEventMsg *msg)
{
    MQTTMessage *mqtt_message = (MQTTMessage *)msg->msg;
    uintptr_t    packet_id    = (uintptr_t)msg->msg;

    switch (msg->event_type) {
        case MQTT_EVENT_UNDEF:
            Log_i("undefined event occur.");
            break;

        case MQTT_EVENT_DISCONNECT:
            Log_i("MQTT disconnect.");
            break;

        case MQTT_EVENT_RECONNECT:
            Log_i("MQTT reconnect.");
            break;

        case MQTT_EVENT_PUBLISH_RECEIVED:
            Log_i("topic message arrived but without any related handle: topic=%.*s, topic_msg=%.*s",
                  mqtt_message->topic_len, STRING_PTR_PRINT_SANITY_CHECK(mqtt_message->topic_name),
                  mqtt_message->payload_len, STRING_PTR_PRINT_SANITY_CHECK((char *)mqtt_message->payload));
            break;
        case MQTT_EVENT_SUBSCRIBE_SUCCESS:
            Log_i("subscribe success, packet-id=%u", (unsigned int)packet_id);
            break;

        case MQTT_EVENT_SUBSCRIBE_TIMEOUT:
            Log_i("subscribe wait ack timeout, packet-id=%u", (unsigned int)packet_id);
            break;

        case MQTT_EVENT_SUBSCRIBE_NACK:
            Log_i("subscribe nack, packet-id=%u", (unsigned int)packet_id);
            break;

        case MQTT_EVENT_UNSUBSCRIBE_SUCCESS:
            Log_i("unsubscribe success, packet-id=%u", (unsigned int)packet_id);
            break;

        case MQTT_EVENT_UNSUBSCRIBE_TIMEOUT:
            Log_i("unsubscribe timeout, packet-id=%u", (unsigned int)packet_id);
            break;

        case MQTT_EVENT_UNSUBSCRIBE_NACK:
            Log_i("unsubscribe nack, packet-id=%u", (unsigned int)packet_id);
            break;

        case MQTT_EVENT_PUBLISH_SUCCESS:
            Log_i("publish success, packet-id=%u", (unsigned int)packet_id);
            break;

        case MQTT_EVENT_PUBLISH_TIMEOUT:
            Log_i("publish timeout, packet-id=%u", (unsigned int)packet_id);
            break;

        case MQTT_EVENT_PUBLISH_NACK:
            Log_i("publish nack, packet-id=%u", (unsigned int)packet_id);
            break;
        default:
            Log_i("Should NOT arrive here.");
            break;
    }
}

/**
 * @brief Setup MQTT construct parameters.
 *
 * @param[in,out] initParams @see MQTTInitParams
 * @param[in] device_info @see DeviceInfo
 */
static void _setup_connect_init_params(MQTTInitParams *initParams, DeviceInfo *device_info)
{
    initParams->device_info            = device_info;
    initParams->command_timeout        = QCLOUD_IOT_MQTT_COMMAND_TIMEOUT;
    initParams->keep_alive_interval_ms = QCLOUD_IOT_MQTT_KEEP_ALIVE_INTERNAL;
    initParams->auto_connect_enable    = 1;
    initParams->event_handle.h_fp      = _mqtt_event_handler;
    initParams->event_handle.context   = NULL;
}

/**
 * @brief Publish a test mqtt message.
 *
 * @param[in, out] client pointer to mqtt client
 * @param[in] topic_keyword topic keyword
 * @param[in] qos qos to publish
 * @return packet id (>=0) when success, or err code (<0) @see IotReturnCode
 */
static int _publish_test_msg(void *client, const char *topic_keyword, QoS qos)
{
    char        topic_name[128] = {0};
    DeviceInfo *dev_info        = IOT_MQTT_GetDeviceInfo(client);

    int size = HAL_Snprintf(
        topic_name, sizeof(topic_name), "%s/%s/%s", STRING_PTR_PRINT_SANITY_CHECK(dev_info->product_id),
        STRING_PTR_PRINT_SANITY_CHECK(dev_info->device_name), STRING_PTR_PRINT_SANITY_CHECK(topic_keyword));
    if (size < 0 || size > sizeof(topic_name) - 1) {
        Log_e("topic content length not enough! content size:%d  buf size:%d", size, (int)sizeof(topic_name));
        return QCLOUD_ERR_FAILURE;
    }

    static int    test_count = 0;
    PublishParams pub_params = DEFAULT_PUB_PARAMS;
    pub_params.qos           = qos;

    char topic_content[128] = {0};
    size = HAL_Snprintf(topic_content, sizeof(topic_content), "{\"action\": \"publish_test\", \"count\": \"%d\"}",
                        test_count++);
    if (size < 0 || size > sizeof(topic_content) - 1) {
        Log_e("payload content length not enough! content size:%d  buf size:%d", size, (int)sizeof(topic_content));
        return -3;
    }

    pub_params.payload     = topic_content;
    pub_params.payload_len = strlen(topic_content);

    return IOT_MQTT_Publish(client, topic_name, &pub_params);
}

/**
 * @brief Callback when MQTT msg arrives @see OnMessageHandler
 *
 * @param[in, out] client pointer to mqtt client
 * @param[in] message publish message from server
 * @param[in] usr_data user data of SubscribeParams, @see SubscribeParams
 */
static void _on_message_callback(void *client, MQTTMessage *message, void *user_data)
{
    if (!message) {
        return;
    }

    Log_i("Receive Message With topicName:%.*s, payload:%.*s", (int)message->topic_len,
          STRING_PTR_PRINT_SANITY_CHECK(message->topic_name), (int)message->payload_len,
          STRING_PTR_PRINT_SANITY_CHECK((char *)message->payload));
}

/**
 * @brief Subscribe MQTT topic and wait for sub result.
 *
 * @param[in, out] client pointer to mqtt client
 * @param[in] topic_keyword topic keyword
 * @param[in] qos qos to subscribe
 * @return @see IotReturnCode
 */
static int _subscribe_topic_wait_result(void *client, char *topic_keyword, QoS qos)
{
    char        topic_name[128] = {0};
    DeviceInfo *dev_info        = IOT_MQTT_GetDeviceInfo(client);

    int size = HAL_Snprintf(
        topic_name, sizeof(topic_name), "%s/%s/%s", STRING_PTR_PRINT_SANITY_CHECK(dev_info->product_id),
        STRING_PTR_PRINT_SANITY_CHECK(dev_info->device_name), STRING_PTR_PRINT_SANITY_CHECK(topic_keyword));
    if (size < 0 || size > sizeof(topic_name) - 1) {
        Log_e("topic content length not enough! content size:%d  buf size:%d", size, (int)sizeof(topic_name));
        return QCLOUD_ERR_FAILURE;
    }

    SubscribeParams sub_params    = DEFAULT_SUB_PARAMS;
    sub_params.qos                = qos;
    sub_params.on_message_handler = _on_message_callback;

    int rc = IOT_MQTT_Subscribe(client, topic_name, &sub_params);
    if (rc < 0) {
        Log_e("MQTT subscribe FAILED: %d", rc);
        return rc;
    }

    int wait_cnt = 10;
    while (!IOT_MQTT_IsSubReady(client, topic_name) && (wait_cnt > 0)) {
        // wait for subscription result
        rc = IOT_MQTT_Yield(client, 1000);
        if (rc) {
            Log_e("MQTT error: %d", rc);
            return rc;
        }
        wait_cnt--;
    }

    if (wait_cnt > 0) {
        return QCLOUD_RET_SUCCESS;
    } else {
        Log_e("wait for subscribe result timeout!");
        return QCLOUD_ERR_FAILURE;
    }
}

/**
 * @brief Unsubscribe MQTT topic.
 *
 * @param[in, out] client pointer to mqtt client
 * @param[in] topic_keyword topic keyword
 * @return @see IotReturnCode
 */
static int _unsubscribe_topic(void *client, char *topic_keyword)
{
    char        topic_name[128] = {0};
    DeviceInfo *dev_info        = IOT_MQTT_GetDeviceInfo(client);

    int size = HAL_Snprintf(
        topic_name, sizeof(topic_name), "%s/%s/%s", STRING_PTR_PRINT_SANITY_CHECK(dev_info->product_id),
        STRING_PTR_PRINT_SANITY_CHECK(dev_info->device_name), STRING_PTR_PRINT_SANITY_CHECK(topic_keyword));
    if (size < 0 || size > sizeof(topic_name) - 1) {
        Log_e("topic content length not enough! content size:%d  buf size:%d", size, (int)sizeof(topic_name));
        return QCLOUD_ERR_FAILURE;
    }

    int rc = IOT_MQTT_Unsubscribe(client, topic_name);
    if (rc < 0) {
        Log_e("MQTT unsubscribe FAILED: %d", rc);
        return rc;
    }

    // wait for unsuback
    rc = IOT_MQTT_Yield(client, 500);
    return rc;
}

// ----------------------------------------------------------------------------
// Main
// ----------------------------------------------------------------------------
static int sg_main_exit = 0;

#ifdef __linux__

#include <signal.h>
#include <pthread.h>
#include <unistd.h>

static void _main_exit(int sig)
{
    Log_e("demo exit by signal:%d\n", sig);
    sg_main_exit = 1;
}
#endif

int main(int argc, char **argv)
{
#ifdef __linux__
    signal(SIGINT, _main_exit);
#endif

    int rc;

    // init log level
    LogHandleFunc func;
    func.log_malloc               = HAL_Malloc;
    func.log_free                 = HAL_Free;
    func.log_get_current_time_str = HAL_Timer_current;
    func.log_printf               = HAL_Printf;
    func.log_handle               = NULL;
    utils_log_init(func, eLOG_DEBUG, 2048);

    DeviceInfo device_info;

    rc = HAL_GetDevInfo((void *)&device_info);
    if (rc) {
        Log_e("get device info failed: %d", rc);
        return rc;
    }

    // init connection
    MQTTInitParams init_params = DEFAULT_MQTT_INIT_PARAMS;
    _setup_connect_init_params(&init_params, &device_info);

    // create MQTT client and connect with server
    void *client = IOT_MQTT_Construct(&init_params);
    if (client) {
        Log_i("Cloud Device Construct Success");
    } else {
        Log_e("MQTT Construct failed!");
        return QCLOUD_ERR_FAILURE;
    }

    // subscribe normal topics and wait result
    rc = _subscribe_topic_wait_result(client, "data", QOS0);
    if (rc) {
        Log_e("Client Subscribe Topic Failed: %d", rc);
        return rc;
    }

    do {
        rc = _publish_test_msg(client, "data", QOS1);
        if (rc < 0) {
            Log_e("client publish topic failed :%d.", rc);
        }

        rc = IOT_MQTT_Yield(client, 2000);
        if (rc == QCLOUD_ERR_MQTT_ATTEMPTING_RECONNECT) {
            HAL_SleepMs(1000);
            continue;
        } else if (rc != QCLOUD_RET_SUCCESS && rc != QCLOUD_RET_MQTT_RECONNECTED) {
            Log_e("exit with error: %d", rc);
            break;
        }
    } while (!sg_main_exit);

    rc = _unsubscribe_topic(client, "data");
    rc |= IOT_MQTT_Destroy(&client);

    utils_log_deinit();
    return rc;
}
