/* 
 *  pins.h is part of the HB-RF-ETH firmware - https://github.com/alexreinert/HB-RF-ETH
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

#include "driver/gpio.h"
#include "soc/adc_channel.h"

#define HM_RX_PIN GPIO_NUM_35
#define HM_TX_PIN GPIO_NUM_2

#define HM_SDA_PIN GPIO_NUM_18
#define HM_SCL_PIN GPIO_NUM_5

#define HM_RST_PIN GPIO_NUM_23
#define HM_BTN_PIN GPIO_NUM_34
#define HM_RED_PIN GPIO_NUM_15
#define HM_GREEN_PIN GPIO_NUM_14
#define HM_BLUE_PIN GPIO_NUM_12

#define LED_STATUS_PIN GPIO_NUM_4
#define LED_PWR_PIN GPIO_NUM_16

#define DCF_PIN GPIO_NUM_39

#define BOARD_REV_SENSE_CHANNEL ((adc_channel_t)ADC1_GPIO36_CHANNEL)
#define BOARD_REV_SENSE_UNIT ADC_UNIT_1

#define ETH_PHY_ADDR 0
#define ETH_POWER_PIN GPIO_NUM_13
#define ETH_MDC_PIN GPIO_NUM_32
#define ETH_MDIO_PIN GPIO_NUM_33
