/* 
 *  mqttpublisher.h is part of the HB-RF-ETH firmware - https://github.com/alexreinert/HB-RF-ETH
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

#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "mqtt_client.h"
#include <atomic>
#include "settings.h"
#include "devicestatus.h"

// Publishes the device and radio module status to an MQTT broker.
//
// Topics (<prefix> defaults to "hb-rf-eth/<serial>"):
//   <prefix>/status  "online" / "offline" (retained, offline is sent as last will)
//   <prefix>/state   JSON document, same format as GET /api/v1/status (retained)
//
// Optionally Home Assistant MQTT discovery messages are published below
// "homeassistant/", so that all values appear as entities of one device.
class MqttPublisher
{
private:
    Settings *_settings;
    DeviceStatus *_deviceStatus;
    esp_mqtt_client_handle_t _client = NULL;
    TaskHandle_t _tHandle = NULL;
    std::atomic<bool> _connected;
    std::atomic<bool> _publishAll;

    char _topicPrefix[80] = {0};
    char _statusTopic[96] = {0};
    char _stateTopic[96] = {0};
    char _deviceId[32] = {0};

    void publishState();
    void publishDiscovery();
    void publishDiscoveryEntity(const char *component, const char *key, const char *name, const char *valueTemplate, const char *unit, const char *deviceClass, const char *stateClass, bool diagnostic, const char *icon);

public:
    MqttPublisher(Settings *settings, DeviceStatus *deviceStatus);
    void start();
    void stop();

    void _taskFunc();
    void _handleEvent(esp_mqtt_event_handle_t event);
};
