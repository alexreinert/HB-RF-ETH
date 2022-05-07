/* 
 *  ethernet.h is part of the HB-RF-ETH firmware - https://github.com/alexreinert/HB-RF-ETH
 *  
 *  Copyright 2022 Alexander Reinert
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

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_netif.h"
#include "esp_eth.h"
#include "esp_event.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "settings.h"

class Ethernet
{
private:
  esp_netif_config_t _netif_cfg;
  esp_netif_t *_eth_netif;
  esp_eth_handle_t _eth_handle;
  eth_mac_config_t _mac_config;
  eth_phy_config_t _phy_config;
  esp_eth_mac_t *_mac;
  esp_eth_phy_t *_phy;
  esp_eth_config_t _eth_config;

  Settings *_settings;
  bool _isConnected;

public:
  Ethernet(Settings *settings);

  void start();
  void stop();

  void getNetworkSettings(ip4_addr_t *ip, ip4_addr_t *netmask, ip4_addr_t *gateway, ip4_addr_t *dns1, ip4_addr_t *dns2);

  void _handleETHEvent(esp_event_base_t event_base, int32_t event_id, void *event_data);
  void _handleIPEvent(esp_event_base_t event_base, int32_t event_id, void *event_data);
};