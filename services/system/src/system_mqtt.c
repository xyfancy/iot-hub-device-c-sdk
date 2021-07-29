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
 * @file system_mqtt.c
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

#include "qcloud_iot_system.h"

#define MAX_SYSTEM_INFO_SIZE 256

#define SYS_MQTT_GET_RESOURCE_TIME "{\"type\":\"get\",\"resource\":[\"time\"]}"

/**
 * @brief data structure for system time service
 */
typedef struct {
    bool     result_recv_ok;
    uint32_t time;
    uint64_t ntptime1;
    uint64_t ntptime2;
    uint64_t result_recv_time;
} SystemResultInfo;

/**
 * @brief
 *
 * @param[in,out] client pointer to mqtt client
 * @param[in] message message from topic
 * @param[in,out] usr_data pointer to @see SystemResultInfo
 */
static void _system_mqtt_message_callback(void *client, const MQTTMessage *message, void *usr_data)
{
    int               rc     = 0;
    SystemResultInfo *result = (SystemResultInfo *)usr_data;
    UtilsJsonValue    value;

    Log_d("Receive system result message:%.*s", message->payload_len, message->payload);

    // get time
    rc = utils_json_value_get("time", strlen("time"), (char *)message->payload, message->payload_len, &value);
    if (rc) {
        return;
    }
    rc = utils_json_value_data_get(value, UTILS_JSON_VALUE_TYPE_UINT32, &result->time);
    if (rc) {
        return;
    }

    // get ntptime1
    rc = utils_json_value_get("ntptime1", strlen("ntptime1"), (char *)message->payload, message->payload_len, &value);
    if (rc) {
        return;
    }
    rc = utils_json_value_data_get(value, UTILS_JSON_VALUE_TYPE_UINT64, &result->ntptime1);
    if (rc) {
        return;
    }

    // get ntptime2
    rc = utils_json_value_get("ntptime2", strlen("ntptime2"), (char *)message->payload, message->payload_len, &value);
    if (rc) {
        return;
    }
    rc = utils_json_value_data_get(value, UTILS_JSON_VALUE_TYPE_UINT64, &result->ntptime2);
    if (rc) {
        result->ntptime1 = result->time * 1000;
        result->ntptime2 = result->time * 1000;
    }

    result->result_recv_time = HAL_Timer_CurrentMs();
    result->result_recv_ok   = true;
}

/**
 * @brief Check and subscribe system result topic.
 *
 * @param[in,out] client pointer to mqtt client
 * @return @see IotReturnCode
 */
static int _system_mqtt_result_topic_check_and_sub(void *client, const char *topic)
{
    int               rc          = 0;
    SubscribeParams   sub_params  = DEFAULT_SUB_PARAMS;
    SystemResultInfo *system_info = (SystemResultInfo *)HAL_Malloc(sizeof(SystemResultInfo));
    if (!system_info) {
        return QCLOUD_ERR_MALLOC;
    }

    system_info->result_recv_ok = false;

    sub_params.on_message_handler = _system_mqtt_message_callback;
    sub_params.qos                = QOS1;
    sub_params.user_data          = system_info;
    sub_params.user_data_free     = HAL_Free;

    rc = IOT_MQTT_SubscribeSync(client, topic, &sub_params);
    if (rc) {
        HAL_Free(system_info);
    }
    return rc;
}

/**
 * @brief Publish get time message to system topic.
 *
 * @param[in,out] client pointer to mqtt client
 * @return >= 0 for success
 */
static int _system_mqtt_get_resource_time_publish(void *client)
{
    char topic_name[MAX_SIZE_OF_CLOUD_TOPIC];
    HAL_Snprintf(topic_name, sizeof(topic_name), "$sys/operation/%s/%s",
                 STRING_PTR_PRINT_SANITY_CHECK(IOT_MQTT_GetDeviceInfo(client)->product_id),
                 STRING_PTR_PRINT_SANITY_CHECK(IOT_MQTT_GetDeviceInfo(client)->device_name));

    PublishParams pub_params = DEFAULT_PUB_PARAMS;
    pub_params.qos           = QOS0;
    pub_params.payload       = SYS_MQTT_GET_RESOURCE_TIME;
    pub_params.payload_len   = strlen(SYS_MQTT_GET_RESOURCE_TIME);
    return IOT_MQTT_Publish(client, topic_name, &pub_params);
}

/**
 * @brief Wait system result, timeout @see QCLOUD_IOT_MQTT_WAIT_ACK_TIMEOUT
 *
 * @param[in,out] client pointer to mqtt client
 * @param[in] topic system result topic
 * @param[out] time time from system result topic
 * @return @see IotReturnCode
 */
static int _system_mqtt_result_wait(void *client, const char *topic, uint32_t *time)
{
    int               rc = 0;
    Timer             wait_result_timer;
    SystemResultInfo *result;

    HAL_Timer_CountdownMs(&wait_result_timer, QCLOUD_IOT_MQTT_WAIT_ACK_TIMEOUT);

    result = IOT_MQTT_GetSubUsrData(client, topic);
    if (!result) {
        return QCLOUD_ERR_FAILURE;
    }

    while (!rc && !HAL_Timer_Expired(&wait_result_timer)) {
        rc = IOT_MQTT_Yield(client, QCLOUD_IOT_MQTT_YIELD_TIMEOUT);
        if (result->result_recv_ok) {
            *time = result->time;
            return QCLOUD_RET_SUCCESS;
        }
    }
    return QCLOUD_ERR_MQTT_REQUEST_TIMEOUT;
}

/**
 * @brief Get time from system result topic
 *
 * @param[in,out] client pointer to mqtt client
 * @param[out] time time from system result topic
 * @return @see IotReturnCode
 */
int IOT_Sys_GetTime(void *client, uint32_t *time)
{
    POINTER_SANITY_CHECK(client, QCLOUD_ERR_INVAL);
    POINTER_SANITY_CHECK(time, QCLOUD_ERR_INVAL);

    int  rc = 0;
    char system_result_topic[MAX_SIZE_OF_CLOUD_TOPIC];

    HAL_Snprintf(system_result_topic, MAX_SIZE_OF_CLOUD_TOPIC, "$sys/operation/result/%s/%s",
                 STRING_PTR_PRINT_SANITY_CHECK(IOT_MQTT_GetDeviceInfo(client)->product_id),
                 STRING_PTR_PRINT_SANITY_CHECK(IOT_MQTT_GetDeviceInfo(client)->device_name));

    rc = _system_mqtt_result_topic_check_and_sub(client, system_result_topic);
    if (rc) {
        return rc;
    }

    rc = _system_mqtt_get_resource_time_publish(client);
    if (rc < 0) {
        return rc;
    }

    return _system_mqtt_result_wait(client, system_result_topic, time);
}

/**
 * @brief Get ntp time from system result topic and set to system.
 *
 * @param[in,out] client pointer to mqtt client
 * @return @see IotReturnCode
 */
int IOT_Sys_SyncNTPTime(void *client)
{
    POINTER_SANITY_CHECK(client, QCLOUD_ERR_INVAL);

    int      rc       = 0;
    uint32_t time_get = 0;
    uint64_t local_publish_before, local_ntptime = 0;

    SystemResultInfo *result = NULL;

    char system_result_topic[MAX_SIZE_OF_CLOUD_TOPIC];

    HAL_Snprintf(system_result_topic, MAX_SIZE_OF_CLOUD_TOPIC, "$sys/operation/result/%s/%s",
                 STRING_PTR_PRINT_SANITY_CHECK(IOT_MQTT_GetDeviceInfo(client)->product_id),
                 STRING_PTR_PRINT_SANITY_CHECK(IOT_MQTT_GetDeviceInfo(client)->device_name));

    rc = _system_mqtt_result_topic_check_and_sub(client, system_result_topic);
    if (rc) {
        return rc;
    }

    // prepare for publish
    result = IOT_MQTT_GetSubUsrData(client, system_result_topic);
    if (!result) {
        return -1;
    }
    result->result_recv_ok = false;
    local_publish_before   = HAL_Timer_CurrentMs();

    // publish and wait
    rc = _system_mqtt_get_resource_time_publish(client);
    if (rc < 0) {
        return rc;
    }

    rc = _system_mqtt_result_wait(client, system_result_topic, &time_get);
    if (rc) {
        return rc;
    }

    local_ntptime = (result->ntptime2 + result->ntptime1 + result->result_recv_time - local_publish_before) / 2;

    rc = HAL_Timer_SetSystimeSec(time_get);
    if (rc) {
        Log_e("set systime sec failed, timestamp %d sec,  please check permission or other ret:%d", time_get, rc);
    } else {
        Log_i("set systime sec success, timestamp %d sec", time_get);
    }

    rc = HAL_Timer_SetSystimeMs(local_ntptime);
    if (rc) {
        Log_e("set systime ms failed, timestamp %lld, please check permission or other ret :%d", local_ntptime, rc);
    } else {
        Log_i("set systime ms success, timestamp %lld ms", local_ntptime);
    }
    return rc;
}
