/* 
 *  mdnsserver.cpp is part of the HB-RF-ETH firmware - https://github.com/alexreinert/HB-RF-ETH
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

#include "mdnsserver.h"
#include "mdns.h"
#include "esp_system.h"

void MDns::start(Settings* settings)
{
    ESP_ERROR_CHECK_WITHOUT_ABORT(mdns_init());
    ESP_ERROR_CHECK_WITHOUT_ABORT(mdns_hostname_set(settings->getHostname()));

    ESP_ERROR_CHECK_WITHOUT_ABORT(mdns_service_add(NULL, "_http", "_tcp", 80, NULL, 0));
    ESP_ERROR_CHECK_WITHOUT_ABORT(mdns_service_add(NULL, "_raw-uart", "_udp", 3008, NULL, 0));
    ESP_ERROR_CHECK_WITHOUT_ABORT(mdns_service_add(NULL, "_ntp", "_udp", 123, NULL, 0));
}

void MDns::stop()
{
    mdns_free();
}
