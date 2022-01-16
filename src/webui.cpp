/* 
 *  webui.cpp is part of the HB-RF-ETH firmware - https://github.com/alexreinert/HB-RF-ETH
 *  
 *  Copyright 2021 Alexander Reinert
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
#include "mbedtls/md.h"
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

EMBED_HANDLER("/*", index_html_gz, "text/html")
EMBED_HANDLER("/main.1e43358e.js", main_1e43358e_js_gz, "application/javascript")
EMBED_HANDLER("/main.1e43358e.css", main_1e43358e_css_gz, "text/css")
EMBED_HANDLER("/favicon.26242483.ico", favicon_26242483_ico_gz, "image/x-icon")

static Settings *_settings;
static LED *_statusLED;
static SysInfo *_sysInfo;
static UpdateCheck *_updateCheck;
static Ethernet *_ethernet;
static RawUartUdpListener *_rawUartUdpListener;
static RadioModuleConnector *_radioModuleConnector;
static RadioModuleDetector *_radioModuleDetector;
static char _token[46];

const char *ip2str(ip4_addr_t addr, ip4_addr_t fallback)
{
    if (addr.addr == IPADDR_ANY || addr.addr == IPADDR_NONE)
    {
        if (fallback.addr == IPADDR_ANY || fallback.addr == IPADDR_NONE)
        {
            return "";
        }
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

void formatRadioMAC(uint32_t radioMAC, char *buf)
{
    if (radioMAC == 0)
    {
        sprintf(buf, "n/a");
    }
    else
    {
        sprintf(buf, "0x%06X", radioMAC);
    }
}

esp_err_t validate_auth(httpd_req_t *req)
{
    char auth[60] = {0};
    if (httpd_req_get_hdr_value_str(req, "Authorization", auth, sizeof(auth)) != ESP_OK)
        return ESP_FAIL;

    if (strncmp(auth, "Token ", 6) != 0)
        return ESP_FAIL;

    if (strcmp(auth + 6, _token) != 0)
        return ESP_FAIL;

    return ESP_OK;
}

esp_err_t post_login_json_handler_func(httpd_req_t *req)
{
    char buffer[1024];
    int len = httpd_req_recv(req, buffer, sizeof(buffer) - 1);

    if (len > 0)
    {
        buffer[len] = 0;

        cJSON *root = cJSON_Parse(buffer);

        char *password = cJSON_GetStringValue(cJSON_GetObjectItem(root, "password"));

        bool isAuthenticated = (password != NULL) && (strcmp(password, _settings->getAdminPassword()) == 0);

        cJSON_Delete(root);

        httpd_resp_set_type(req, "application/json");
        root = cJSON_CreateObject();

        cJSON_AddBoolToObject(root, "isAuthenticated", isAuthenticated);
        if (isAuthenticated)
        {
            cJSON_AddStringToObject(root, "token", _token);
        }

        const char *json = cJSON_Print(root);
        httpd_resp_sendstr(req, json);
        free((void *)json);
        cJSON_Delete(root);

        return ESP_OK;
    }

    return ESP_FAIL;
}

httpd_uri_t post_login_json_handler = {
    .uri = "/login.json",
    .method = HTTP_POST,
    .handler = post_login_json_handler_func,
    .user_ctx = NULL};

esp_err_t get_sysinfo_json_handler_func(httpd_req_t *req)
{
    httpd_resp_set_type(req, "application/json");
    cJSON *root = cJSON_CreateObject();

    cJSON *sysinfo = cJSON_AddObjectToObject(root, "sysInfo");

    cJSON_AddStringToObject(sysinfo, "serial", _sysInfo->getSerialNumber());
    cJSON_AddStringToObject(sysinfo, "currentVersion", _sysInfo->getCurrentVersion());
    cJSON_AddStringToObject(sysinfo, "latestVersion", _updateCheck->getLatestVersion());

    cJSON_AddStringToObject(sysinfo, "rawUartRemoteAddress", ip2str(_rawUartUdpListener->getConnectedRemoteAddress()));

    cJSON_AddNumberToObject(sysinfo, "memoryUsage", _sysInfo->getMemoryUsage());
    cJSON_AddNumberToObject(sysinfo, "cpuUsage", _sysInfo->getCpuUsage());

    switch (_radioModuleDetector->getRadioModuleType())
    {
    case RADIO_MODULE_HM_MOD_RPI_PCB:
        cJSON_AddStringToObject(sysinfo, "radioModuleType", "HM-MOD-RPI-PCB");
        break;
    case RADIO_MODULE_RPI_RF_MOD:
        cJSON_AddStringToObject(sysinfo, "radioModuleType", "RPI-RF-MOD");
        break;
    default:
        cJSON_AddStringToObject(sysinfo, "radioModuleType", "-");
        break;
    }
    cJSON_AddStringToObject(sysinfo, "radioModuleSerial", _radioModuleDetector->getSerial());
    char radioMAC[9];
    formatRadioMAC(_radioModuleDetector->getBidCosRadioMAC(), radioMAC);
    cJSON_AddStringToObject(sysinfo, "radioModuleBidCosRadioMAC", radioMAC);
    formatRadioMAC(_radioModuleDetector->getHmIPRadioMAC(), radioMAC);
    cJSON_AddStringToObject(sysinfo, "radioModuleHmIPRadioMAC", radioMAC);
    cJSON_AddStringToObject(sysinfo, "radioModuleSGTIN", _radioModuleDetector->getSGTIN());

    const char *json = cJSON_Print(root);
    httpd_resp_sendstr(req, json);
    free((void *)json);
    cJSON_Delete(root);

    return ESP_OK;
}

httpd_uri_t get_sysinfo_json_handler = {
    .uri = "/sysinfo.json",
    .method = HTTP_GET,
    .handler = get_sysinfo_json_handler_func,
    .user_ctx = NULL};

void add_settings(cJSON *root)
{
    cJSON *settings = cJSON_AddObjectToObject(root, "settings");

    cJSON_AddStringToObject(settings, "hostname", _settings->getHostname());

    cJSON_AddBoolToObject(settings, "useDHCP", _settings->getUseDHCP());

    ip4_addr_t currentIP, currentNetmask, currentGateway, currentDNS1, currentDNS2;
    _ethernet->getNetworkSettings(&currentIP, &currentNetmask, &currentGateway, &currentDNS1, &currentDNS2);
    cJSON_AddStringToObject(settings, "localIP", ip2str(_settings->getLocalIP(), currentIP));
    cJSON_AddStringToObject(settings, "netmask", ip2str(_settings->getNetmask(), currentNetmask));
    cJSON_AddStringToObject(settings, "gateway", ip2str(_settings->getGateway(), currentGateway));
    cJSON_AddStringToObject(settings, "dns1", ip2str(_settings->getDns1(), currentDNS1));
    cJSON_AddStringToObject(settings, "dns2", ip2str(_settings->getDns2(), currentDNS2));

    cJSON_AddNumberToObject(settings, "timesource", _settings->getTimesource());

    cJSON_AddNumberToObject(settings, "dcfOffset", _settings->getDcfOffset());

    cJSON_AddNumberToObject(settings, "gpsBaudrate", _settings->getGpsBaudrate());

    cJSON_AddStringToObject(settings, "ntpServer", _settings->getNtpServer());

    cJSON_AddNumberToObject(settings, "ledBrightness", _settings->getLEDBrightness());
}

esp_err_t get_settings_json_handler_func(httpd_req_t *req)
{
    if (validate_auth(req) != ESP_OK)
    {
        httpd_resp_set_status(req, "401 Not authorized");
        httpd_resp_sendstr(req, "401 Not authorized");
        return ESP_OK;
    }

    httpd_resp_set_type(req, "application/json");
    cJSON *root = cJSON_CreateObject();

    add_settings(root);

    const char *json = cJSON_Print(root);
    httpd_resp_sendstr(req, json);
    free((void *)json);
    cJSON_Delete(root);

    return ESP_OK;
}

httpd_uri_t get_settings_json_handler = {
    .uri = "/settings.json",
    .method = HTTP_GET,
    .handler = get_settings_json_handler_func,
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

WebUI::WebUI(Settings *settings, LED *statusLED, SysInfo *sysInfo, UpdateCheck *updateCheck, Ethernet *ethernet, RawUartUdpListener *rawUartUdpListener, RadioModuleConnector *radioModuleConnector, RadioModuleDetector *radioModuleDetector)
{
    _settings = settings;
    _statusLED = statusLED;
    _sysInfo = sysInfo;
    _ethernet = ethernet;
    _updateCheck = updateCheck;
    _rawUartUdpListener = rawUartUdpListener;
    _radioModuleConnector = radioModuleConnector;
    _radioModuleDetector = radioModuleDetector;

    char tokenBase[21];
    *((uint32_t *)tokenBase) = esp_random();
    *((uint32_t *)(tokenBase + sizeof(uint32_t))) = esp_random();
    strcpy(tokenBase + 2 * sizeof(uint32_t), _sysInfo->getSerialNumber());

    unsigned char shaResult[32];

    mbedtls_md_context_t ctx;
    mbedtls_md_init(&ctx);
    mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 0);
    mbedtls_md_starts(&ctx);
    mbedtls_md_update(&ctx, (unsigned char *)tokenBase, 20);
    mbedtls_md_finish(&ctx, shaResult);
    mbedtls_md_free(&ctx);

    size_t tokenLength;
    mbedtls_base64_encode((unsigned char *)_token, sizeof(_token), &tokenLength, shaResult, sizeof(shaResult));
    _token[tokenLength] = 0;
}

void WebUI::start()
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;
    config.max_uri_handlers = 10;
    config.uri_match_fn = httpd_uri_match_wildcard;

    httpd_handle_t _httpd_handle = NULL;

    if (httpd_start(&_httpd_handle, &config) == ESP_OK)
    {
        httpd_register_uri_handler(_httpd_handle, &post_login_json_handler);
        httpd_register_uri_handler(_httpd_handle, &get_sysinfo_json_handler);
        httpd_register_uri_handler(_httpd_handle, &get_settings_json_handler);
        httpd_register_uri_handler(_httpd_handle, &post_settings_json_handler);
        httpd_register_uri_handler(_httpd_handle, &post_ota_update_handler);

        httpd_register_uri_handler(_httpd_handle, &main_1e43358e_js_gz_handler);
        httpd_register_uri_handler(_httpd_handle, &main_1e43358e_css_gz_handler);
        httpd_register_uri_handler(_httpd_handle, &favicon_26242483_ico_gz_handler);
        httpd_register_uri_handler(_httpd_handle, &index_html_gz_handler);
    }
}

void WebUI::stop()
{
    httpd_stop(_httpd_handle);
}
