/* 
 *  pushbuttonhandler.cpp is part of the HB-RF-ETH firmware - https://github.com/alexreinert/HB-RF-ETH
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

#include "pushbuttonhandler.h"

static const char *TAG = "PushButtonHandler";

PushButtonHandler::PushButtonHandler()
{
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = 1ULL << HM_BTN_PIN;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);
}

void PushButtonHandler::handleStartupFactoryReset(LED *powerLED, LED *statusLED, Settings *settings)
{
    if (gpio_get_level(HM_BTN_PIN) == 0)
    {
        // reset request start if button is pressed at least for 4sec
        for (int i = 0; i < 40; i++)
        {
            if (gpio_get_level(HM_BTN_PIN) == 1)
            {
                return;
            }
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }

        powerLED->setState(LED_STATE_OFF);
        statusLED->setState(LED_STATE_BLINK_FAST);

        ESP_LOGI(TAG, "Factory Reset mode started.");

        // wait for release of button
        while (gpio_get_level(HM_BTN_PIN) == 0)
        {
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }

        // wait to be pressed again or timeout
        for (int i = 0; i < 40; i++)
        {
            if (gpio_get_level(HM_BTN_PIN) == 0)
            {
                break;
            }
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }

        // reset request start if button is pressed at least for 4sec
        for (int i = 0; i < 40; i++)
        {
            if (gpio_get_level(HM_BTN_PIN) == 1)
            {
                statusLED->setState(LED_STATE_ON);
                vTaskDelay(1000 / portTICK_PERIOD_MS);
                powerLED->setState(LED_STATE_ON);
                statusLED->setState(LED_STATE_OFF);
                ESP_LOGI(TAG, "Factory Reset mode timeout.");
                return;
            }
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }

        statusLED->setState(LED_STATE_OFF);
        vTaskDelay(100 / portTICK_PERIOD_MS);

        settings->clear();
        ESP_LOGI(TAG, "Factory Reset done.");

        powerLED->setState(LED_STATE_ON);
        statusLED->setState(LED_STATE_ON);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        powerLED->setState(LED_STATE_BLINK);
        statusLED->setState(LED_STATE_BLINK_INV);
    }
}