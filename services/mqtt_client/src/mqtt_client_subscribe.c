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
 * @file mqtt_client_subscribe.c
 * @brief
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

#include "mqtt_client.h"

/**
 * @brief Push node to subscribe(unsubscribe) ACK wait list.
 *
 * @param[in,out] client pointer to mqtt_client
 * @param[in] packet_len packet len of publish packet
 * @param[in] packet_id packet id
 * @param[in] type mqtt packet type SUBSCRIBE or UNSUBSCRIBE
 * @param[in] handler subtopic handle
 * @param[out] node node to push to list
 * @return @see IoT_Return_Code
 */
static int _push_sub_info_to_list(Qcloud_IoT_Client *client, int packet_len, uint16_t packet_id, MQTTPacketType type,
                                  const SubTopicHandle *handler, void **node)
{
    IOT_FUNC_ENTRY;
    void *            list     = client->list_sub_wait_ack;
    QcloudIotSubInfo *sub_info = NULL;

    sub_info = (QcloudIotSubInfo *)HAL_Malloc(sizeof(QcloudIotSubInfo) + packet_len);
    if (!sub_info) {
        Log_e("memory malloc failed!");
        IOT_FUNC_EXIT_RC(QCLOUD_ERR_FAILURE);
    }

    sub_info->buf       = (uint8_t *)sub_info + sizeof(QcloudIotSubInfo);
    sub_info->len       = packet_len;
    sub_info->type      = type;
    sub_info->packet_id = packet_id;
    sub_info->handler   = *handler;
    memcpy(sub_info->buf, client->write_buf, packet_len);
    HAL_Timer_countdown_ms(&sub_info->sub_start_time, client->command_timeout_ms);

    *node = utils_list_push(list, sub_info);
    if (!*node) {
        HAL_Free(sub_info);
        Log_e("list push failed! Check the list len!");
        IOT_FUNC_EXIT_RC(QCLOUD_ERR_FAILURE);
    }

    IOT_FUNC_EXIT_RC(QCLOUD_RET_SUCCESS);
}

/**
 * @brief Remove node signed with packet id from subscribe ACK wait list, and return the sub handler
 *
 * @param[in,out] client pointer to mqtt_client
 * @param[in] packet_id packet id
 * @param[out] sub_handle @see SubTopicHandle
 */
static void _remove_sub_info_from_list(Qcloud_IoT_Client *client, uint16_t packet_id, SubTopicHandle *sub_handle)
{
    void *node, *iter = NULL;
    void *list = client->list_sub_wait_ack;

    QcloudIotSubInfo *sub_info;

    if (!utils_list_len_get(list)) {
        return;
    }

    iter = utils_list_iterator_create(list, LIST_HEAD);
    if (!iter) {
        return;
    }

    while ((node = utils_list_iterator_next(iter))) {
        sub_info = (QcloudIotSubInfo *)utils_list_get_val(node);
        if (!sub_info) {
            Log_e("node's value is invalid!");
            utils_list_remove(list, node);
            continue;
        }

        if (sub_info->packet_id == packet_id) {
            // return handle
            *sub_handle = sub_info->handler;
            utils_list_remove(list, node);
        }
    }

    utils_list_iterator_destroy(iter);
}

/**
 * @brief Remove sub handle when unsubscribe.
 *
 * @param[in,out] client pointer to mqtt_client
 * @param[in] topic_filter topic to remove
 * @return true topic exist
 * @return false topic no exist
 */
static bool _remove_sub_handle_from_array(Qcloud_IoT_Client *client, const char *topic_filter)
{
    int  i;
    bool topic_exists = false;

    // remove from message handler array
    HAL_MutexLock(client->lock_generic);
    for (i = 0; i < MAX_MESSAGE_HANDLERS; ++i) {
        if ((client->sub_handles[i].topic_filter && !strcmp(client->sub_handles[i].topic_filter, topic_filter)) ||
            strstr(topic_filter, "/#") || strstr(topic_filter, "/+")) {
            // notify this event to topic subscriber
            if (client->sub_handles[i].params.on_sub_event_handler) {
                client->sub_handles[i].params.on_sub_event_handler(client, MQTT_EVENT_UNSUBSCRIBE,
                                                                   client->sub_handles[i].params.user_data);
            }
            // free the topic filter malloced in qcloud_iot_mqtt_subscribe
            HAL_Free((void *)client->sub_handles[i].topic_filter);
            client->sub_handles[i].topic_filter = NULL;
            // we don't want to break here, if the same topic is registered*with 2 callbacks.Unlikely scenario
            topic_exists = true;
        }
    }
    HAL_MutexUnlock(client->lock_generic);
    return topic_exists;
}

/**
 * @brief Add sub handle when unsubscribe.
 *
 * @param[in,out] client pointer to mqtt_client
 * @param[in] sub_handle sub_handle to be add to array
 * @return true topic exist
 * @return false topic no exist
 */
static int _add_sub_handle_to_array(Qcloud_IoT_Client *client, const SubTopicHandle *sub_handle)
{
    IOT_FUNC_ENTRY;
    int i, i_free = -1;

    HAL_MutexLock(client->lock_generic);

    for (i = 0; i < MAX_MESSAGE_HANDLERS; ++i) {
        if (client->sub_handles[i].topic_filter) {
            if (!strcmp(client->sub_handles[i].topic_filter, sub_handle->topic_filter)) {
                i_free = i;
                // free the memory before
                HAL_Free(client->sub_handles->topic_filter);
                Log_w("Identical topic found: %s", sub_handle->topic_filter);
                break;
            }
        }
        i_free = i_free == -1 ? i : i_free;
    }

    if (-1 == i_free) {
        Log_e("NO more @sub_handles space!");
        HAL_MutexUnlock(client->lock_generic);
        IOT_FUNC_EXIT_RC(QCLOUD_ERR_FAILURE);
    }

    client->sub_handles[i_free] = *sub_handle;
    HAL_MutexUnlock(client->lock_generic);
    IOT_FUNC_EXIT_RC(QCLOUD_RET_SUCCESS);
}

/**
 * @brief Serialize and send subscribe packet.
 *
 * @param[in,out] client pointer to mqtt client
 * @param[in] topic_filter topic to subscribe
 * @param[in] params subscribe params
 * @return >=0 for packet id, < 0 for failed @see IoT_Return_Code
 */
int qcloud_iot_mqtt_subscribe(Qcloud_IoT_Client *client, const char *topic_filter, const SubscribeParams *params)
{
    IOT_FUNC_ENTRY;
    int            rc, packet_len, qos = params->qos;
    uint16_t       packet_id;
    char *         topic_filter_stored;
    void *         node = NULL;
    SubTopicHandle sub_handle;

    // topic filter should be valid in the whole sub life
    topic_filter_stored = HAL_Malloc(strlen(topic_filter) + 1);
    if (!topic_filter_stored) {
        IOT_FUNC_EXIT_RC(QCLOUD_ERR_MALLOC);
    }
    strncpy(topic_filter_stored, topic_filter, strlen(topic_filter) + 1);

    packet_id = get_next_packet_id(client);
    Log_d("subscribe topic_name=%s|packet_id=%d", topic_filter_stored, packet_id);
    // serialize packet
    HAL_MutexLock(client->lock_write_buf);
    packet_len = mqtt_subscribe_packet_serialize(client->write_buf, client->write_buf_size, packet_id, 1,
                                                 &topic_filter_stored, &qos);
    if (packet_len < 0) {
        HAL_MutexUnlock(client->lock_write_buf);
        rc = packet_len == MQTT_ERR_SHORT_BUFFER ? QCLOUD_ERR_BUF_TOO_SHORT : QCLOUD_ERR_FAILURE;
        goto exit;
    }

    // add node into sub ack wait list
    sub_handle.topic_filter = topic_filter_stored;
    sub_handle.params       = *params;

    rc = _push_sub_info_to_list(client, packet_len, packet_id, SUBSCRIBE, &sub_handle, &node);
    if (rc) {
        HAL_MutexUnlock(client->lock_write_buf);
        goto exit;
    }

    // send packet
    rc = send_mqtt_packet(client, packet_len);
    HAL_MutexUnlock(client->lock_write_buf);
    if (rc) {
        utils_list_remove(client->list_sub_wait_ack, node);
        goto exit;
    }
    IOT_FUNC_EXIT_RC(packet_id);
exit:
    HAL_Free(topic_filter_stored);
    IOT_FUNC_EXIT_RC(rc);
}

/**
 * @brief Deserialize suback packet and return sub result.
 *
 * @param[in,out] client pointer to mqtt client
 * @return @see IoT_Return_Code
 */
int qcloud_iot_mqtt_handle_suback(Qcloud_IoT_Client *client)
{
    IOT_FUNC_ENTRY;
    int            rc, count = 0;
    uint16_t       packet_id = 0;
    int            granted_qos;
    MQTTEventMsg   msg        = {MQTT_EVENT_SUBSCRIBE_SUCCESS, NULL};
    SubTopicHandle sub_handle = {0};
    MQTTEventType  event_type = MQTT_EVENT_SUBSCRIBE_SUCCESS;

    rc = mqtt_suback_packet_deserialize(client->read_buf, client->read_buf_size, 1, &count, &packet_id, &granted_qos);
    if (rc) {
        IOT_FUNC_EXIT_RC(rc);
    }
    msg.msg = (void *)(uintptr_t)packet_id;

    // remove sub info and get sub handle
    _remove_sub_info_from_list(client, packet_id, &sub_handle);
    if (!sub_handle.topic_filter) {
        Log_e("can't get sub handle from list!");
        IOT_FUNC_EXIT_RC(QCLOUD_ERR_MQTT_SUB);
    }

    // check return code in SUBACK packet: 0x00(QOS0, SUCCESS),0x01(QOS1, SUCCESS),0x02(QOS2,SUCCESS),0x80(Failure)
    if (granted_qos != 0x80) {
        // if success, add sub handle to array
        rc = _add_sub_handle_to_array(client, &sub_handle);
        if (rc) {
            IOT_FUNC_EXIT_RC(rc);
        }
    } else {
        msg.event_type = MQTT_EVENT_SUBSCRIBE_NACK;
        event_type     = MQTT_EVENT_SUBSCRIBE_NACK;
        Log_e("MQTT SUBSCRIBE failed, packet_id: %u topic: %s", packet_id, sub_handle.topic_filter);
        HAL_Free(sub_handle.topic_filter);
        rc = QCLOUD_ERR_MQTT_SUB;
    }

    // notify this event to user callback
    if (client->event_handle.h_fp) {
        client->event_handle.h_fp(client, client->event_handle.context, &msg);
    }

    // notify this event to topic subscriber
    if (sub_handle.params.on_sub_event_handler) {
        sub_handle.params.on_sub_event_handler(client, event_type, sub_handle.params.user_data);
    }
    IOT_FUNC_EXIT_RC(rc);
}

/**
 * @brief Serialize and send unsubscribe packet.
 *
 * @param[in,out] client pointer to mqtt client
 * @param[in] topic_filter topic to unsubscribe
 * @return >=0 packet id, < 0 for failed @see IoT_Return_Code
 */
int qcloud_iot_mqtt_unsubscribe(Qcloud_IoT_Client *client, const char *topic_filter)
{
    IOT_FUNC_ENTRY;
    int      rc, packet_len;
    uint16_t packet_id;

    void *         node = NULL;
    SubTopicHandle sub_handle;
    memset(&sub_handle, 0, sizeof(SubTopicHandle));

    // remove from sub handle
    if (!_remove_sub_handle_from_array(client, topic_filter)) {
        Log_e("subscription does not exists: %s", topic_filter);
        IOT_FUNC_EXIT_RC(QCLOUD_ERR_MQTT_UNSUB_FAIL);
    }

    // topic filter should be valid in the whole sub life
    char *topic_filter_stored = HAL_Malloc(strlen(topic_filter) + 1);
    if (!topic_filter_stored) {
        Log_e("malloc failed");
        IOT_FUNC_EXIT_RC(QCLOUD_ERR_FAILURE);
    }
    strncpy(topic_filter_stored, topic_filter, strlen(topic_filter) + 1);

    packet_id = get_next_packet_id(client);
    Log_d("unsubscribe topic_name=%s|packet_id=%d", topic_filter_stored, packet_id);

    HAL_MutexLock(client->lock_write_buf);
    packet_len = mqtt_unsubscribe_packet_serialize(client->write_buf, client->write_buf_size, packet_id, 1,
                                                   &topic_filter_stored);
    if (packet_len < 0) {
        HAL_MutexUnlock(client->lock_write_buf);
        rc = packet_len == MQTT_ERR_SHORT_BUFFER ? QCLOUD_ERR_BUF_TOO_SHORT : QCLOUD_ERR_FAILURE;
        goto exit;
    }

    // add node into sub ack wait list
    sub_handle.topic_filter = topic_filter_stored;

    rc = _push_sub_info_to_list(client, packet_len, packet_id, UNSUBSCRIBE, &sub_handle, &node);
    if (rc) {
        Log_e("push unsubscribe info failed!");
        HAL_MutexUnlock(client->lock_write_buf);
        goto exit;
    }

    /* send the unsubscribe packet */
    rc = send_mqtt_packet(client, packet_len);
    HAL_MutexUnlock(client->lock_write_buf);
    if (rc) {
        utils_list_remove(client->list_sub_wait_ack, node);
        goto exit;
    }

    IOT_FUNC_EXIT_RC(packet_id);
exit:
    HAL_Free(topic_filter_stored);
    IOT_FUNC_EXIT_RC(rc);
}

/**
 * @brief Deserialize unsuback packet and remove from list.
 *
 * @param[in,out] client pointer to mqtt client
 * @return @see IoT_Return_Code
 */
int qcloud_iot_mqtt_handle_unsuback(Qcloud_IoT_Client *client)
{
    IOT_FUNC_ENTRY;
    int            rc;
    uint16_t       packet_id  = 0;
    SubTopicHandle sub_handle = {0};
    MQTTEventMsg   msg;

    rc = mqtt_unsuback_packet_deserialize(client->read_buf, client->read_buf_size, &packet_id);
    if (rc) {
        IOT_FUNC_EXIT_RC(rc);
    }

    _remove_sub_info_from_list(client, packet_id, &sub_handle);
    if (sub_handle.topic_filter) {
        HAL_Free(sub_handle.topic_filter);
    }

    if (client->event_handle.h_fp) {
        msg.event_type = MQTT_EVENT_UNSUBSCRIBE_SUCCESS;
        msg.msg        = (void *)(uintptr_t)packet_id;
        client->event_handle.h_fp(client, client->event_handle.context, &msg);
    }

    IOT_FUNC_EXIT_RC(QCLOUD_RET_SUCCESS);
}

/**
 * @brief Process suback waiting timeout.
 *
 * @param[in,out] client pointer to mqtt client
 */
void qcloud_iot_mqtt_check_sub_timeout(Qcloud_IoT_Client *client)
{
    IOT_FUNC_ENTRY;
    void *node, *iter = NULL;
    void *list = client->list_sub_wait_ack;

    MQTTEventMsg      msg;
    QcloudIotSubInfo *sub_info;

    if (!utils_list_len_get(list)) {
        return;
    }

    iter = utils_list_iterator_create(list, LIST_HEAD);
    if (!iter) {
        Log_e("new list iterator failed!");
        return;
    }

    while ((node = utils_list_iterator_next(iter))) {
        sub_info = (QcloudIotSubInfo *)utils_list_get_val(node);
        if (!sub_info) {
            Log_e("node's value is invalid!");
            utils_list_remove(list, node);
            continue;
        }

        // check the request if timeout or not
        if (HAL_Timer_remain(&sub_info->sub_start_time) > 0) {
            continue;
        }

        // notify timeout event
        if (client->event_handle.h_fp) {
            msg.event_type =
                SUBSCRIBE == sub_info->type ? MQTT_EVENT_SUBSCRIBE_TIMEOUT : MQTT_EVENT_UNSUBSCRIBE_TIMEOUT;
            msg.msg = (void *)(uintptr_t)sub_info->packet_id;

            if (sub_info->handler.params.on_sub_event_handler) {
                sub_info->handler.params.on_sub_event_handler(client, MQTT_EVENT_SUBSCRIBE_TIMEOUT,
                                                              sub_info->handler.params.user_data);
            }
            client->event_handle.h_fp(client, client->event_handle.context, &msg);
        }

        if (sub_info->handler.topic_filter) {
            HAL_Free((void *)(sub_info->handler.topic_filter));
        }

        utils_list_remove(list, node);
    }
    utils_list_iterator_destroy(iter);
    IOT_FUNC_EXIT;
}

/**
 * @brief Resubscribe topic when reconnect.
 *
 * @param[in,out] client pointer to mqtt client
 * @return @see IoT_Return_Code
 */
int qcloud_iot_mqtt_resubscribe(Qcloud_IoT_Client *client)
{
    IOT_FUNC_ENTRY;
    int   rc, itr = 0;
    char *topic = NULL;

    for (itr = 0; itr < MAX_MESSAGE_HANDLERS; itr++) {
        topic = (char *)client->sub_handles[itr].topic_filter;
        if (!topic) {
            continue;
        }

        rc = qcloud_iot_mqtt_subscribe(client, topic, &(client->sub_handles[itr].params));
        if (rc < 0) {
            Log_e("resubscribe topic[%s] failed %d!", topic, rc);
            IOT_FUNC_EXIT_RC(rc);
        }
    }

    IOT_FUNC_EXIT_RC(QCLOUD_RET_SUCCESS);
}

/**
 * @brief Return if topic is sub ready.
 *
 * @param[in,out] client pointer to mqtt client
 * @param[in] topic_filter topic filter
 * @return true for ready
 * @return false for not ready
 */
bool qcloud_iot_mqtt_is_sub_ready(Qcloud_IoT_Client *client, const char *topic_filter)
{
    IOT_FUNC_ENTRY;
    int i = 0;
    HAL_MutexLock(client->lock_generic);
    for (i = 0; i < MAX_MESSAGE_HANDLERS; ++i) {
        if ((client->sub_handles[i].topic_filter && !strcmp(client->sub_handles[i].topic_filter, topic_filter)) ||
            strstr(topic_filter, "/#") || strstr(topic_filter, "/+")) {
            HAL_MutexUnlock(client->lock_generic);
            return true;
        }
    }
    HAL_MutexUnlock(client->lock_generic);
    return false;
}
