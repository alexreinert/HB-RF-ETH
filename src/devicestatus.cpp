/* 
 *  devicestatus.cpp is part of the HB-RF-ETH firmware - https://github.com/alexreinert/HB-RF-ETH
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

#include "devicestatus.h"
#include <math.h>
#include <stdio.h>
#include "esp_timer.h"
#include "lwip/ip4_addr.h"

static double round2(double value)
{
    return round(value * 100.0) / 100.0;
}

static void formatIP(uint32_t addr, char *buf, int len)
{
    if (addr == IPADDR_ANY || addr == IPADDR_NONE)
    {
        buf[0] = 0;
        return;
    }
    ip4_addr_t ip = {.addr = addr};
    ip4addr_ntoa_r(&ip, buf, len);
}

static void formatMAC(uint32_t radioMAC, char *buf, size_t len)
{
    if (radioMAC == 0)
    {
        snprintf(buf, len, "n/a");
    }
    else
    {
        snprintf(buf, len, "0x%06X", radioMAC);
    }
}

DeviceStatus::DeviceStatus(Settings *settings, SysInfo *sysInfo, UpdateCheck *updateCheck, Ethernet *ethernet, RawUartUdpListener *rawUartUdpListener, RadioModuleDetector *radioModuleDetector)
    : _settings(settings), _sysInfo(sysInfo), _updateCheck(updateCheck), _ethernet(ethernet), _rawUartUdpListener(rawUartUdpListener), _radioModuleDetector(radioModuleDetector)
{
}

const char *DeviceStatus::radioModuleTypeToString(radio_module_type_t type)
{
    switch (type)
    {
    case RADIO_MODULE_HM_MOD_RPI_PCB:
        return "HM-MOD-RPI-PCB";
    case RADIO_MODULE_RPI_RF_MOD:
        return "RPI-RF-MOD";
    case RADIO_MODULE_HMIP_RFUSB:
        return "HMIP-RFUSB";
    default:
        return "-";
    }
}

const char *DeviceStatus::boardTypeToString(board_type_t type)
{
    switch (type)
    {
    case BOARD_TYPE_REV_1_8_PUB:
        return "REV 1.8 PUB";
    case BOARD_TYPE_REV_1_8_SK:
        return "REV 1.8 SK";
    case BOARD_TYPE_REV_1_10_PUB:
        return "REV 1.10 PUB";
    case BOARD_TYPE_REV_1_10_SK:
        return "REV 1.10 SK";
    default:
        return "unknown";
    }
}

uint32_t DeviceStatus::getCcuAddress()
{
    return _rawUartUdpListener->getConnectedRemoteAddress().addr;
}

const char *DeviceStatus::getSerialNumber()
{
    return _sysInfo->getSerialNumber();
}

const char *DeviceStatus::getHostname()
{
    return _settings->getHostname();
}

const char *DeviceStatus::getFirmwareVersion()
{
    return _sysInfo->getCurrentVersion();
}

cJSON *DeviceStatus::toJson()
{
    char buf[32];
    cJSON *root = cJSON_CreateObject();

    cJSON *device = cJSON_AddObjectToObject(root, "device");
    cJSON_AddStringToObject(device, "serial", _sysInfo->getSerialNumber());
    cJSON_AddStringToObject(device, "hostname", _settings->getHostname());
    cJSON_AddStringToObject(device, "boardType", boardTypeToString(_sysInfo->getBoardType()));
    cJSON_AddStringToObject(device, "firmwareVersion", _sysInfo->getCurrentVersion());
    cJSON_AddStringToObject(device, "latestFirmwareVersion", _updateCheck->getLatestVersion());
    cJSON_AddNumberToObject(device, "uptime", (double)(esp_timer_get_time() / 1000000LL));
    cJSON_AddNumberToObject(device, "cpuUsage", round2(_sysInfo->getCpuUsage()));
    cJSON_AddNumberToObject(device, "memoryUsage", round2(_sysInfo->getMemoryUsage()));

    ip4_addr_t ip, netmask, gateway, dns1, dns2;
    _ethernet->getNetworkSettings(&ip, &netmask, &gateway, &dns1, &dns2);
    formatIP(ip.addr, buf, sizeof(buf));
    cJSON_AddStringToObject(device, "ipAddress", buf);

    cJSON *ccu = cJSON_AddObjectToObject(root, "ccu");
    uint32_t ccuAddress = getCcuAddress();
    formatIP(ccuAddress, buf, sizeof(buf));
    cJSON_AddBoolToObject(ccu, "connected", buf[0] != 0);
    cJSON_AddStringToObject(ccu, "address", buf);

    cJSON *radio = cJSON_AddObjectToObject(root, "radioModule");
    radio_module_type_t type = _radioModuleDetector->getRadioModuleType();
    cJSON_AddBoolToObject(radio, "detected", type != RADIO_MODULE_NONE);
    cJSON_AddStringToObject(radio, "type", radioModuleTypeToString(type));
    cJSON_AddStringToObject(radio, "serial", _radioModuleDetector->getSerial());
    formatMAC(_radioModuleDetector->getBidCosRadioMAC(), buf, sizeof(buf));
    cJSON_AddStringToObject(radio, "bidCosRadioMAC", buf);
    formatMAC(_radioModuleDetector->getHmIPRadioMAC(), buf, sizeof(buf));
    cJSON_AddStringToObject(radio, "hmIPRadioMAC", buf);
    cJSON_AddStringToObject(radio, "sgtin", _radioModuleDetector->getSGTIN());
    const uint8_t *fw = _radioModuleDetector->getFirmwareVersion();
    if (type != RADIO_MODULE_NONE)
    {
        snprintf(buf, sizeof(buf), "%d.%d.%d", fw[0], fw[1], fw[2]);
    }
    else
    {
        buf[0] = 0;
    }
    cJSON_AddStringToObject(radio, "firmwareVersion", buf);

    return root;
}
