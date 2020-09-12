/* 
 *  radiomoduleconnector_utils.h is part of the HB-RF-ETH firmware - https://github.com/alexreinert/HB-RF-ETH
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

#include "esp_log.h"

#define sem_take(__sem, __timeout) (xSemaphoreTake(__sem, __timeout * 1000 / portTICK_PERIOD_MS) == pdTRUE)
#define sem_give(__sem) xSemaphoreGive(__sem)
#define sem_init(__sem) __sem = xSemaphoreCreateBinary();

#define log_frame(__text, __buffer, __len) \
    ESP_LOGD(TAG, __text);                 \
    ESP_LOG_BUFFER_HEX_LEVEL(TAG, __buffer, __len, ESP_LOG_DEBUG);

