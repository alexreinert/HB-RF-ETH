/* 
 *  led.h is part of the HB-RF-ETH firmware - https://github.com/alexreinert/HB-RF-ETH
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

#pragma once

#include <stdio.h>
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "settings.h"

#define MAX_LED_COUNT 5

typedef enum
{
    LED_STATE_OFF = 0,
    LED_STATE_ON = 1,
    LED_STATE_BLINK = 2,
    LED_STATE_BLINK_INV = 3,
    LED_STATE_BLINK_FAST = 4,
    LED_STATE_BLINK_SLOW = 5,
} led_state_t;

class LED
{
private:
    uint8_t _state;
    ledc_channel_config_t _channel_conf;
    void _setPinState(bool enabled);

public:
    static void start(Settings *settings);
    static void stop();

    LED(gpio_num_t pin);
    void setState(led_state_t state);
    void updatePinState();
};