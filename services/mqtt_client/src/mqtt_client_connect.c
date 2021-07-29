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
 * @file mqtt_client_connect.c
 * @brief
 * @author fancyxu (fancyxu@tencent.com)
 * @version 1.0
 * @date 2021-05-28
 *
 * @par Change Log:
 * <table>
 * <tr><th>Date       <th>Version <th>Author    <th>Description
 * <tr><td>2021-05-28 <td>1.0     <td>fancyxu   <td>first commit
 * <tr><td>2021-07-08 <td>1.1     <td>fancyxu   <td>fix code standard of IotReturnCode and QcloudIotClient
 * </table>
 */

#include "mqtt_client.h"

/**
 * @brief Serialize and send connect packet.
 *
 * @param[in,out] client pointer to mqtt client
 * @return @see IotReturnCode
 */
static int _mqtt_connect(QcloudIotClient *client)
{
    IOT_FUNC_ENTRY;
    uint8_t session_present, connack_rc;
    int     rc, packet_len;

    // TCP or TLS network connect
    rc = client->network_stack.connect(&(client->network_stack));
    if (rc) {
        IOT_FUNC_EXIT_RC(rc);
    }

    // send MQTT CONNECT packet
    HAL_MutexLock(client->lock_write_buf);
    packet_len = mqtt_connect_packet_serialize(client->write_buf, client->write_buf_size, &client->options);
    if (packet_len > 0) {
        rc = send_mqtt_packet(client, packet_len);
    } else {
        rc = packet_len == MQTT_ERR_SHORT_BUFFER ? QCLOUD_ERR_BUF_TOO_SHORT : QCLOUD_ERR_FAILURE;
    }
    HAL_MutexUnlock(client->lock_write_buf);
    if (rc) {
        IOT_FUNC_EXIT_RC(rc);
    }

    // recv MQTT CONNACK packet
    rc = qcloud_iot_mqtt_wait_for_read(client, CONNACK);
    if (rc) {
        IOT_FUNC_EXIT_RC(rc);
    }

    rc = mqtt_connack_packet_deserialize(client->read_buf, client->read_buf_size, &session_present, &connack_rc);
    if (rc) {
        IOT_FUNC_EXIT_RC(rc);
    }

    if (CONNACK_CONNECTION_ACCEPTED != connack_rc) {
        IOT_FUNC_EXIT_RC(connack_rc);
    }

    // set connect state
    set_client_conn_state(client, CONNECTED);

    HAL_MutexLock(client->lock_generic);
    client->was_manually_disconnected = client->is_ping_outstanding = 0;
    HAL_Timer_Countdown(&client->ping_timer, client->options.keep_alive_interval);
    HAL_MutexUnlock(client->lock_generic);
    IOT_FUNC_EXIT_RC(rc);
}

/**
 * @brief Connect MQTT server.
 *
 * @param[in,out] client pointer to mqtt client
 * @return  @see IotReturnCode
 */
int qcloud_iot_mqtt_connect(QcloudIotClient *client)
{
    IOT_FUNC_ENTRY;
    int rc = 0;

    // check connection state first
    if (get_client_conn_state(client)) {
        IOT_FUNC_EXIT_RC(QCLOUD_RET_MQTT_ALREADY_CONNECTED);
    }

    rc = _mqtt_connect(client);
    if (rc) {
        client->network_stack.disconnect(&(client->network_stack));
    }

    IOT_FUNC_EXIT_RC(rc);
}

/**
 * @brief Reconnect MQTT server.
 *
 * @param[in,out] client pointer to mqtt client
 * @return  @see IotReturnCode
 */
int qcloud_iot_mqtt_attempt_reconnect(QcloudIotClient *client)
{
    IOT_FUNC_ENTRY;
    int rc = 0;

    Log_i("attempt to reconnect...");

    if (get_client_conn_state(client)) {
        IOT_FUNC_EXIT_RC(QCLOUD_RET_MQTT_ALREADY_CONNECTED);
    }

    rc = qcloud_iot_mqtt_connect(client);

    if (!get_client_conn_state(client)) {
        IOT_FUNC_EXIT_RC(rc);
    }

    if (!client->options.clean_session) {
        IOT_FUNC_EXIT_RC(QCLOUD_RET_MQTT_RECONNECTED);
    }

    rc = qcloud_iot_mqtt_resubscribe(client);
    if (rc) {
        IOT_FUNC_EXIT_RC(rc);
    }

    IOT_FUNC_EXIT_RC(QCLOUD_RET_MQTT_RECONNECTED);
}

/**
 * @brief Disconnect MQTT server.
 *
 * @param[in,out] client pointer to mqtt client
 * @return  @see IotReturnCode
 */
int qcloud_iot_mqtt_disconnect(QcloudIotClient *client)
{
    IOT_FUNC_ENTRY;
    int rc, packet_len = 0;

    if (!get_client_conn_state(client)) {
        IOT_FUNC_EXIT_RC(QCLOUD_ERR_MQTT_NO_CONN);
    }

    HAL_MutexLock(client->lock_write_buf);
    packet_len = mqtt_disconnect_packet_serialize(client->write_buf, client->write_buf_size);
    if (packet_len > 0) {
        rc = send_mqtt_packet(client, packet_len);
    } else {
        rc = packet_len == MQTT_ERR_SHORT_BUFFER ? QCLOUD_ERR_BUF_TOO_SHORT : QCLOUD_ERR_FAILURE;
    }
    HAL_MutexUnlock(client->lock_write_buf);
    if (rc) {
        IOT_FUNC_EXIT_RC(rc);
    }

    client->network_stack.disconnect(&(client->network_stack));
    set_client_conn_state(client, NOTCONNECTED);
    client->was_manually_disconnected = 1;

    Log_i("mqtt disconnect!");

    IOT_FUNC_EXIT_RC(QCLOUD_RET_SUCCESS);
}

/**
 * @brief Serialize and send pingreq packet.
 *
 * @param[in,out] client pointer to mqtt client
 * @param[in] try_times if failed, retry times
 * @return @see IotReturnCode
 */
int qcloud_iot_mqtt_pingreq(QcloudIotClient *client, int try_times)
{
    IOT_FUNC_ENTRY;
    int rc, packet_len, i = 0;

    HAL_MutexLock(client->lock_write_buf);
    packet_len = mqtt_pingreq_packet_serialize(client->write_buf, client->write_buf_size);
    if (packet_len > 0) {
        do {
            rc = send_mqtt_packet(client, packet_len);
        } while (rc && (i++ < try_times));
    } else {
        rc = packet_len == MQTT_ERR_SHORT_BUFFER ? QCLOUD_ERR_BUF_TOO_SHORT : QCLOUD_ERR_FAILURE;
    }
    HAL_MutexUnlock(client->lock_write_buf);
    IOT_FUNC_EXIT_RC(rc);
}
