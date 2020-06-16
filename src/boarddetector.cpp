/* 
 *  boarddetector.cpp is part of the HB-RF-ETH firmware - https://github.com/alexreinert/HB-RF-ETH
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

#include "boarddetector.h"
#include "esp_adc_cal.h"
#include "esp_log.h"
#include "pins.h"

#define DEFAULT_VREF 1100

static const char *TAG = "BoardDetector";

uint32_t get_voltage(adc_unit_t adc_unit, adc_channel_t adc_channel, adc_bits_width_t adc_width, adc_atten_t adc_atten)
{
    esp_adc_cal_characteristics_t *adc_chars = reinterpret_cast<esp_adc_cal_characteristics_t *>(calloc(1, sizeof(esp_adc_cal_characteristics_t)));
    esp_adc_cal_characterize(adc_unit, adc_atten, adc_width, DEFAULT_VREF, adc_chars);

    adc_gpio_init(adc_unit, adc_channel);

    if (adc_unit == ADC_UNIT_1)
    {
        adc1_config_width(adc_width);
        adc1_config_channel_atten((adc1_channel_t)adc_channel, adc_atten);
    }
    else
    {
        adc2_config_channel_atten((adc2_channel_t)adc_channel, adc_atten);
    }

    uint32_t voltage;
    esp_adc_cal_get_voltage(adc_channel, adc_chars, &voltage);

    free(adc_chars);

    return voltage;
}

board_type_t BoardDetector::detect()
{
    uint32_t voltage = get_voltage(BOARD_REV_SENSE_UNIT, BOARD_REV_SENSE_CHANNEL, ADC_WIDTH_BIT_10, ADC_ATTEN_DB_11);

    switch (voltage)
    {
    case 1500 ... 1800:
        return BOARD_TYPE_REV_1_8_SK;

    case 2600 ... 2900:
        return BOARD_TYPE_REV_1_8_PUB;

    default:
        ESP_LOGW(TAG, "Could not determine board, voltage: %u", voltage);
        return BOARD_TYPE_UNKNOWN;
    }
}