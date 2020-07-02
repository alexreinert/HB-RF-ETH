/* 
 *  webui.cpp is part of the HB-RF-ETH firmware - https://github.com/alexreinert/HB-RF-ETH
 *  
 *  Copyright 2020 Alexander Reinert
 *  
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *  
 *      http://www.apache.org/licenses/LICENSE-2.0
 *  
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include "webui.h"
#include "esp_log.h"
#include "cJSON.h"
#include "esp_ota_ops.h"
#include "tcpip_adapter.h"
#include "mbedtls/base64.h"

static const char *TAG = "WebUI";

#define EMBED_HANDLER(_uri, _resource, _contentType)                   \
    extern const char _resource[] asm("_binary_" #_resource "_start"); \
    extern const size_t _resource##_length asm(#_resource "_length");  \
    esp_err_t _resource##_handler_func(httpd_req_t *req)               \
    {                                                                  \
        httpd_resp_set_type(req, _contentType);                        \
        httpd_resp_set_hdr(req, "Content-Encoding", "gzip");           \
        httpd_resp_send(req, _resource, _resource##_length);           \
        return ESP_OK;                                                 \
    };                                                                 \
    httpd_uri_t _resource##_handler = {                                \
        .uri = _uri,                                                   \
        .method = HTTP_GET,                                            \
        .handler = _resource##_handler_func,                           \
        .user_ctx = NULL};

EMBED_HANDLER("/", index_htm_gzip, "text/html")
EMBED_HANDLER("/about.htm", about_htm_gzip, "text/html")
EMBED_HANDLER("/webui.js", webui_js_gzip, "application/javascript")
EMBED_HANDLER("/jquery.min.js", jquery_min_js_gzip, "application/javascript")
EMBED_HANDLER("/vue.min.js", vue_min_js_gzip, "application/javascript")
EMBED_HANDLER("/bootstrap.min.css", bootstrap_min_css_gzip, "text/css")
EMBED_HANDLER("/bootstrap-vue.min.css", bootstrap_vue_min_css_gzip, "text/css")
EMBED_HANDLER("/bootstrap-vue.min.js", bootstrap_vue_min_js_gzip, "application/javascript")
EMBED_HANDLER("/vuelidate.min.js", vuelidate_min_js_gzip, "application/javascript")
EMBED_HANDLER("/validators.min.js", validators_min_js_gzip, "application/javascript")
EMBED_HANDLER("/vue-i18n.min.js", vue_i18n_min_js_gzip, "application/javascript")
EMBED_HANDLER("/favicon.ico", favicon_ico_gzip, "image/x-icon")

static Settings *_settings;
static LED *_statusLED;
static SysInfo *_sysInfo;
static UpdateCheck *_updateCheck;
static RawUartUdpListener *_rawUartUdpListener;

const char *ip2str(ip4_addr_t addr, ip4_addr_t fallback)
{
    if (addr.addr == IPADDR_ANY || addr.addr == IPADDR_NONE)
    {
        return ip4addr_ntoa(&fallback);
    }
    return ip4addr_ntoa(&addr);
}

const char *ip2str(ip4_addr_t addr)
{
    if (addr.addr == IPADDR_ANY || addr.addr == IPADDR_NONE)
    {
        return "";
    }
    return ip4addr_ntoa(&addr);
}

esp_err_t validate_auth(httpd_req_t *req)
{
    char auth[64] = {0};
    if (httpd_req_get_hdr_value_str(req, "Authorization", auth, sizeof(auth)) != ESP_OK)
        return ESP_FAIL;

    if (strncmp(auth, "Basic ", 6) != 0)
        return ESP_FAIL;

    char decoded[64] = {0};
    size_t decodedLength;
    if (mbedtls_base64_decode((unsigned char *)decoded, sizeof(decoded), &decodedLength, (unsigned char *)auth + 6, strlen(auth + 6)) != 0)
        return ESP_FAIL;

    decoded[decodedLength] = 0;

    if (strncmp(decoded, "admin:", 6) != 0)
        return ESP_FAIL;

    if (strcmp(decoded + 6, _settings->getAdminPassword()) != 0)
        return ESP_FAIL;

    return ESP_OK;
}

void add_settings(cJSON *root)
{
    cJSON *settings = cJSON_AddObjectToObject(root, "settings");

    cJSON_AddStringToObject(settings, "adminPassword", "");
    cJSON_AddStringToObject(settings, "adminPasswordRepeat", "");

    cJSON_AddStringToObject(settings, "hostname", _settings->getHostname());
    tcpip_adapter_ip_info_t ipInfo;
    tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_ETH, &ipInfo);
    cJSON_AddBoolToObject(settings, "useDHCP", _settings->getUseDHCP());
    cJSON_AddStringToObject(settings, "localIP", ip2str(_settings->getLocalIP(), ipInfo.ip));
    cJSON_AddStringToObject(settings, "netmask", ip2str(_settings->getNetmask(), ipInfo.netmask));
    cJSON_AddStringToObject(settings, "gateway", ip2str(_settings->getGateway(), ipInfo.gw));
    tcpip_adapter_dns_info_t dnsInfo;
    tcpip_adapter_get_dns_info(TCPIP_ADAPTER_IF_ETH, TCPIP_ADAPTER_DNS_MAIN, &dnsInfo);
    cJSON_AddStringToObject(settings, "dns1", ip2str(_settings->getDns1(), dnsInfo.ip.u_addr.ip4));
    tcpip_adapter_get_dns_info(TCPIP_ADAPTER_IF_ETH, TCPIP_ADAPTER_DNS_BACKUP, &dnsInfo);
    if (dnsInfo.ip.u_addr.ip4.addr != IPADDR_ANY)
    {
        cJSON_AddStringToObject(settings, "dns2", ip2str(_settings->getDns2(), dnsInfo.ip.u_addr.ip4));
    }

    cJSON_AddNumberToObject(settings, "timesource", _settings->getTimesource());

    cJSON_AddNumberToObject(settings, "dcfOffset", _settings->getDcfOffset());

    cJSON_AddNumberToObject(settings, "gpsBaudrate", _settings->getGpsBaudrate());

    cJSON_AddStringToObject(settings, "ntpServer", _settings->getNtpServer());

    cJSON_AddNumberToObject(settings, "ledBrightness", _settings->getLEDBrightness());
}

void add_sysInfo(cJSON *root)
{
    cJSON *sysinfo = cJSON_AddObjectToObject(root, "sysInfo");

    cJSON_AddStringToObject(sysinfo, "serial", _sysInfo->getSerialNumber());
    cJSON_AddStringToObject(sysinfo, "currentVersion", _sysInfo->getCurrentVersion());
    cJSON_AddStringToObject(sysinfo, "latestVersion", _updateCheck->getLatestVersion());

    cJSON_AddStringToObject(sysinfo, "rawUartRemoteAddress", ip2str(_rawUartUdpListener->getConnectedRemoteAddress()));

    cJSON_AddNumberToObject(sysinfo, "memoryUsage", _sysInfo->getMemoryUsage());
    cJSON_AddNumberToObject(sysinfo, "cpuUsage", _sysInfo->getCpuUsage());
}

esp_err_t get_data_json_handler_func(httpd_req_t *req)
{
    bool isAuthenticated = (validate_auth(req) == ESP_OK);

    httpd_resp_set_type(req, "application/json");
    cJSON *root = cJSON_CreateObject();

    cJSON_AddBoolToObject(root, "loggedIn", isAuthenticated);

    add_sysInfo(root);

    if (isAuthenticated)
        add_settings(root);

    const char *json = cJSON_Print(root);
    httpd_resp_sendstr(req, json);
    free((void *)json);
    cJSON_Delete(root);

    return ESP_OK;
}

httpd_uri_t get_data_json_handler = {
    .uri = "/getData.json",
    .method = HTTP_GET,
    .handler = get_data_json_handler_func,
    .user_ctx = NULL};

ip4_addr_t cJSON_GetIPAddrValue(const cJSON *item)
{
    ip4_addr_t res{.addr = IPADDR_ANY};

    if (cJSON_IsString(item))
    {
        ip4addr_aton(item->valuestring, &res);
    }

    return res;
}

bool cJSON_GetBoolValue(const cJSON *item)
{
    if (cJSON_IsBool(item))
    {
        return item->type == cJSON_True;
    }

    return false;
}

esp_err_t post_settings_json_handler_func(httpd_req_t *req)
{
    if (validate_auth(req) != ESP_OK)
    {
        httpd_resp_set_status(req, "401 Not authorized");
        httpd_resp_sendstr(req, "401 Not authorized");
        return ESP_OK;
    }

    char buffer[1024];
    int len = httpd_req_recv(req, buffer, sizeof(buffer) - 1);

    if (len > 0)
    {
        buffer[len] = 0;
        cJSON *root = cJSON_Parse(buffer);

        char *adminPassword = cJSON_GetStringValue(cJSON_GetObjectItem(root, "adminPassword"));

        char *hostname = cJSON_GetStringValue(cJSON_GetObjectItem(root, "hostname"));
        bool useDHCP = cJSON_GetBoolValue(cJSON_GetObjectItem(root, "useDHCP"));
        ip4_addr_t localIP = cJSON_GetIPAddrValue(cJSON_GetObjectItem(root, "localIP"));
        ip4_addr_t netmask = cJSON_GetIPAddrValue(cJSON_GetObjectItem(root, "netmask"));
        ip4_addr_t gateway = cJSON_GetIPAddrValue(cJSON_GetObjectItem(root, "gateway"));
        ip4_addr_t dns1 = cJSON_GetIPAddrValue(cJSON_GetObjectItem(root, "dns1"));
        ip4_addr_t dns2 = cJSON_GetIPAddrValue(cJSON_GetObjectItem(root, "dns2"));

        timesource_t timesource = (timesource_t)cJSON_GetObjectItem(root, "timesource")->valueint;

        int dcfOffset = cJSON_GetObjectItem(root, "dcfOffset")->valueint;

        int gpsBaudrate = cJSON_GetObjectItem(root, "gpsBaudrate")->valueint;

        char *ntpServer = cJSON_GetStringValue(cJSON_GetObjectItem(root, "ntpServer"));

        int ledBrightness = cJSON_GetObjectItem(root, "ledBrightness")->valueint;

        if (adminPassword && strlen(adminPassword) > 0)
            _settings->setAdminPassword(adminPassword);

        _settings->setNetworkSettings(hostname, useDHCP, localIP, netmask, gateway, dns1, dns2);
        _settings->setTimesource(timesource);
        _settings->setDcfOffset(dcfOffset);
        _settings->setGpsBaudrate(gpsBaudrate);
        _settings->setNtpServer(ntpServer);
        _settings->setLEDBrightness(ledBrightness);

        _settings->save();

        cJSON_Delete(root);

        httpd_resp_set_type(req, "application/json");
        root = cJSON_CreateObject();

        add_settings(root);

        const char *json = cJSON_Print(root);
        httpd_resp_sendstr(req, json);
        free((void *)json);
        cJSON_Delete(root);

        return ESP_OK;
    }

    return ESP_FAIL;
}

httpd_uri_t post_settings_json_handler = {
    .uri = "/settings.json",
    .method = HTTP_POST,
    .handler = post_settings_json_handler_func,
    .user_ctx = NULL};

#define OTA_CHECK(a, str, ...)                                                    \
    do                                                                            \
    {                                                                             \
        if (!(a))                                                                 \
        {                                                                         \
            ESP_LOGE(TAG, "%s(%d): " str, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, str);       \
            goto err;                                                             \
        }                                                                         \
    } while (0)

esp_err_t post_ota_update_handler_func(httpd_req_t *req)
{
    if (validate_auth(req) != ESP_OK)
    {
        httpd_resp_set_status(req, "401 Not authorized");
        httpd_resp_sendstr(req, "401 Not authorized");
        return ESP_OK;
    }

    esp_ota_handle_t ota_handle;

    char ota_buff[1024];
    int content_length = req->content_len;
    int content_received = 0;
    int recv_len;
    bool is_req_body_started = false;
    const esp_partition_t *update_partition = esp_ota_get_next_update_partition(NULL);

    do
    {
        if ((recv_len = httpd_req_recv(req, ota_buff, MIN(content_length, sizeof(ota_buff)))) < 0)
        {
            if (recv_len != HTTPD_SOCK_ERR_TIMEOUT)
            {
                ESP_LOGE(TAG, "OTA socket Error %d", recv_len);
                return ESP_FAIL;
            }
        }

        if (!is_req_body_started)
        {
            is_req_body_started = true;

            char *body_start_p = strstr(ota_buff, "\r\n\r\n") + 4;
            int body_part_len = recv_len - (body_start_p - ota_buff);

            OTA_CHECK(esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &ota_handle) == ESP_OK, "Could not start OTA");
            ESP_LOGW(TAG, "Begin OTA Update to partition %s, File Size: %d", update_partition->label, content_length);
            _statusLED->setState(LED_STATE_BLINK_FAST);

            OTA_CHECK(esp_ota_write(ota_handle, body_start_p, body_part_len) == ESP_OK, "Error writing OTA");
        }
        else
        {
            OTA_CHECK(esp_ota_write(ota_handle, ota_buff, recv_len) == ESP_OK, "Error writing OTA");
            content_received += recv_len;
        }
    } while ((recv_len > 0) && (content_received < content_length));

    OTA_CHECK(esp_ota_end(ota_handle) == ESP_OK, "Error writing OTA");
    OTA_CHECK(esp_ota_set_boot_partition(update_partition) == ESP_OK, "Error writing OTA");

    ESP_LOGI(TAG, "OTA finished, please restart to activate new firmware.");
    httpd_resp_sendstr(req, "Firmware update completed, please restart to activate new firmware.");

    _statusLED->setState(LED_STATE_OFF);
    return ESP_OK;

err:
    _statusLED->setState(LED_STATE_OFF);
    return ESP_FAIL;
}

httpd_uri_t post_ota_update_handler = {
    .uri = "/ota_update",
    .method = HTTP_POST,
    .handler = post_ota_update_handler_func,
    .user_ctx = NULL};

WebUI::WebUI(Settings *settings, LED *statusLED, SysInfo *sysInfo, UpdateCheck *updateCheck, RawUartUdpListener *rawUartUdpListener)
{
    _settings = settings;
    _statusLED = statusLED;
    _sysInfo = sysInfo;
    _updateCheck = updateCheck;
    _rawUartUdpListener = rawUartUdpListener;
}

void WebUI::start()
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 15;
    httpd_handle_t _httpd_handle = NULL;

    if (httpd_start(&_httpd_handle, &config) == ESP_OK)
    {
        httpd_register_uri_handler(_httpd_handle, &index_htm_gzip_handler);
        httpd_register_uri_handler(_httpd_handle, &about_htm_gzip_handler);
        httpd_register_uri_handler(_httpd_handle, &webui_js_gzip_handler);
        httpd_register_uri_handler(_httpd_handle, &jquery_min_js_gzip_handler);
        httpd_register_uri_handler(_httpd_handle, &vue_min_js_gzip_handler);
        httpd_register_uri_handler(_httpd_handle, &bootstrap_min_css_gzip_handler);
        httpd_register_uri_handler(_httpd_handle, &bootstrap_vue_min_css_gzip_handler);
        httpd_register_uri_handler(_httpd_handle, &bootstrap_vue_min_js_gzip_handler);
        httpd_register_uri_handler(_httpd_handle, &vuelidate_min_js_gzip_handler);
        httpd_register_uri_handler(_httpd_handle, &validators_min_js_gzip_handler);
        httpd_register_uri_handler(_httpd_handle, &vue_i18n_min_js_gzip_handler);
        httpd_register_uri_handler(_httpd_handle, &favicon_ico_gzip_handler);

        httpd_register_uri_handler(_httpd_handle, &post_ota_update_handler);
        httpd_register_uri_handler(_httpd_handle, &post_settings_json_handler);
        httpd_register_uri_handler(_httpd_handle, &get_data_json_handler);
    }
}

void WebUI::stop()
{
    httpd_stop(_httpd_handle);
}