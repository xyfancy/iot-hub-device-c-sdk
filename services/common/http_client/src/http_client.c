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
 * @file http_client.c
 * @brief
 * @author fancyxu (fancyxu@tencent.com)
 * @version 1.0
 * @date 2021-10-22
 *
 * @par Change Log:
 * <table>
 * <tr><th>Date       <th>Version <th>Author    <th>Description
 * <tr><td>2021-10-22 <td>1.0     <td>fancyxu   <td>first commit
 * </table>
 */

#include "qcloud_iot_http_client.h"

#include "network_interface.h"

/**
 * @brief Http data.
 *
 */
typedef struct {
    const char *data;
    int         data_len;
} HTTPData;

/**
 * @brief Only focus on response code and content,
 * @ref https://datatracker.ietf.org/doc/html/rfc7231
 *
 */
typedef struct {
    int      status_code;     /**< https://en.wikipedia.org/wiki/List_of_HTTP_status_codes */
    int      content_length;  /**< content length */
    uint8_t *content_buf;     /**< content buffer */
    int      content_buf_len; /**< content buffer length */
    int      recv_len;
    int      need_recv_len;
} IotHTTPResponseData;

/**
 * @brief HTTP client.
 *
 */
typedef struct {
    IotHTTPResponseData response;
    Network             network;
} IotHTTPClient;

/**************************************************************************************
 * network
 **************************************************************************************/

/**
 * @brief Connect with host.
 *
 * @param[in,out] client pointer to http client. @see IotHTTPClient
 * @param[in] host http host
 * @param[in] port http port, 80(http)/443(https)
 * @param[in] ca_crt https ca
 * @return 0 for success. others @see IotReturnCode
 */
static int _http_client_connect(IotHTTPClient *client, const char *host, int port, const char *ca_crt)
{
    int rc = 0;

    char port_str[6];
    HAL_Snprintf(port_str, 6, "%d", port);

    client->network.type = NETWORK_TCP;
#if !defined(AUTH_WITH_NO_TLS) && defined(AUTH_MODE_CERT)
    if (ca_crt) {
        // TODO: support https
    }
#endif
    client->network.host = host;
    client->network.port = port_str;

    rc = network_init(&(client->network));
    return rc ? rc : client->network.connect(&(client->network));
}

/**
 * @brief Disconnect with host.
 *
 * @param[in,out] client pointer to http client. @see IotHTTPClient
 */
static void _http_client_disconnect(IotHTTPClient *client)
{
    return client->network.disconnect(&(client->network));
}

/**
 * @brief Send data to http server.
 *
 * @param[in,out] client pointer to http client. @see IotHTTPClient
 * @param[in] buf buffer tot send
 * @param[in] len buffer length
 * @return 0 for success. others @see IotReturnCode
 */
static int _http_client_send(IotHTTPClient *client, char *buf, uint32_t len)
{
    size_t written_len;
    return client->network.write(&(client->network), (uint8_t *)buf, len, HTTP_WRITE_TIMEOUT_MS, &written_len);
}

/**
 * @brief Recv data from http server.
 *
 * @param[in,out] client pointer to http client. @see IotHTTPClient
 * @param[out] buf
 * @param[in] len
 * @param[in] timeout_ms
 * @return >0 for read data length, others @see IotReturnCode
 */
static int _http_client_recv(IotHTTPClient *client, uint8_t *buf, uint32_t len, uint32_t timeout_ms)
{
    size_t read_len;

    int rc = client->network.read(&client->network, buf, len, timeout_ms, &read_len);
    switch (rc) {
        case QCLOUD_ERR_TCP_NOTHING_TO_READ:
            return QCLOUD_RET_SUCCESS;
        default:
            return rc ? rc : read_len;
    }
}

/**************************************************************************************
 * request
 **************************************************************************************/

/**
 * @brief Parse url and get host & path.
 *
 * @param[in] url url str
 * @param[out] host pointer to host str and length. @see HTTPData
 * @param[out] path pointer to path str and length. @see HTTPData
 * @return 0 for success. others @see IotReturnCode
 */
static int _http_client_parse_url(const char *url, HTTPData *host, HTTPData *path)
{
    // start from: http:// or https://
    char *host_ptr = (char *)strstr(url, "://");
    if (!host_ptr) {
        return QCLOUD_ERR_HTTP_PARSE;
    }
    host->data = host_ptr + 3;

    path->data = strchr(host->data, '/');
    if (!path->data) {
        host->data_len = url + strlen(url) - host->data;
        path->data_len = 1;
        path->data     = "/";
        return QCLOUD_RET_SUCCESS;
    }

    host->data_len = path->data - host->data;

    char *fragment_ptr = strchr(host_ptr, '#');
    path->data_len     = fragment_ptr ? fragment_ptr - path->data : strlen(path->data);
    return QCLOUD_RET_SUCCESS;
}

/**
 * @brief Send http request line.
 *
 * @param[in,out] client pointer to http client. @see IotHTTPClient
 * @param[in] params params needed to send request to http server, @see IotHTTPRequestParams
 * @return int
 */
static int _http_client_send_request_line(IotHTTPClient *client, const IotHTTPRequestParams *params)
{
    int      len, rc, buf_len;
    char *   buf;
    HTTPData host, path;

    /**
     * @brief order @see IotHTTPMethod
     *
     */
    const char *method_str[] = {"GET", "POST", "PUT", "DELETE", "HEAD"};

    rc = _http_client_parse_url(params->url, &host, &path);
    if (rc) {
        Log_e("http parse url failed %d", rc);
        return rc;
    }

    buf_len = path.data_len + host.data_len + 128;
    buf     = HAL_Malloc(buf_len);
    if (!buf) {
        Log_e("http malloc request line failed %d", rc);
        return QCLOUD_ERR_MALLOC;
    }

    len = HAL_Snprintf(buf, buf_len, "%s %.*s HTTP/1.1\r\nHost:%.*s\r\n", method_str[params->method], path.data_len,
                       path.data, host.data_len, host.data);
    rc  = _http_client_send(client, buf, len);
    HAL_Free(buf);
    return rc;
}

/**
 * @brief Send http request content(optional).
 *
 * @param[in,out] client pointer to http client. @see IotHTTPClient
 * @param[in] params params needed to send request to http server, @see IotHTTPRequestParams
 * @return int
 */
static int _http_client_send_request_content(IotHTTPClient *client, const IotHTTPRequestParams *params)
{
    int   rc = 0, len, buf_len;
    char *buf;

    buf_len = 32 + strlen(params->content_type);
    buf     = HAL_Malloc(buf_len);
    if (!buf) {
        Log_e("http malloc request line failed %d", rc);
        return QCLOUD_ERR_MALLOC;
    }

    len = HAL_Snprintf(buf, buf_len, "Content-Length:%d\r\n", params->content_length);
    rc  = _http_client_send(client, buf, len);
    if (rc) {
        goto exit;
    }

    if (params->content_type) {
        len = HAL_Snprintf(buf, buf_len, "Content-Type:%s\r\n", params->content_type);
        rc  = _http_client_send(client, buf, len);
        if (rc) {
            goto exit;
        }
    }

    rc = _http_client_send(client, "\r\n", 2);
    if (rc) {
        goto exit;
    }

    return _http_client_send(client, (char *)params->content, params->content_length);
exit:
    Log_e("send request content failed %d", rc);
    return rc;
}

/**
 * @brief Send http request to http server.
 *
 * @param[in,out] client pointer to http client. @see IotHTTPClient
 * @param[in] params @see IotHTTPRequestParams
 * @return 0 for success. others @see IotReturnCode
 */
static int _http_client_send_request(IotHTTPClient *client, const IotHTTPRequestParams *params)
{
    int rc;

    // 1. send request line
    rc = _http_client_send_request_line(client, params);
    if (rc) {
        Log_e("http send request line failed, rc=%d", rc);
        return rc;
    }

    // 2. send request header
    if (params->header) {
        rc = _http_client_send(client, (char *)params->header, strlen(params->header));
        if (rc) {
            Log_e("write failed %d", rc);
            return rc;
        }
    }
    if (!params->content) {  // no payload body
        return _http_client_send(client, "\r\n", 2);
    }

    // 3. send payload body
    return _http_client_send_request_content(client, params);
}

/**************************************************************************************
 * response
 **************************************************************************************/

/**
 * @brief Recv content data.
 *
 * @param[in,out] client pointer to http client. @see IotHTTPClient
 * @param[in] offset offset of content buf
 * @param[in] timeout_ms timeout for recv
 * @return length of content data recv stored in content buf
 */
static int _http_client_recv_content(IotHTTPClient *client, int offset, uint32_t timeout_ms)
{
    IOT_FUNC_ENTRY;

    int rc = 0;

    uint8_t *buf     = client->response.content_buf;
    int      buf_len = client->response.content_buf_len;

    IotHTTPResponseData *response = &client->response;

    if (response->need_recv_len <= 0) {
        // return offset for data already received
        return offset;
    }

    // need recv length may much longger than buffer length
    int len = response->need_recv_len > buf_len - offset ? buf_len - offset : response->need_recv_len;

    rc = _http_client_recv(client, buf + offset, len, timeout_ms);
    if (rc <= 0) {
        return rc;
    }
    response->recv_len += rc;
    response->need_recv_len -= rc;
    return rc + offset;
}

/**
 * @brief Recv response from http server.
 *
 * @param[in,out] client pointer to http client. @see IotHTTPClient
 * @param[in] timeout_ms timeout for recv
 * @return 0 for success. others @see IotReturnCode
 */
static int _http_client_recv_response(IotHTTPClient *client, uint32_t timeout_ms)
{
    Timer timer;
    char *content_length, *body_end;
    int   rc, len = 0;
    char *buf     = (char *)client->response.content_buf;
    int   buf_len = client->response.content_buf_len;

    IotHTTPResponseData *response = &client->response;
    memset(buf, 0, buf_len);
    HAL_Timer_CountdownMs(&timer, timeout_ms);

    // 1. found body end
    while (NULL == (body_end = strstr(buf, "\r\n\r\n"))) {
        // timeout
        if (HAL_Timer_Expired(&timer)) {
            return QCLOUD_ERR_HTTP_TIMEOUT;
        }
        // timeout 100ms for header less than buff len
        rc = _http_client_recv(client, (uint8_t *)buf + len, buf_len - len - 1, 100);
        if (rc < 0) {
            Log_e("read failed");
            return rc;
        }
        len += rc;
    }

    body_end += 4;                 // \r\n\r\n
    len = len - (body_end - buf);  // body length

    // 2. get response code
    response->status_code = atoi(buf + 9);
    switch (response->status_code) {
        case 403:
            return QCLOUD_ERR_HTTP_AUTH;
        case 404:
            return QCLOUD_ERR_HTTP_NOT_FOUND;
        default:
            if (response->status_code < 200 || response->status_code >= 400) {
                Log_w("HTTP status code %d", response->status_code);
                return QCLOUD_ERR_HTTP;
            }
            break;
    }

    // 3. parse header
    content_length = strstr(buf, "Content-Length");
    if (!content_length) {
        Log_e("Could not parse header");
        response->content_length = -1;
        return QCLOUD_ERR_HTTP;
    }
    response->content_length = atoi(content_length + strlen("Content-Length: "));

    // 4. remove data and receive body data
    memmove(buf, body_end, len);
    response->recv_len      = len;
    response->need_recv_len = response->content_length - len;
    return _http_client_recv_content(client, len, HAL_Timer_Remain(&timer));
}

/**************************************************************************************
 * API
 **************************************************************************************/

/**
 * @brief Malloc http client.
 *
 * @return pointer to http client
 */
void *IOT_HTTP_Init(void)
{
    return HAL_Malloc(sizeof(IotHTTPClient));
}

/**
 * @brief Free http client.
 *
 */
void IOT_HTTP_Deinit(void *client)
{
    HAL_Free(client);
}

/**
 * @brief Connect http server.
 *
 * @param[in,out] client pointer to http client
 * @param[in] params params needed to connect http server, @see IotHTTPConnectParams
 * @return 0 for success. others @see IotReturnCode
 */
int IOT_HTTP_Connect(void *client, IotHTTPConnectParams *params)
{
    POINTER_SANITY_CHECK(client, QCLOUD_ERR_INVAL);
    IotHTTPClient *http_client = (IotHTTPClient *)client;
    memset(http_client, 0, sizeof(IotHTTPClient));

    int rc;

    // parse url
    HTTPData host, path;
    rc = _http_client_parse_url(params->url, &host, &path);
    if (rc) {
        Log_e("http parse url failed %d", rc);
        return rc;
    }

    // copy host
    char *host_str = HAL_Malloc(host.data_len + 1);
    if (!host_str) {
        return QCLOUD_ERR_MALLOC;
    }
    strncpy(host_str, host.data, host.data_len);
    host_str[host.data_len] = '\0';

    // http connect
    rc = _http_client_connect(http_client, host_str, params->port, params->ca_crt);
    HAL_Free(host_str);
    return rc;
}

/**
 * @brief Request http server.
 *
 * @param[in,out] client pointer to http client
 * @param[in] params params needed to send request to http server, @see IotHTTPRequestParams
 * @return 0 for success. others @see IotReturnCode
 */
int IOT_HTTP_Request(void *client, IotHTTPRequestParams *params)
{
    POINTER_SANITY_CHECK(client, QCLOUD_ERR_INVAL);
    return _http_client_send_request((IotHTTPClient *)client, params);
}

/**
 * @brief Send data to http server.
 *
 * @param[in,out] client pointer to http client
 * @param[in] data data to send
 * @param[out] data_len data len
 * @return 0 for success. others @see IotReturnCode
 */
int IOT_HTTP_Send(void *client, uint8_t *data, int data_len)
{
    POINTER_SANITY_CHECK(client, QCLOUD_ERR_INVAL);
    return _http_client_send((IotHTTPClient *)client, (char *)data, data_len);
}

/**
 * @brief Recv data from http server.
 *
 * @param[in,out] client pointer to http client
 * @param[out] buf buffer to store recv data
 * @param[in] buf_len buffer len
 * @param timeout_ms timeout for recv
 * @return >= 0 for recv data len. others @see IotReturnCode
 */
int IOT_HTTP_Recv(void *client, uint8_t *buf, int buf_len, uint32_t timeout_ms)
{
    POINTER_SANITY_CHECK(client, QCLOUD_ERR_INVAL);
    IotHTTPClient *http_client            = (IotHTTPClient *)client;
    http_client->response.content_buf     = buf;
    http_client->response.content_buf_len = buf_len;
    return http_client->response.need_recv_len ? _http_client_recv_content(http_client, 0, timeout_ms)
                                               : _http_client_recv_response(http_client, timeout_ms);
}

/**
 * @brief Check is recv finished.
 *
 * @param[in,out] client pointer to http client
 * @return true for finished.
 */
int IOT_HTTP_IsRecvFinished(void *client)
{
    POINTER_SANITY_CHECK(client, QCLOUD_ERR_INVAL);
    IotHTTPClient *http_client = (IotHTTPClient *)client;
    return http_client->response.need_recv_len == 0;
}

/**
 * @brief Disconnect http server.
 *
 * @param[in,out] client pointer to http client
 */
void IOT_HTTP_Disconnect(void *client)
{
    POINTER_SANITY_CHECK_RTN(client);
    _http_client_disconnect((IotHTTPClient *)client);
}
