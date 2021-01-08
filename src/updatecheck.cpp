/* 
 *  updatecheck.cpp is part of the HB-RF-ETH firmware - https://github.com/alexreinert/HB-RF-ETH
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

#include "updatecheck.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "string.h"
#include "cJSON.h"

static const char *TAG = "UpdateCheck";

void _update_check_task_func(void *parameter)
{
  {
    ((UpdateCheck *)parameter)->_taskFunc();
  }
}

UpdateCheck::UpdateCheck(SysInfo* sysInfo, LED *statusLED) : _sysInfo(sysInfo), _statusLED(statusLED)
{
}

void UpdateCheck::start()
{
  xTaskCreate(_update_check_task_func, "UpdateCheck", 4096, this, 3, &_tHandle);
}

void UpdateCheck::stop()
{
  vTaskDelete(_tHandle);
}

const char *UpdateCheck::getLatestVersion()
{
  return _latestVersion;
}

void UpdateCheck::_updateLatestVersion()
{
  char url[128];
  snprintf(url, sizeof(url), "https://www.debmatic.de/hb-rf-eth/latestVersion.json?board=%d&serial=%s&version=%s", _sysInfo->getBoardType(), _sysInfo->getSerialNumber(), _sysInfo->getCurrentVersion());

  esp_http_client_config_t config = {};
  config.url = url;

  esp_http_client_handle_t client = esp_http_client_init(&config);

  if (esp_http_client_perform(client) == ESP_OK)
  {
    char buffer[128];
    int len = esp_http_client_read(client, buffer, sizeof(buffer) - 1);

    if (len > 0)
    {
      buffer[len] = 0;
      cJSON *json = cJSON_Parse(buffer);

      char *latestVersion = cJSON_GetStringValue(cJSON_GetObjectItem(json, "version"));
      if (latestVersion != NULL)
      {
        strcpy(_latestVersion, latestVersion);
      }
      cJSON_Delete(json);
    }
  }

  esp_http_client_cleanup(client);
}

void UpdateCheck::_taskFunc()
{
  // some time for initial network connection
  vTaskDelay(30000 / portTICK_PERIOD_MS);

  for (;;)
  {
    ESP_LOGI(TAG, "Start checking for the latest available firmware.");
    _updateLatestVersion();

    if (strcmp(_latestVersion, "n/a") != 0 && strcmp(_sysInfo->getCurrentVersion(), _latestVersion) < 0)
    {
      ESP_LOGW(TAG, "An updated firmware with version %s is available.", _latestVersion);
      _statusLED->setState(LED_STATE_BLINK_SLOW);
    }
    else
    {
      ESP_LOGI(TAG, "There is no newer firmware available.");
    }

    vTaskDelay((8 * 60 * 60000) / portTICK_PERIOD_MS); // 8h
  }

  vTaskDelete(NULL);
}