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
 * @file network_interface.c
 * @brief network interface for tcp/tls
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

#ifdef __cplusplus
extern "C" {
#endif

#include "network_interface.h"

/**
 * @brief TCP init, do nothing.
 *
 * @param[in,out] network pointer to network handle
 * @return always ok
 */
int network_tcp_init(Network *network)
{
    return QCLOUD_RET_SUCCESS;
}

/**
 * @brief TCP connect.
 *
 * @param[in,out] network pointer to network handle
 * @return @see IoT_Return_Code
 */
int network_tcp_connect(Network *network)
{
    POINTER_SANITY_CHECK(network, QCLOUD_ERR_INVAL);

    network->handle = HAL_TCP_Connect(network->host, network->port);
    if (0 == network->handle) {
        return -1;
    }

    return 0;
}

/**
 * @brief TCP read.
 *
 * @param[in,out] network pointer to network handle
 * @param[out] data data buffer to be read in
 * @param[in] datalen data buffer len
 * @param[in] timeout_ms read timeout
 * @param[out] read_len read data len
 * @return @see IoT_Return_Code
 */
int network_tcp_read(Network *network, unsigned char *data, size_t datalen, uint32_t timeout_ms, size_t *read_len)
{
    POINTER_SANITY_CHECK(network, QCLOUD_ERR_INVAL);

    int rc = 0;

    rc = HAL_TCP_Read(network->handle, data, (uint32_t)datalen, timeout_ms, read_len);

    return rc;
}

/**
 * @brief TCP write.
 *
 * @param[in,out] network pointer to network handle
 * @param[out] data data buffer to write
 * @param[in] datalen data buffer len
 * @param[in] timeout_ms write timeout
 * @param[out] written_len len of written data
 * @return @see IoT_Return_Code
 */
int network_tcp_write(Network *network, unsigned char *data, size_t datalen, uint32_t timeout_ms, size_t *written_len)
{
    POINTER_SANITY_CHECK(network, QCLOUD_ERR_INVAL);

    int rc = 0;

    rc = HAL_TCP_Write(network->handle, data, datalen, timeout_ms, written_len);

    return rc;
}

/**
 * @brief TCP disconnect
 *
 * @param[in,out] network pointer to network handle
 */
void network_tcp_disconnect(Network *network)
{
    POINTER_SANITY_CHECK_RTN(network);

    if (0 == network->handle) {
        return;
    }

    HAL_TCP_Disconnect(network->handle);
    network->handle = 0;
    return;
}

#ifndef AUTH_WITH_NOTLS

/**
 * @brief TLS init, do nothing.
 *
 * @param[in,out] network pointer to network handle
 * @return always ok
 */
int network_tls_init(Network *network)
{
    return QCLOUD_RET_SUCCESS;
}

/**
 * @brief TLS connect.
 *
 * @param[in,out] network pointer to network handle
 * @return @see IoT_Return_Code
 */
int network_tls_connect(Network *network)
{
    POINTER_SANITY_CHECK(network, QCLOUD_ERR_INVAL);

    int ret = QCLOUD_ERR_SSL_CONNECT;

    network->handle = (uintptr_t)HAL_TLS_Connect(&(network->ssl_connect_params), network->host, network->port);
    if (network->handle != 0) {
        ret = QCLOUD_RET_SUCCESS;
    }

    return ret;
}

/**
 * @brief TLS read.
 *
 * @param[in,out] network pointer to network handle
 * @param[out] data data buffer to be read in
 * @param[in] datalen data buffer len
 * @param[in] timeout_ms read timeout
 * @param[out] read_len read data len
 * @return @see IoT_Return_Code
 */
int network_tls_read(Network *network, unsigned char *data, size_t datalen, uint32_t timeout_ms, size_t *read_len)
{
    POINTER_SANITY_CHECK(network, QCLOUD_ERR_INVAL);

    int rc = HAL_TLS_Read(network->handle, data, datalen, timeout_ms, read_len);

    return rc;
}

/**
 * @brief TLS write.
 *
 * @param[in,out] network pointer to network handle
 * @param[out] data data buffer to write
 * @param[in] datalen data buffer len
 * @param[in] timeout_ms write timeout
 * @param[out] written_len len of written data
 * @return @see IoT_Return_Code
 */
int network_tls_write(Network *network, unsigned char *data, size_t datalen, uint32_t timeout_ms, size_t *written_len)
{
    POINTER_SANITY_CHECK(network, QCLOUD_ERR_INVAL);

    int rc = HAL_TLS_Write(network->handle, data, datalen, timeout_ms, written_len);

    return rc;
}

/**
 * @brief TLS disconnect
 *
 * @param[in,out] network pointer to network handle
 */
void network_tls_disconnect(Network *network)
{
    POINTER_SANITY_CHECK_RTN(network);

    HAL_TLS_Disconnect(network->handle);
    network->handle = 0;
}

#endif

/**
 * @brief Return handle.
 *
 * @param[in] network pointer to network
 * @return handle of network
 */
int is_network_connected(Network *network)
{
    return network->handle;
}

/**
 * @brief Init network, support tcp, tls(if AUTH_WITH_NOTLS defined).
 *
 * @param network pointer to network
 * @return @see IoT_Return_Code
 */
int network_init(Network *network)
{
    POINTER_SANITY_CHECK(network, QCLOUD_ERR_INVAL);

    switch (network->type) {
        case NETWORK_TCP:
            network->init         = network_tcp_init;
            network->connect      = network_tcp_connect;
            network->read         = network_tcp_read;
            network->write        = network_tcp_write;
            network->disconnect   = network_tcp_disconnect;
            network->is_connected = is_network_connected;
            network->handle       = 0;
            break;

#ifndef AUTH_WITH_NOTLS
        case NETWORK_TLS:
            network->init         = network_tls_init;
            network->connect      = network_tls_connect;
            network->read         = network_tls_read;
            network->write        = network_tls_write;
            network->disconnect   = network_tls_disconnect;
            network->is_connected = is_network_connected;
            network->handle       = 0;
            break;
#endif

        default:
            Log_e("unknown network type: %d", network->type);
            return QCLOUD_ERR_INVAL;
    }
    return network->init(network);
}

#ifdef __cplusplus
}
#endif
