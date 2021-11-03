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
 * @file data_template_sample.c
 * @brief
 * @author fancyxu (fancyxu@tencent.com)
 * @version 1.0
 * @date 2021-09-27
 *
 * @par Change Log:
 * <table>
 * <tr><th>Date       <th>Version <th>Author    <th>Description
 * <tr><td>2021-09-27 <td>1.0     <td>fancyxu   <td>first commit
 * </table>
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "qcloud_iot_common.h"
#include "qcloud_iot_explorer.h"
#include "data_template_config.h"

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
static void _setup_connect_init_params(MQTTInitParams *init_params, DeviceInfo *device_info)
{
    init_params->device_info            = device_info;
    init_params->command_timeout        = QCLOUD_IOT_MQTT_COMMAND_TIMEOUT;
    init_params->keep_alive_interval_ms = QCLOUD_IOT_MQTT_KEEP_ALIVE_INTERNAL;
    init_params->auto_connect_enable    = 1;
    init_params->event_handle.h_fp      = _mqtt_event_handler;
    init_params->event_handle.context   = NULL;
}

// ----------------------------------------------------------------------------
// Data template callback
// ----------------------------------------------------------------------------

static void _method_control_callback(UtilsJsonValue client_token, UtilsJsonValue params, void *usr_data)
{
    char buf[256];
    Log_i("recv msg[%.*s]: params=%.*s", client_token.value_len, client_token.value, params.value_len, params.value);
    IOT_DataTemplate_PropertyControlReply(usr_data, buf, sizeof(buf), 0, client_token);
    usr_data_template_property_parse(params);
}

static void _method_get_status_reply_callback(UtilsJsonValue client_token, int code, UtilsJsonValue reported,
                                              UtilsJsonValue control, void *usr_data)
{
    char buf[256];
    Log_i("recv msg[%.*s]: code=%d|reported=%.*s|control=%.*s", client_token.value_len, client_token.value, code,
          reported.value_len, STRING_PTR_PRINT_SANITY_CHECK(reported.value), control.value_len,
          STRING_PTR_PRINT_SANITY_CHECK(control.value));
    usr_data_template_property_parse(control);
    IOT_DataTemplate_PropertyClearControl(usr_data, buf, sizeof(buf));
}

static void _method_action_callback(UtilsJsonValue client_token, UtilsJsonValue action_id, UtilsJsonValue params,
                                    void *usr_data)
{
    UsrActionIndex            index;
    int                       rc;
    DataTemplatePropertyValue value_time, value_color, value_total_time;

    char buf[256];

    Log_i("recv msg[%.*s]: action_id=%.*s|params=%.*s", client_token.value_len, client_token.value, action_id.value_len,
          action_id.value, params.value_len, params.value);

    rc = usr_data_template_action_parse(action_id, params, &index);
    if (rc) {
        return;
    }

    switch (index) {
        case USR_ACTION_INDEX_LIGHT_BLINK:
            value_time       = usr_data_template_action_input_value_get(USR_ACTION_INDEX_LIGHT_BLINK,
                                                                  USR_ACTION_LIGHT_BLINK_INPUT_INDEX_TIME);
            value_color      = usr_data_template_action_input_value_get(USR_ACTION_INDEX_LIGHT_BLINK,
                                                                   USR_ACTION_LIGHT_BLINK_INPUT_INDEX_COLOR);
            value_total_time = usr_data_template_action_input_value_get(USR_ACTION_INDEX_LIGHT_BLINK,
                                                                        USR_ACTION_LIGHT_BLINK_INPUT_INDEX_TOTAL_TIME);
            Log_i("light[%d] blink %d every %d s ", value_color.value_enum, value_time.value_int,
                  value_total_time.value_int);
            usr_data_template_action_reply(usr_data, buf, sizeof(buf), index, client_token, 0, "{\"err_code\":0}");
            break;
        default:
            break;
    }
}

// ----------------------------------------------------------------------------
// Data template upstream
// ----------------------------------------------------------------------------

/**
 * @brief Report status.
 *
 * @param[in,out] client pointer to mqtt client
 */
static void _cycle_report(void *client)
{
    char         buf[256];
    static Timer sg_cycle_report_timer;
    if (HAL_Timer_Expired(&sg_cycle_report_timer)) {
        usr_data_template_event_post(client, buf, sizeof(buf), USR_EVENT_INDEX_STATUS_REPORT,
                                     "{\"status\":0,\"message\":\"ok\"}");
        HAL_Timer_Countdown(&sg_cycle_report_timer, 60);
    }
}

/**
 * @brief Init usr data template and data.
 *
 */
static void _usr_init(void)
{
    usr_data_template_init();

    DataTemplatePropertyValue value;
    value.value_int = 0;
    usr_data_template_property_value_set(USR_PROPERTY_INDEX_POWER_SWITCH, value);
    value.value_enum = 0;
    usr_data_template_property_value_set(USR_PROPERTY_INDEX_COLOR, value);
    value.value_int = 10;
    usr_data_template_property_value_set(USR_PROPERTY_INDEX_BRIGHTNESS, value);
    value.value_string = "light";
    usr_data_template_property_value_set(USR_PROPERTY_INDEX_NAME, value);
    value.value_int = 30;
    usr_data_template_property_struct_value_set(USR_PROPERTY_INDEX_POSITION, USR_PROPERTY_POSITION_INDEX_LONGITUDE,
                                                value);
    value.value_int = 30;
    usr_data_template_property_struct_value_set(USR_PROPERTY_INDEX_POSITION, USR_PROPERTY_POSITION_INDEX_LATITUDE,
                                                value);
    value.value_string = "high";
    usr_data_template_property_value_set(USR_PROPERTY_INDEX_POWER, value);
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
    Log_e("demo exit by signal:%d", sig);
    sg_main_exit = 1;
}
#endif

int main(int argc, char **argv)
{
#ifdef __linux__
    signal(SIGINT, _main_exit);
#endif

    int rc;

    char buf[1024];

    // init log level
    LogHandleFunc func;
    func.log_malloc               = HAL_Malloc;
    func.log_free                 = HAL_Free;
    func.log_get_current_time_str = HAL_Timer_Current;
    func.log_printf               = HAL_Printf;
    func.log_handle               = NULL;
    utils_log_init(func, LOG_LEVEL_DEBUG, 2048);

    DeviceInfo device_info;

    rc = HAL_GetDevInfo((void *)&device_info);
    if (rc) {
        Log_e("get device info failed: %d", rc);
        return rc;
    }

    _usr_init();

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
    IotDataTemplateCallback callback                            = DEFAULT_DATA_TEMPLATE_CALLBACK;
    callback.property_callback.method_control_callback          = _method_control_callback;
    callback.property_callback.method_get_status_reply_callback = _method_get_status_reply_callback;
    callback.action_callback.method_action_callback             = _method_action_callback;

    rc = IOT_DataTemplate_Init(client, callback, client);
    if (rc) {
        Log_e("Client Subscribe Topic Failed: %d", rc);
        return rc;
    }

    IOT_DataTemplate_PropertyGetStatus(client, buf, sizeof(buf));

    do {
        rc = IOT_MQTT_Yield(client, QCLOUD_IOT_MQTT_YIELD_TIMEOUT);
        switch (rc) {
            case QCLOUD_RET_SUCCESS:
                break;
            case QCLOUD_ERR_MQTT_ATTEMPTING_RECONNECT:
                continue;
            case QCLOUD_RET_MQTT_RECONNECTED:
                IOT_DataTemplate_PropertyGetStatus(client, buf, sizeof(buf));
                break;
            default:
                Log_e("Exit loop caused of errCode:%d", rc);
                goto exit;
        }
        _cycle_report(client);
        usr_data_template_property_report(client, buf, sizeof(buf));
    } while (!sg_main_exit);

exit:
    IOT_DataTemplate_Deinit(client);
    rc |= IOT_MQTT_Destroy(&client);
    utils_log_deinit();
    return rc;
}
