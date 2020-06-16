/* 
 *  ethernet.h is part of the HB-RF-ETH firmware - https://github.com/alexreinert/HB-RF-ETH
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

#pragma once

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "tcpip_adapter.h"
#include "esp_eth.h"
#include "esp_event.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "settings.h"

class Ethernet
{
private:
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

  bool getIsConnected();

  void _handleETHEvent(esp_event_base_t event_base, int32_t event_id, void *event_data);
  void _handleIPEvent(esp_event_base_t event_base, int32_t event_id, void *event_data);
};