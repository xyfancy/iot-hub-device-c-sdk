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
 * @file mqtt_client.h
 * @brief mqtt client internel api
 * @author fancyxu (fancyxu@tencent.com)
 * @version 1.0
 * @date 2021-05-31
 *
 * @par Change Log:
 * <table>
 * <tr><th>Date       <th>Version <th>Author    <th>Description
 * <tr><td>2021-05-31 <td>1.0     <td>fancyxu   <td>first commit
 * </table>
 */

#ifndef QCLOUD_IOT_HUB_SDK_EMBEDDED_C_SERVICES_MQTT_CLIENT_INC_MQTT_CLIENT_H_
#define QCLOUD_IOT_HUB_SDK_EMBEDDED_C_SERVICES_MQTT_CLIENT_INC_MQTT_CLIENT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "qcloud_iot_mqtt_client.h"

#include "mqtt_packet.h"

#include "network_interface.h"

#include "utils_list.h"
#include "utils_base64.h"
#include "utils_hmac.h"

/**
 * @brief Packet id, random from [1 - 65536]
 *
 */
#define MAX_PACKET_ID (65535)

/**
 * @brief Max size of conn Id
 *
 */
#define MAX_CONN_ID_LEN (6)

/**
 * @brief Max number of topic subscribed
 *
 */
#define MAX_MESSAGE_HANDLERS (10)

/**
 * @brief Max number in repub list
 *
 */
#define MAX_REPUB_NUM (20)

/**
 * @brief Minimal wait interval when reconnect
 *
 */
#define MIN_RECONNECT_WAIT_INTERVAL (1000)

/**
 * @brief Minimal MQTT timeout value
 *
 */
#define MIN_COMMAND_TIMEOUT (500)

/**
 * @brief Maxmal MQTT timeout value
 *
 */
#define MAX_COMMAND_TIMEOUT (20000)

/**
 * @brief Max size of a topic name
 *
 */
#define MAX_SIZE_OF_CLOUD_TOPIC ((MAX_SIZE_OF_DEVICE_NAME) + (MAX_SIZE_OF_PRODUCT_ID) + 64 + 6)

/**
 * @brief Minimal TLS handshaking timeout value (unit: ms)
 *
 */
#define QCLOUD_IOT_TLS_HANDSHAKE_TIMEOUT (5 * 1000)

/**
 * @brief Enable repeat packet id filter
 *
 */
#define MQTT_RMDUP_MSG_ENABLED

/**
 * @brief Connect status of mqtt server
 *
 */
typedef enum {
    NOTCONNECTED = 0,
    CONNECTED    = 1,
} ConnStatus;

/**
 * @brief data structure for topic subscription handle
 */
typedef struct {
    char *          topic_filter; /**< topic name, wildcard filter is supported */
    SubscribeParams params;       /**< params needed to subscribe */
} SubTopicHandle;

/**
 * @brief MQTT QCloud IoT Client structure
 *
 */
typedef struct {
    DeviceInfo *device_info; /**< device info needed to connect server */

    uint16_t next_packet_id;     /**< MQTT random packet id */
    uint32_t command_timeout_ms; /**< MQTT command timeout, unit:ms */

    uint8_t write_buf[QCLOUD_IOT_MQTT_TX_BUF_LEN]; /**< MQTT write buffer */
    uint8_t read_buf[QCLOUD_IOT_MQTT_RX_BUF_LEN];  /**< MQTT read buffer */
    size_t  write_buf_size;                        /**< size of MQTT write buffer */
    size_t  read_buf_size;                         /**< size of MQTT read buffer */

    MQTTEventHandler event_handle;        /**< callback for MQTT event */
    uint8_t          auto_connect_enable; /**< enable auto connection or not */

    void *lock_generic;      /**< mutex/lock for this client struture */
    void *lock_write_buf;    /**< mutex/lock for write buffer */
    void *list_pub_wait_ack; /**< puback waiting list */
    void *list_sub_wait_ack; /**< suback waiting list */

    char    host_addr[HOST_STR_LENGTH]; /**< MQTT server host */
    Network network_stack;              /**< MQTT network stack */

    MQTTPacketConnectOption options;                  /**< handle to connection parameters */
    char                    conn_id[MAX_CONN_ID_LEN]; /**< connect id */

    SubTopicHandle sub_handles[MAX_MESSAGE_HANDLERS]; /**< subscription handle array */
    Timer          ping_timer;                        /**< MQTT ping timer */
    Timer          reconnect_delay_timer;             /**< MQTT reconnect delay timer */
    uint8_t        was_manually_disconnected;         /**< was disconnect by server or device */
    uint8_t        is_ping_outstanding;             /**< 1 = ping request is sent while ping response not arrived yet */
    uint32_t       current_reconnect_wait_interval; /**< unit:ms */

    uint8_t  is_connected;                 /**< is connected or not */
    uint32_t counter_network_disconnected; /**< number of disconnection*/

#ifdef MQTT_RMDUP_MSG_ENABLED
#define MQTT_MAX_REPEAT_BUF_LEN 10
    uint16_t     repeat_packet_id_buf[MQTT_MAX_REPEAT_BUF_LEN]; /**< repeat packet id buffer */
    unsigned int current_packet_id_cnt;                         /**< index of packet id buffer */
#endif
} Qcloud_IoT_Client;

/**
 * @brief topic publish info
 *
 */
typedef struct {
    uint8_t *buf;            /**< msg buffer */
    uint32_t len;            /**< msg length */
    uint16_t packet_id;      /**< packet id */
    Timer    pub_start_time; /**< timer for puback waiting */
} QcloudIotPubInfo;

/**
 * @brief topic subscribe/unsubscribe info
 *
 */
typedef struct {
    uint8_t *      buf;            /**< msg buffer */
    uint16_t       len;            /**< msg length */
    MQTTPacketType type;           /**< type: sub or unsub */
    uint16_t       packet_id;      /**< packet id */
    Timer          sub_start_time; /**< timer for suback waiting */
    SubTopicHandle handler;        /**< handle of topic subscribed(unsubscribed) */
} QcloudIotSubInfo;

/**************************************************************************************
 * common
 **************************************************************************************/

/**
 * @brief Get the next packet id object.
 *
 * @param[in,out] client pointer to mqtt client
 * @return uint16_t
 */
uint16_t get_next_packet_id(Qcloud_IoT_Client *client);

/**
 * @brief Get the next conn id object.
 *
 * @param[out] conn_id buffer to save conn id
 */
void get_next_conn_id(char *conn_id);

/**
 * @brief Set the client conn state object.
 *
 * @param[in,out] client pointer to mqtt client
 * @param[in] connected connect status, @see ConnStatus
 */
void set_client_conn_state(Qcloud_IoT_Client *client, uint8_t connected);

/**
 * @brief Get the client conn state object.
 *
 * @param[in,out] client
 * @return @see ConnStatus
 */
uint8_t get_client_conn_state(Qcloud_IoT_Client *client);

/**
 * @brief Send mqtt packet, timeout = command_timeout_ms.
 *
 * @param[in,out] client pointer to mqtt client
 * @param[in] length length of data to be sent, data is saved in client write_buf
 * @return @see IoT_Return_Code
 */
int send_mqtt_packet(Qcloud_IoT_Client *client, size_t length);

/**************************************************************************************
 * connect
 **************************************************************************************/

/**
 * @brief Connect MQTT server.
 *
 * @param[in,out] client pointer to mqtt client
 * @return  @see IoT_Return_Code
 */
int qcloud_iot_mqtt_connect(Qcloud_IoT_Client *client);

/**
 * @brief Reconnect MQTT server.
 *
 * @param[in,out] client pointer to mqtt client
 * @return  @see IoT_Return_Code
 */
int qcloud_iot_mqtt_attempt_reconnect(Qcloud_IoT_Client *client);

/**
 * @brief Disconnect MQTT server.
 *
 * @param[in,out] client pointer to mqtt client
 * @return  @see IoT_Return_Code
 */
int qcloud_iot_mqtt_disconnect(Qcloud_IoT_Client *client);

/**
 * @brief Serialize and send pingreq packet.
 *
 * @param[in,out] client pointer to mqtt client
 * @param[in] try_times if failed, retry times
 * @return @see IoT_Return_Code
 */
int qcloud_iot_mqtt_pingreq(Qcloud_IoT_Client *client, int try_times);

/**************************************************************************************
 * publish
 **************************************************************************************/

/**
 * @brief Serialize and send publish packet.
 *
 * @param[in,out] client pointer to mqtt_client
 * @param[in] topic_name topic to publish
 * @param[in] params publish params
 * @return >=0 for packet id, < 0 for failed @see IoT_Return_Code
 */
int qcloud_iot_mqtt_publish(Qcloud_IoT_Client *client, const char *topic_name, const PublishParams *params);

/**
 * @brief Deserialize publish packet and deliver_message.
 *
 * @param[in,out] client pointer to mqtt_client
 * @param[in] params publish params
 * @return >=0 for packet id, < 0 for failed @see IoT_Return_Code
 */
int qcloud_iot_mqtt_handle_publish(Qcloud_IoT_Client *client);

/**
 * @brief Deserialize puback packet.
 *
 * @param[in,out] client pointer to mqtt_client
 * @return 0 for success.
 */
int qcloud_iot_mqtt_handle_puback(Qcloud_IoT_Client *client);

/**
 * @brief Process puback waiting timout.
 *
 * @param[in,out] client pointer to mqtt_client
 */
void qcloud_iot_mqtt_check_pub_timeout(Qcloud_IoT_Client *client);

/**************************************************************************************
 * subscribe
 **************************************************************************************/

/**
 * @brief Serialize and send subscribe packet.
 *
 * @param[in,out] client pointer to mqtt client
 * @param[in] topic_filter topic to subscribe
 * @param[in] params subscribe params
 * @return >=0 for packet id, < 0 for failed @see IoT_Return_Code
 */
int qcloud_iot_mqtt_subscribe(Qcloud_IoT_Client *client, const char *topic_filter, const SubscribeParams *params);

/**
 * @brief Deserialize suback packet and return sub result.
 *
 * @param[in,out] client pointer to mqtt client
 * @return @see IoT_Return_Code
 */
int qcloud_iot_mqtt_handle_suback(Qcloud_IoT_Client *client);

/**
 * @brief Serialize and send unsubscribe packet.
 *
 * @param[in,out] client pointer to mqtt client
 * @param[in] topic_filter topic to unsubscribe
 * @return >=0 packet id, < 0 for failed @see IoT_Return_Code
 */
int qcloud_iot_mqtt_unsubscribe(Qcloud_IoT_Client *client, const char *topic_filter);

/**
 * @brief Deserialize unsuback packet and remove from list.
 *
 * @param[in,out] client pointer to mqtt client
 * @return @see IoT_Return_Code
 */
int qcloud_iot_mqtt_handle_unsuback(Qcloud_IoT_Client *client);

/**
 * @brief Process suback waiting timeout.
 *
 * @param[in,out] client pointer to mqtt client
 */
void qcloud_iot_mqtt_check_sub_timeout(Qcloud_IoT_Client *client);

/**
 * @brief Resubscribe topic when reconnect.
 *
 * @param[in,out] client pointer to mqtt client
 * @return @see IoT_Return_Code
 */
int qcloud_iot_mqtt_resubscribe(Qcloud_IoT_Client *client);

/**
 * @brief Return if topic is sub ready.
 *
 * @param[in,out] client pointer to mqtt client
 * @param[in] topic_filter topic filter
 * @return true for ready
 * @return false for not ready
 */
bool qcloud_iot_mqtt_is_sub_ready(Qcloud_IoT_Client *client, const char *topic_filter);

/**************************************************************************************
 * yield
 **************************************************************************************/

/**
 * @brief Check connection and keep alive state, read/handle MQTT message in synchronized way.
 *
 * @param[in,out] client pointer to mqtt client
 * @param[in] timeout_ms timeout value (unit: ms) for this operation
 *
 * @return QCLOUD_RET_SUCCESS when success, QCLOUD_ERR_MQTT_ATTEMPTING_RECONNECT when try reconnecting, others @see
 * IoT_Return_Code
 */
int qcloud_iot_mqtt_yield(Qcloud_IoT_Client *client, uint32_t timeout_ms);

/**
 * @brief Wait read specific mqtt packet, such as connack.
 *
 * @param[in,out] client pointer to mqtt client
 * @param[in] packet_type packet type except to read
 * @return @see IoT_Return_Code
 */
int qcloud_iot_mqtt_wait_for_read(Qcloud_IoT_Client *client, uint8_t packet_type);

#ifdef __cplusplus
}
#endif

#endif  // QCLOUD_IOT_HUB_SDK_EMBEDDED_C_SERVICES_MQTT_CLIENT_INC_MQTT_CLIENT_H_
