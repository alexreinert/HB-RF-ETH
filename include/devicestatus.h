/* 
 *  devicestatus.h is part of the HB-RF-ETH firmware - https://github.com/alexreinert/HB-RF-ETH
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

#include "cJSON.h"
#include "settings.h"
#include "sysinfo.h"
#include "updatecheck.h"
#include "ethernet.h"
#include "rawuartudplistener.h"
#include "radiomoduledetector.h"

// Collects the device and radio module status in one place, so that the
// REST API and the MQTT publisher expose exactly the same data.
class DeviceStatus
{
private:
    Settings *_settings;
    SysInfo *_sysInfo;
    UpdateCheck *_updateCheck;
    Ethernet *_ethernet;
    RawUartUdpListener *_rawUartUdpListener;
    RadioModuleDetector *_radioModuleDetector;

public:
    DeviceStatus(Settings *settings, SysInfo *sysInfo, UpdateCheck *updateCheck, Ethernet *ethernet, RawUartUdpListener *rawUartUdpListener, RadioModuleDetector *radioModuleDetector);

    // Returns a new cJSON object, the caller has to free it with cJSON_Delete
    cJSON *toJson();

    // IPv4 address of the connected CCU / debmatic / piVCCU, 0 if none is connected
    uint32_t getCcuAddress();

    const char *getSerialNumber();
    const char *getHostname();
    const char *getFirmwareVersion();

    static const char *radioModuleTypeToString(radio_module_type_t type);
    static const char *boardTypeToString(board_type_t type);
};
