/* 
 *  webui.h is part of the HB-RF-ETH firmware - https://github.com/alexreinert/HB-RF-ETH
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
#include <stdint.h>
#include "settings.h"
#include "led.h"
#include "updatecheck.h"
#include "rawuartudplistener.h"
#include "esp_http_server.h"

class WebUI
{
private:
    httpd_handle_t _httpd_handle;

public:
    WebUI(Settings* settings, LED* statusLED, SysInfo *sysInfo, UpdateCheck* updateCheck, RawUartUdpListener* rawUartUdpListener, RadioModuleConnector* radioModuleConnector);
    void start();
    void stop();
};