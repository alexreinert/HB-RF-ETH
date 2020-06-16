/* 
 *  led.h is part of the HB-RF-ETH firmware - https://github.com/alexreinert/HB-RF-ETH
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
#include "driver/gpio.h"

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
    gpio_num_t _pin;
    uint8_t _state;

public:
    static void start();
    static void stop();

    LED(gpio_num_t pin);
    void setState(led_state_t state);
    void updatePinState();
};