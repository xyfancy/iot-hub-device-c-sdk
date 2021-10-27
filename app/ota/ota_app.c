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
 * @file broadcast_sample.c
 * @brief
 * @author fancyxu (fancyxu@tencent.com)
 * @version 1.0
 * @date 2021-07-18
 *
 * @par Change Log:
 * <table>
 * <tr><th>Date       <th>Version <th>Author    <th>Description
 * <tr><td>2021-07-18 <td>1.0     <td>fancyxu   <td>first commit
 * </table>
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "qcloud_iot_common.h"
#include "ota_downloader.h"

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
                  mqtt_message->payload_len, STRING_PTR_PRINT_SANITY_CHECK(mqtt_message->payload_str));
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
// OTA callback
// ----------------------------------------------------------------------------

void _update_firmware_callback(UtilsJsonValue version, UtilsJsonValue url, UtilsJsonValue md5sum, uint32_t file_size,
                               void *usr_data)
{
    Log_i("recv firmware: version=%.*s|url=%.*s|md5sum=%.*s|file_size=%u", version.value_len, version.value,
          url.value_len, url.value, md5sum.value_len, md5sum.value, file_size);
    // only one firmware one time is supportted now
    OTAFirmwareInfo firmware_info;
    memset(&firmware_info, 0, sizeof(OTAFirmwareInfo));
    strncpy(firmware_info.version, version.value, version.value_len);
    strncpy(firmware_info.md5sum, md5sum.value, md5sum.value_len);
    firmware_info.file_size = file_size;
    ota_downloader_info_set(&firmware_info, url.value, url.value_len);
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

    char buf[1024];

    // init log level
    LogHandleFunc func = {
        .log_malloc               = HAL_Malloc,
        .log_free                 = HAL_Free,
        .log_get_current_time_str = HAL_Timer_Current,
        .log_printf               = HAL_Printf,
        .log_handle               = NULL,
    };
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

    // OTA init
    IotOTAUpdateCallback ota_callback = {
        .update_firmware_callback      = _update_firmware_callback,
        .report_version_reply_callback = NULL,
    };

    rc = IOT_OTA_Init(client, ota_callback, NULL);
    if (rc) {
        Log_e("OTA init failed!, rc=%d", rc);
        return rc;
    }

    rc = IOT_OTA_ReportVersion(client, buf, sizeof(buf), QCLOUD_IOT_DEVICE_SDK_VERSION);
    if (rc) {
        Log_e("OTA report version failed!, rc=%d", rc);
        return rc;
    }

    rc = ota_downloader_init(client);
    if (rc) {
        Log_e("OTA downloader init failed!, rc=%d", rc);
        return rc;
    }

    do {
        ota_downloader_process();

        rc = IOT_MQTT_Yield(client, QCLOUD_IOT_MQTT_YIELD_TIMEOUT);
        switch (rc) {
            case QCLOUD_RET_SUCCESS:
                break;
            case QCLOUD_ERR_MQTT_ATTEMPTING_RECONNECT:
                continue;
            case QCLOUD_RET_MQTT_RECONNECTED:
                break;
            default:
                Log_e("Exit loop caused of errCode:%d", rc);
                goto exit;
        }
    } while (!sg_main_exit);
exit:
    ota_downloader_deinit();
    IOT_OTA_Deinit(client);
    rc = IOT_MQTT_Destroy(&client);
    utils_log_deinit();
    return rc;
}
