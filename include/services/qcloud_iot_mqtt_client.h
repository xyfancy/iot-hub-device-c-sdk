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
 * @file qcloud_iot_mqtt_client.h
 * @brief
 * @author fancyxu (fancyxu@tencent.com)
 * @version 1.0
 * @date 2021-05-28
 *
 * @par Change Log:
 * <table>
 * <tr><th>Date       <th>Version <th>Author    <th>Description
 * <tr><td>2021-05-28 <td>1.0     <td>fancyxu   <td>first commit
 * <tr><td>2021-07-07 <td>1.1     <td>fancyxu   <td>support user host for unittest
 * <tr><td>2021-07-08 <td>1.1     <td>fancyxu   <td>fix code standard of IotReturnCode and QcloudIotClient
 * </table>
 */

#ifndef IOT_HUB_DEVICE_C_SDK_INCLUDE_SERVICES_QCLOUD_IOT_MQTT_CLIENT_H_
#define IOT_HUB_DEVICE_C_SDK_INCLUDE_SERVICES_QCLOUD_IOT_MQTT_CLIENT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "qcloud_iot_hub.h"

/**
 * @brief MQTT event type
 *
 */
typedef enum {
    MQTT_EVENT_UNDEF               = 0,  /**< MQTT undefined event */
    MQTT_EVENT_DISCONNECT          = 1,  /**< MQTT disconnect */
    MQTT_EVENT_RECONNECT           = 2,  /**< MQTT reconnect */
    MQTT_EVENT_SUBSCRIBE_SUCCESS   = 3,  /**< MQTT subscribe success */
    MQTT_EVENT_SUBSCRIBE_TIMEOUT   = 4,  /**< MQTT subscribe timeout */
    MQTT_EVENT_SUBSCRIBE_NACK      = 5,  /**< MQTT subscribe fail */
    MQTT_EVENT_UNSUBSCRIBE_SUCCESS = 6,  /**< MQTT unsubscribe success */
    MQTT_EVENT_UNSUBSCRIBE_TIMEOUT = 7,  /**< MQTT unsubscribe timeout */
    MQTT_EVENT_UNSUBSCRIBE_NACK    = 8,  /**< MQTT unsubscribe fail */
    MQTT_EVENT_PUBLISH_SUCCESS     = 9,  /**< MQTT publish success */
    MQTT_EVENT_PUBLISH_TIMEOUT     = 10, /**< MQTT publish timeout */
    MQTT_EVENT_PUBLISH_NACK        = 11, /**< MQTT publish fail */
    MQTT_EVENT_PUBLISH_RECEIVED    = 12, /**< MQTT received msg from server */
    MQTT_EVENT_CLIENT_DESTROY      = 13, /**< MQTT client destroy */
    MQTT_EVENT_UNSUBSCRIBE         = 14, /**< MQTT unsubscribe */
    MQTT_EVENT_GATEWAY_SEARCH      = 15,
} MQTTEventType;

/**
 * @brief MQTT event message
 *
 */
typedef struct {
    MQTTEventType event_type; /**< MQTT event type */
    void *        msg;
} MQTTEventMsg;

/**
 * @brief Define MQTT callback when MQTT event happen
 *
 * @param[in,out] client pointer to mqtt client
 * @param[in] context user callback context, @see MQTTEventHandler
 * @param[in] msg the event message @see MQTTEventMsg
 */
typedef void (*MQTTEventHandleFun)(void *client, void *context, MQTTEventMsg *msg);

/* The structure of MQTT event handle */

/**
 * @brief Define structure to handle mqtt event
 *
 */
typedef struct {
    MQTTEventHandleFun h_fp;
    void *             context;
} MQTTEventHandler;

/**
 * @brief The structure of MQTT init parameters
 *
 */
typedef struct {
    DeviceInfo *     device_info;            /**< device info */
    const char *     host;                   /**< host for user, null for default using QCLOUD_IOT_MQTT_DIRECT_DOMAIN */
    uint32_t         command_timeout;        /**< timeout value (unit: ms) for MQTT connect/pub/sub/yield */
    uint32_t         keep_alive_interval_ms; /**< MQTT keep alive time interval in millisecond */
    uint8_t          clean_session;          /**< flag of clean session, 1 clean, 0 not clean */
    uint8_t          auto_connect_enable;    /**< flag of auto reconnection, 1 is enable and recommended */
    MQTTEventHandler event_handle;           /**< event callback */
} MQTTInitParams;

/**
 * Default MQTT init parameters
 */

#define DEFAULT_MQTT_INIT_PARAMS                  \
    {                                             \
        NULL, NULL, 5000, 240 * 1000, 1, 1, { 0 } \
    }

/**
 * @brief MQTT Quality of Service level
 *
 */
typedef enum {
    QOS0 = 0, /**< At most once delivery */
    QOS1 = 1, /**< At least once delivery, PUBACK is required */
    QOS2 = 2  /**< Exactly once delivery. NOT supported currently */
} QoS;

/**
 * @brief  MQTT message parameter for pub/sub
 *
 */
typedef struct {
    QoS      qos;          // MQTT QoS level
    uint8_t  retain;       // RETAIN flag
    uint8_t  dup;          // DUP flag
    uint16_t packet_id;    // MQTT Id
    char *   topic_name;   // MQTT topic
    int      topic_len;    // topic length
    void *   payload;      // MQTT msg payload
    int      payload_len;  // MQTT length of msg payload
} MQTTMessage;

/**
 * @brief Params needed to publish expcept topic name(as a paramter of function).
 *
 */
typedef struct {
    QoS     qos;          // MQTT QoS level
    uint8_t retain;       // RETAIN flag
    uint8_t dup;          // DUP flag
    void *  payload;      // MQTT msg payload
    int     payload_len;  // MQTT length of msg payload
} PublishParams;

/**
 * @brief Default MQTT publish params
 *
 */
#define DEFAULT_PUB_PARAMS  \
    {                       \
        QOS0, 0, 0, NULL, 0 \
    }

/**
 * @brief Define MQTT SUBSCRIBE callback when message arrived
 *
 * @param[in,out] client pointer to mqtt client
 * @param[in] message publish message from server
 * @param[in] usr_data user data of SubscribeParams, @see SubscribeParams
 */
typedef void (*OnMessageHandler)(void *client, MQTTMessage *message, void *usr_data);

/**
 * @brief Define MQTT SUBSCRIBE callback when event happened
 *
 * @param[in,out] client pointer to mqtt client
 * @param[in] event_type @see MQTTEventType
 * @param[in] usr_data user data of SubscribeParams, @see SubscribeParams
 *
 * @return none
 */
typedef void (*OnSubEventHandler)(void *client, MQTTEventType event_type, void *usr_data);

/**
 * @brief Define structure to do MQTT subscription
 *
 */
typedef struct {
    QoS               qos;                  /**< MQTT QoS level */
    OnMessageHandler  on_message_handler;   /**< callback when message arrived */
    OnSubEventHandler on_sub_event_handler; /**< callback when event happened */
    void *            user_data;            /**< user context for callback */
} SubscribeParams;

/**
 * Default MQTT subscribe parameters
 *
 */
#define DEFAULT_SUB_PARAMS     \
    {                          \
        QOS0, NULL, NULL, NULL \
    }

/**
 * @brief Create MQTT client and connect to MQTT server.
 *
 * @param[in] params MQTT init parameters
 * @return a valid MQTT client handle when success, or NULL otherwise
 */
void *IOT_MQTT_Construct(const MQTTInitParams *params);

/**
 * @brief Close connection and destroy MQTT client.
 *
 * @param client pointer to mqtt client pointer
 * @return @see IotReturnCode
 */
int IOT_MQTT_Destroy(void **pClient);

/**
 * @brief Check connection and keep alive state, read/handle MQTT packet in synchronized way.
 *
 * @param[in,out] client pointer to mqtt client
 * @param[in] timeout_ms timeout value (unit: ms) for this operation
 * @return QCLOUD_RET_SUCCESS when success, QCLOUD_ERR_MQTT_ATTEMPTING_RECONNECT when try reconnecting, others @see
 * IotReturnCode
 */
int IOT_MQTT_Yield(void *pClient, uint32_t timeout_ms);

/**
 * @brief Publish MQTT message.
 *
 * @param[in,out] client pointer to mqtt client
 * @param[in] topic_name topic to publish
 * @param[in] params @see PublishParams
 * @return packet id (>=0) when success, or err code (<0) @see IotReturnCode
 */
int IOT_MQTT_Publish(void *client, const char *topic_name, const PublishParams *params);

/**
 * @brief Subscribe MQTT topic.
 *
 * @param[in,out] client pointer to mqtt client
 * @param[in] topic_filter topic filter to subscribe
 * @param[in] params @see SubscribeParams
 * @return packet id (>=0) when success, or err code (<0) @see IotReturnCode
 */
int IOT_MQTT_Subscribe(void *client, const char *topic_filter, const SubscribeParams *params);

/**
 * @brief Unsubscribe MQTT topic.
 *
 * @param[in,out] client pointer to mqtt client
 * @param[in] topic_filter topic filter to unsubscribe
 * @return packet id (>=0) when success, or err code (<0) @see IotReturnCode
 */
int IOT_MQTT_Unsubscribe(void *client, const char *topic_filter);

/**
 * @brief Check if MQTT topic has been subscribed or not
 *
 * @param[in,out] client pointer to mqtt client
 * @param[in] topic_filter topic filter to subscribe
 * @return true already subscribed
 * @return false not ready
 */
bool IOT_MQTT_IsSubReady(void *client, const char *topic_filter);

/**
 * @brief Check if MQTT is connected.
 *
 * @param[in,out] client pointer to mqtt client
 * @return true connected
 * @return false no connected
 */
bool IOT_MQTT_IsConnected(void *pClient);

/**
 * @brief Get device info using to connect mqtt server.
 *
 * @param[in,out] client pointer to mqtt client
 * @return @see DeviceInfo
 */
DeviceInfo *IOT_MQTT_GetDeviceInfo(void *pClient);

#ifdef __cplusplus
}
#endif

#endif  // IOT_HUB_DEVICE_C_SDK_INCLUDE_SERVICES_QCLOUD_IOT_MQTT_CLIENT_H_
