/* 
 *  led.cpp is part of the HB-RF-ETH firmware - https://github.com/alexreinert/HB-RF-ETH
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

#include "led.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

static uint8_t _blinkState = 0;
static LED *_leds[MAX_LED_COUNT] = {0};
static TaskHandle_t _switchTaskHandle = NULL;

void ledSwitcherTask(void *parameter)
{
    for (;;)
    {
        _blinkState = (_blinkState + 1) % 24;

        for (uint8_t i = 0; i < MAX_LED_COUNT; i++)
        {
            if (_leds[i] == 0)
            {
                break;
            }
            _leds[i]->updatePinState();
        }
        vTaskDelay(125 / portTICK_PERIOD_MS);
    }

    vTaskDelete(NULL);
}

void LED::start()
{
    if (!_switchTaskHandle)
    {
        xTaskCreate(ledSwitcherTask, "LED_Switcher", 4096, NULL, 10, &_switchTaskHandle);
    }
}

void LED::stop()
{
    if (_switchTaskHandle)
    {
        vTaskDelete(_switchTaskHandle);
        _switchTaskHandle = NULL;
    }
}

LED::LED(gpio_num_t pin) : _pin(pin)
{
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = 1ULL << pin;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);

    for (uint8_t i = 0; i < MAX_LED_COUNT; i++)
    {
        if (_leds[i] == 0)
        {
            _leds[i] = this;
            break;
        }
    }
}

void LED::setState(led_state_t state)
{
    _state = state;
    updatePinState();
}

void LED::updatePinState()
{
    switch (_state)
    {
    case LED_STATE_OFF:
        gpio_set_level(_pin, 0);
        break;
    case LED_STATE_ON:
        gpio_set_level(_pin, 1);
        break;
    case LED_STATE_BLINK:
        gpio_set_level(_pin, (_blinkState % 8) < 4 ? 1 : 0);
        break;
    case LED_STATE_BLINK_INV:
        gpio_set_level(_pin, (_blinkState % 8) < 4 ? 0 : 1);
        break;
    case LED_STATE_BLINK_FAST:
        gpio_set_level(_pin, (_blinkState % 2) == 0 ? 1 : 0);
        break;
    case LED_STATE_BLINK_SLOW:
        gpio_set_level(_pin, _blinkState < 12 ? 1 : 0);
        break;
    }
}