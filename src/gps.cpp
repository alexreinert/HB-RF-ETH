/* 
 *  gps.cpp is part of the HB-RF-ETH firmware - https://github.com/alexreinert/HB-RF-ETH
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

#include "GPS.h"
#include "pins.h"
#include "esp_log.h"
#include "string.h"

void gpsSerialQueueHandlerTask(void *parameter)
{
    ((GPS *)parameter)->_gpsSerialQueueHandler();
}

GPS::GPS(Settings *settings, SystemClock *clk) : _settings(settings), _clk(clk)
{
    uart_config_t uart_config = {
        .baud_rate = _settings->getGpsBaudrate(),
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 0,
        .use_ref_tick = false};
    uart_param_config(UART_NUM_2, &uart_config);
    uart_set_pin(UART_NUM_2, GPIO_NUM_0, DCF_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    using namespace std::placeholders;
    _lineReader = new LineReader(std::bind(&GPS::_handleLine, this, _1, _2));
}

void GPS::start()
{
    uart_driver_install(UART_NUM_2, UART_FIFO_LEN * 2, 0, 20, &_uart_queue, 0);
    xTaskCreate(gpsSerialQueueHandlerTask, "GPS_UART_QueueHandler", 4096, this, 15, &_tHandle);
}

void GPS::stop()
{
    uart_driver_delete(UART_NUM_2);
    vTaskDelete(_tHandle);
}

void GPS::_gpsSerialQueueHandler()
{
    uart_event_t event;
    uint8_t *buffer = (uint8_t *)malloc(UART_FIFO_LEN);

    uart_flush_input(UART_NUM_2);

    for (;;)
    {
        if (xQueueReceive(_uart_queue, (void *)&event, (portTickType)portMAX_DELAY))
        {
            switch (event.type)
            {
            case UART_DATA:
                uart_read_bytes(UART_NUM_2, buffer, event.size, portMAX_DELAY);
                _lineReader->Append(buffer, event.size);
                break;
            case UART_FIFO_OVF:
            case UART_BUFFER_FULL:
                uart_flush_input(UART_NUM_2);
                xQueueReset(_uart_queue);
                _lineReader->Flush();
                break;
            case UART_BREAK:
            case UART_PARITY_ERR:
            case UART_FRAME_ERR:
                _lineReader->Flush();
                break;
            default:
                break;
            }
        }
    }

    free(buffer);
    buffer = NULL;
    vTaskDelete(NULL);
}

bool parseRMCTime(unsigned char *buffer, uint16_t len, timeval *tv)
{
    int fieldIndex = 0;
    int fieldStart = 0;

    int fieldLength;

    struct tm time = {};
    time.tm_isdst = 0;

    for (int i = 0; i < len; i++)
    {
        if (buffer[i] == ',')
        {
            fieldLength = i - fieldStart;

            if (fieldIndex == 1)
            {
                if (fieldLength != 9)
                    return false;

                time.tm_sec = ((buffer[fieldStart + 4] - '0') * 10) + (buffer[fieldStart + 5] - '0');
                time.tm_min = ((buffer[fieldStart + 2] - '0') * 10) + (buffer[fieldStart + 3] - '0');
                time.tm_hour = ((buffer[fieldStart + 0] - '0') * 10) + (buffer[fieldStart + 1] - '0');

                tv->tv_usec = ((buffer[fieldStart + 4] - '7') * 100000) + ((buffer[fieldStart + 8] - '0') * 10000);
            }
            else if (fieldIndex == 9)
            {
                if (fieldLength != 6)
                    return false;

                time.tm_year = ((buffer[fieldStart + 4] - '0') * 10) + (buffer[fieldStart + 5] - '0') + 100;
                time.tm_mon = ((buffer[fieldStart + 2] - '0') * 10) + (buffer[fieldStart + 3] - '0') - 1;
                time.tm_mday = ((buffer[fieldStart + 0] - '0') * 10) + (buffer[fieldStart + 1] - '0');
            }

            fieldIndex++;
            fieldStart = i + 1;
        }
    }

    if (fieldIndex != 12)
        return false;

    tv->tv_sec = mktime(&time);

    return true;
}

void GPS::_handleLine(unsigned char *buffer, uint16_t len)
{
    uint64_t startTime = esp_timer_get_time();
    if ((len > 6) && (strncmp((char *)buffer, "$GPRMC", 6) == 0))
    {
        timeval tv;
        if (parseRMCTime(buffer, len, &tv))
        {
            if (_nextSync < startTime)
            {
                _nextSync = startTime + 300 * 1000 * 1000; // every 5 minutes

                tv.tv_usec += esp_timer_get_time() - startTime;
                _clk->setTime(&tv);
            }
        }
    }
}
