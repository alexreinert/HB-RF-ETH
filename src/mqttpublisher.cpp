/* 
 *  mqttpublisher.cpp is part of the HB-RF-ETH firmware - https://github.com/alexreinert/HB-RF-ETH
 *  
 *  Copyright 2026 HB-RF-ETH contributors
 *  
 *  The HB-RF-ETH firmware is licensed under a
 *  Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International License.
 *  
 *  You should have received a copy of the license along with this
 *  work.  If not, see <http://creativecommons.org/licenses/by-nc-sa/4.0/>.
 *  
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *  
 */

#include "mqttpublisher.h"
#include <string.h>
#include <stdio.h>
#include "esp_log.h"
#include "esp_timer.h"
#include "cJSON.h"

static const char *TAG = "MQTT";

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ((MqttPublisher *)handler_args)->_handleEvent((esp_mqtt_event_handle_t)event_data);
}

static void mqtt_task_func(void *parameter)
{
    ((MqttPublisher *)parameter)->_taskFunc();
}

MqttPublisher::MqttPublisher(Settings *settings, DeviceStatus *deviceStatus)
    : _settings(settings), _deviceStatus(deviceStatus), _connected(false), _publishAll(false)
{
}

void MqttPublisher::start()
{
    if (!_settings->getMqttEnabled())
    {
        ESP_LOGI(TAG, "MQTT is disabled");
        return;
    }

    if (strlen(_settings->getMqttServer()) == 0)
    {
        ESP_LOGW(TAG, "MQTT is enabled, but no broker is configured");
        return;
    }

    const char *prefix = _settings->getMqttTopicPrefix();
    if (strlen(prefix) > 0)
    {
        strncpy(_topicPrefix, prefix, sizeof(_topicPrefix) - 1);
    }
    else
    {
        snprintf(_topicPrefix, sizeof(_topicPrefix), "hb-rf-eth/%s", _deviceStatus->getSerialNumber());
    }
    size_t len = strlen(_topicPrefix);
    while (len > 0 && _topicPrefix[len - 1] == '/')
    {
        _topicPrefix[--len] = 0;
    }

    snprintf(_statusTopic, sizeof(_statusTopic), "%s/status", _topicPrefix);
    snprintf(_stateTopic, sizeof(_stateTopic), "%s/state", _topicPrefix);
    snprintf(_deviceId, sizeof(_deviceId), "hb-rf-eth_%s", _deviceStatus->getSerialNumber());

    esp_mqtt_client_config_t config = {};
    config.host = _settings->getMqttServer();
    config.port = _settings->getMqttPort();
    config.transport = MQTT_TRANSPORT_OVER_TCP;
    config.protocol_ver = MQTT_PROTOCOL_V_3_1_1;
    config.client_id = _deviceId;
    if (strlen(_settings->getMqttUser()) > 0)
    {
        config.username = _settings->getMqttUser();
        config.password = _settings->getMqttPassword();
    }
    config.keepalive = 60;
    config.lwt_topic = _statusTopic;
    config.lwt_msg = "offline";
    config.lwt_qos = 1;
    config.lwt_retain = 1;
    config.buffer_size = 2048;

    _client = esp_mqtt_client_init(&config);
    if (_client == NULL)
    {
        ESP_LOGE(TAG, "Could not initialize MQTT client");
        return;
    }

    esp_mqtt_client_register_event(_client, MQTT_EVENT_ANY, mqtt_event_handler, this);

    if (esp_mqtt_client_start(_client) != ESP_OK)
    {
        ESP_LOGE(TAG, "Could not start MQTT client");
        esp_mqtt_client_destroy(_client);
        _client = NULL;
        return;
    }

    xTaskCreate(mqtt_task_func, "MQTT_Publisher", 4096, this, 3, &_tHandle);

    ESP_LOGI(TAG, "MQTT client started, broker %s:%d, topic prefix %s", _settings->getMqttServer(), _settings->getMqttPort(), _topicPrefix);
}

void MqttPublisher::stop()
{
    if (_tHandle != NULL)
    {
        vTaskDelete(_tHandle);
        _tHandle = NULL;
    }

    if (_client != NULL)
    {
        if (_connected)
        {
            esp_mqtt_client_publish(_client, _statusTopic, "offline", 0, 1, 1);
        }
        esp_mqtt_client_stop(_client);
        esp_mqtt_client_destroy(_client);
        _client = NULL;
    }

    _connected = false;
}

void MqttPublisher::_handleEvent(esp_mqtt_event_handle_t event)
{
    switch (event->event_id)
    {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "Connected to broker");
        _connected = true;
        _publishAll = true;
        if (_tHandle != NULL)
        {
            xTaskNotifyGive(_tHandle);
        }
        break;

    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGW(TAG, "Disconnected from broker");
        _connected = false;
        break;

    case MQTT_EVENT_ERROR:
        ESP_LOGW(TAG, "MQTT error");
        break;

    default:
        break;
    }
}

void MqttPublisher::_taskFunc()
{
    const int64_t intervalUs = (int64_t)_settings->getMqttInterval() * 1000000LL;
    int64_t lastPublish = 0;
    uint32_t lastCcuAddress = 0;

    for (;;)
    {
        ulTaskNotifyTake(pdTRUE, 1000 / portTICK_PERIOD_MS);

        if (!_connected)
            continue;

        int64_t now = esp_timer_get_time();
        uint32_t ccuAddress = _deviceStatus->getCcuAddress();

        if (_publishAll.exchange(false))
        {
            esp_mqtt_client_publish(_client, _statusTopic, "online", 0, 1, 1);
            if (_settings->getMqttHaDiscovery())
            {
                publishDiscovery();
            }
            lastPublish = 0;
        }

        if (lastPublish == 0 || ccuAddress != lastCcuAddress || (now - lastPublish) >= intervalUs)
        {
            publishState();
            lastPublish = now;
            lastCcuAddress = ccuAddress;
        }
    }
}

void MqttPublisher::publishState()
{
    cJSON *root = _deviceStatus->toJson();
    char *json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    if (json != NULL)
    {
        esp_mqtt_client_publish(_client, _stateTopic, json, 0, 0, 1);
        free(json);
    }
}

void MqttPublisher::publishDiscoveryEntity(const char *component, const char *key, const char *name, const char *valueTemplate, const char *unit, const char *deviceClass, const char *stateClass, bool diagnostic, const char *icon)
{
    char buf[128];
    cJSON *root = cJSON_CreateObject();

    cJSON_AddStringToObject(root, "name", name);
    snprintf(buf, sizeof(buf), "%s_%s", _deviceId, key);
    cJSON_AddStringToObject(root, "unique_id", buf);
    cJSON_AddStringToObject(root, "object_id", buf);
    cJSON_AddStringToObject(root, "state_topic", _stateTopic);
    cJSON_AddStringToObject(root, "value_template", valueTemplate);
    cJSON_AddStringToObject(root, "availability_topic", _statusTopic);
    if (unit)
        cJSON_AddStringToObject(root, "unit_of_measurement", unit);
    if (deviceClass)
        cJSON_AddStringToObject(root, "device_class", deviceClass);
    if (stateClass)
        cJSON_AddStringToObject(root, "state_class", stateClass);
    if (diagnostic)
        cJSON_AddStringToObject(root, "entity_category", "diagnostic");
    if (icon)
        cJSON_AddStringToObject(root, "icon", icon);

    cJSON *device = cJSON_AddObjectToObject(root, "device");
    cJSON *identifiers = cJSON_AddArrayToObject(device, "identifiers");
    cJSON_AddItemToArray(identifiers, cJSON_CreateString(_deviceId));
    cJSON_AddStringToObject(device, "name", _deviceStatus->getHostname());
    cJSON_AddStringToObject(device, "model", "HB-RF-ETH");
    cJSON_AddStringToObject(device, "sw_version", _deviceStatus->getFirmwareVersion());

    char *json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    if (json != NULL)
    {
        snprintf(buf, sizeof(buf), "homeassistant/%s/%s/%s/config", component, _deviceId, key);
        esp_mqtt_client_publish(_client, buf, json, 0, 1, 1);
        free(json);
    }
}

void MqttPublisher::publishDiscovery()
{
    publishDiscoveryEntity("binary_sensor", "ccu_connected", "CCU connected", "{{ 'ON' if value_json.ccu.connected else 'OFF' }}", NULL, "connectivity", NULL, false, NULL);
    publishDiscoveryEntity("sensor", "ccu_address", "CCU address", "{{ value_json.ccu.address }}", NULL, NULL, NULL, false, "mdi:ip-network");
    publishDiscoveryEntity("sensor", "cpu_usage", "CPU usage", "{{ value_json.device.cpuUsage }}", "%", NULL, "measurement", true, "mdi:cpu-32-bit");
    publishDiscoveryEntity("sensor", "memory_usage", "Memory usage", "{{ value_json.device.memoryUsage }}", "%", NULL, "measurement", true, "mdi:memory");
    publishDiscoveryEntity("sensor", "uptime", "Uptime", "{{ value_json.device.uptime }}", "s", "duration", NULL, true, NULL);
    publishDiscoveryEntity("sensor", "ip_address", "IP address", "{{ value_json.device.ipAddress }}", NULL, NULL, NULL, true, "mdi:ip");
    publishDiscoveryEntity("sensor", "firmware_version", "Firmware version", "{{ value_json.device.firmwareVersion }}", NULL, NULL, NULL, true, "mdi:chip");
    publishDiscoveryEntity("sensor", "latest_firmware_version", "Latest firmware version", "{{ value_json.device.latestFirmwareVersion }}", NULL, NULL, NULL, true, "mdi:update");
    publishDiscoveryEntity("binary_sensor", "radio_module_detected", "Radio module detected", "{{ 'ON' if value_json.radioModule.detected else 'OFF' }}", NULL, NULL, NULL, true, "mdi:radio-tower");
    publishDiscoveryEntity("sensor", "radio_module_type", "Radio module type", "{{ value_json.radioModule.type }}", NULL, NULL, NULL, true, "mdi:radio-tower");
    publishDiscoveryEntity("sensor", "radio_module_serial", "Radio module serial", "{{ value_json.radioModule.serial }}", NULL, NULL, NULL, true, "mdi:barcode");
    publishDiscoveryEntity("sensor", "radio_module_firmware", "Radio module firmware", "{{ value_json.radioModule.firmwareVersion }}", NULL, NULL, NULL, true, "mdi:chip");
    publishDiscoveryEntity("sensor", "radio_module_bidcos_mac", "Radio address (BidCos)", "{{ value_json.radioModule.bidCosRadioMAC }}", NULL, NULL, NULL, true, "mdi:identifier");
    publishDiscoveryEntity("sensor", "radio_module_hmip_mac", "Radio address (HmIP)", "{{ value_json.radioModule.hmIPRadioMAC }}", NULL, NULL, NULL, true, "mdi:identifier");
    publishDiscoveryEntity("sensor", "radio_module_sgtin", "SGTIN", "{{ value_json.radioModule.sgtin }}", NULL, NULL, NULL, true, "mdi:barcode");
}
