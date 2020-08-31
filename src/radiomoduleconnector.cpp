/* 
 *  radiomoduleconnector.cpp is part of the HB-RF-ETH firmware - https://github.com/alexreinert/HB-RF-ETH
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "RadioModuleConnector.h"
#include "hmframe.h"
#include "driver/gpio.h"
#include "pins.h"
#include "esp_log.h"

static const char *TAG = "RadioModuleConnector";

void serialQueueHandlerTask(void *parameter)
{
    ((RadioModuleConnector *)parameter)->_serialQueueHandler();
}

RadioModuleConnector::RadioModuleConnector(LED *redLED, LED *greenLed, LED *blueLed) : _redLED(redLED), _greenLED(greenLed), _blueLED(blueLed)
{
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = 1ULL << HM_RST_PIN;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);

    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 0,
        .use_ref_tick = false};
    uart_param_config(UART_NUM_1, &uart_config);
    uart_set_pin(UART_NUM_1, HM_TX_PIN, HM_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    using namespace std::placeholders;
    _streamParser = new StreamParser(false, std::bind(&RadioModuleConnector::_handleFrame, this, _1, _2));

    resetModule();
}

void RadioModuleConnector::start()
{
    setLED(false, false, false);

    uart_driver_install(UART_NUM_1, UART_FIFO_LEN * 2, 0, 20, &_uart_queue, 0);

    xTaskCreate(serialQueueHandlerTask, "RadioModuleConnector_UART_QueueHandler", 4096, this, 15, &_tHandle);
    resetModule();

    detectRadioModule();
    resetModule();
}

void RadioModuleConnector::stop()
{
    resetModule();
    uart_driver_delete(UART_NUM_1);
    vTaskDelete(_tHandle);
}

void RadioModuleConnector::setFrameHandler(FrameHandler *frameHandler)
{
    atomic_store(&_frameHandler, frameHandler);
}

void RadioModuleConnector::setLED(bool red, bool green, bool blue)
{
    _redLED->setState(red ? LED_STATE_ON : LED_STATE_OFF);
    _greenLED->setState(green ? LED_STATE_ON : LED_STATE_OFF);
    _blueLED->setState(blue ? LED_STATE_ON : LED_STATE_OFF);
}

void RadioModuleConnector::resetModule()
{
    gpio_set_level(HM_RST_PIN, 1);
    vTaskDelay(50 / portTICK_PERIOD_MS);
    gpio_set_level(HM_RST_PIN, 0);
    vTaskDelay(50 / portTICK_PERIOD_MS);
}

void RadioModuleConnector::sendFrame(unsigned char *buffer, uint16_t len)
{
    uart_write_bytes(UART_NUM_1, (const char *)buffer, len);
}

void RadioModuleConnector::_serialQueueHandler()
{
    uart_event_t event;
    uint8_t *buffer = (uint8_t *)malloc(UART_FIFO_LEN);

    uart_flush_input(UART_NUM_1);

    for (;;)
    {
        if (xQueueReceive(_uart_queue, (void *)&event, (portTickType)portMAX_DELAY))
        {
            switch (event.type)
            {
            case UART_DATA:
                uart_read_bytes(UART_NUM_1, buffer, event.size, portMAX_DELAY);
                _streamParser->append(buffer, event.size);
                break;
            case UART_FIFO_OVF:
            case UART_BUFFER_FULL:
                uart_flush_input(UART_NUM_1);
                xQueueReset(_uart_queue);
                _streamParser->flush();
                break;
            case UART_BREAK:
            case UART_PARITY_ERR:
            case UART_FRAME_ERR:
                _streamParser->flush();
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

void RadioModuleConnector::_handleFrame(unsigned char *buffer, uint16_t len)
{
    FrameHandler *frameHandler = (FrameHandler *)atomic_load(&_frameHandler);

    if (frameHandler)
    {
        frameHandler->handleFrame(buffer, len);
    }
}

void RadioModuleConnector::sendFrame(uint8_t counter, uint8_t destination, uint8_t command, unsigned char *data, uint data_len)
{
    HMFrame frame;
    unsigned char sendBuffer[8 + data_len + 10];

    frame.counter = counter;
    frame.destination = destination;
    frame.command = command;
    frame.data = data;
    frame.data_len = data_len;
    uint16_t len = frame.encode(sendBuffer, sizeof(sendBuffer), true);

    ESP_LOGD(TAG, "Sending HM frame:");
    ESP_LOG_BUFFER_HEX_LEVEL(TAG, sendBuffer, len, ESP_LOG_DEBUG);

    sendFrame(sendBuffer, len);
}

void RadioModuleConnector::detectRadioModule()
{
    _detectState = 0;
    _detectRetryCount = 0;
    _detectMsgCounter = 0;

    _radioModuleType = RADIO_MODULE_NONE;

    _detectWaitFrameDataSemaphore = xSemaphoreCreateBinary();
    HMFrame frame;

    FrameHandler *orginalFrameHandler = this;

    atomic_exchange(&_frameHandler, orginalFrameHandler);
    _streamParser->setDecodeEscaped(true);

    while (_detectRetryCount < 3)
    {
        if (xSemaphoreTake(_detectWaitFrameDataSemaphore, 1000 / portTICK_PERIOD_MS) == pdTRUE)
        {
            _detectRetryCount = 0;
        }

        switch (_detectState)
        {
        case 0:
            sendFrame(_detectMsgCounter++, HM_DST_COMMON, HM_CMD_COMMON_IDENTIFY, NULL, 0);
            if (xSemaphoreTake(_detectWaitFrameDataSemaphore, 3000 / portTICK_PERIOD_MS) != pdTRUE)
            {
                sendFrame(_detectMsgCounter++, HM_DST_HMSYSTEM, HM_CMD_HMSYSTEM_IDENTIFY, NULL, 0);
            }
            break;

        case 10:
            sendFrame(_detectMsgCounter++, HM_DST_TRX, HM_CMD_TRX_GET_VERSION, NULL, 0);
            break;

        case 11:
            sendFrame(_detectMsgCounter++, HM_DST_HMSYSTEM, HM_CMD_HMSYSTEM_GET_VERSION, NULL, 0);
            break;

        case 20:
            sendFrame(_detectMsgCounter++, HM_DST_COMMON, HM_CMD_COMMON_GET_SGTIN, NULL, 0);
            break;

        case 30:
            sendFrame(_detectMsgCounter++, HM_DST_LLMAC, HM_CMD_LLMAC_GET_DEFAULT_RF_ADDR, NULL, 0);
            break;

        case 31:
            sendFrame(_detectMsgCounter++, HM_DST_TRX, HM_CMD_TRX_GET_DEFAULT_RF_ADDR, NULL, 0);
            break;

        case 32:
            sendFrame(_detectMsgCounter++, HM_DST_HMIP, HM_CMD_HMIP_GET_DEFAULT_RF_ADDR, NULL, 0);
            break;

        case 40:
            sendFrame(_detectMsgCounter++, HM_DST_LLMAC, HM_CMD_LLMAC_GET_SERIAL, NULL, 0);
            break;

        case 41:
            sendFrame(_detectMsgCounter++, HM_DST_HMSYSTEM, HM_CMD_HMSYSTEM_GET_SERIAL, NULL, 0);
            break;

        case 50:
            sendFrame(_detectMsgCounter++, HM_DST_TRX, HM_CMD_TRX_GET_MCU_TYPE, NULL, 0);
            break;

        case 255:
            atomic_exchange(&_frameHandler, orginalFrameHandler);
            _streamParser->setDecodeEscaped(false);
            return;
        }

        _detectRetryCount++;
    }

    atomic_exchange(&_frameHandler, orginalFrameHandler);
    _streamParser->setDecodeEscaped(false);
}

void RadioModuleConnector::handleFrame(unsigned char *buffer, uint16_t len)
{
    ESP_LOGD(TAG, "Received HM frame:");
    ESP_LOG_BUFFER_HEX_LEVEL(TAG, buffer, len, ESP_LOG_DEBUG);

    HMFrame frame;
    if (!HMFrame::TryParse(buffer, len, &frame))
    {
        return;
    }

    switch (_detectState)
    {
    case 0:
        if (frame.destination == HM_DST_COMMON && frame.command == HM_CMD_COMMON_ACK && frame.data_len == 12 && frame.data[0] == 1 && strncmp((char *)(frame.data + 1), "HMIP_TRX_Bl", 11) == 0)
        {
            // TRX CoPro in bootloader --> start app
            sendFrame(_detectMsgCounter++, HM_DST_COMMON, HM_CMD_COMMON_START_APP, NULL, 0);
        }
        if (frame.destination == HM_DST_HMSYSTEM && frame.command == HM_CMD_HMSYSTEM_ACK && frame.data_len == 10 && frame.data[0] == 2 && strncmp((char *)(frame.data + 1), "Co_CPU_BL", 9) == 0)
        {
            // Legacy CoPro in bootloader --> start app
            sendFrame(_detectMsgCounter++, HM_DST_HMSYSTEM, HM_CMD_HMSYSTEM_START_APP, NULL, 0);
        }
        else if (frame.destination == HM_DST_COMMON && frame.command == HM_CMD_COMMON_ACK && frame.data_len == 14 && frame.data[0] == 1 && strncmp((char *)(frame.data + 1), "DualCoPro_App", 13) == 0)
        {
            // Dual CoPro in app
            _detectState = 10;
            xSemaphoreGive(_detectWaitFrameDataSemaphore);
        }
        else if (frame.destination == HM_DST_COMMON && frame.command == 0 && frame.data_len == 13 && strncmp((char *)frame.data, "DualCoPro_App", 13) == 0)
        {
            // Dual CoPro switched to app
            _detectState = 10;
            xSemaphoreGive(_detectWaitFrameDataSemaphore);
        }
        else if (frame.destination == HM_DST_HMSYSTEM && frame.command == 0 && frame.data_len == 10 && strncmp((char *)frame.data, "Co_CPU_App", 10) == 0)
        {
            // Legacy CoPro switched to app
            _detectState = 11;
            sprintf(_sgtin, "n/a");
            _radioModuleType = RADIO_MODULE_HM_MOD_RPI_PCB;
            xSemaphoreGive(_detectWaitFrameDataSemaphore);
        }
        break;

    case 10:
        if (frame.destination == HM_DST_TRX && frame.command == HM_CMD_TRX_ACK && frame.data_len == 10 && frame.data[0] == 1)
        {
            memcpy(_firmwareVersion, frame.data + 1, 3);
            _detectState = 20;
            xSemaphoreGive(_detectWaitFrameDataSemaphore);
        }
        break;

    case 11:
        if (frame.destination == HM_DST_HMSYSTEM && frame.command == HM_CMD_HMSYSTEM_ACK && frame.data_len == 7 && frame.data[0] == 2)
        {
            memcpy(_firmwareVersion, frame.data + 4, 3);
            _detectState = 31;
            xSemaphoreGive(_detectWaitFrameDataSemaphore);
        }
        break;

    case 20:
        if (frame.destination == HM_DST_COMMON && frame.command == HM_CMD_COMMON_ACK && frame.data_len == 13 && frame.data[0] == 1)
        {
            sprintf(_sgtin, "%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X", frame.data[1], frame.data[2], frame.data[3], frame.data[4], frame.data[5], frame.data[6], frame.data[7], frame.data[8], frame.data[9], frame.data[10], frame.data[11], frame.data[12]);
            _detectState = 30;
            xSemaphoreGive(_detectWaitFrameDataSemaphore);
        }
        break;

    case 30:
        if (frame.destination == HM_DST_LLMAC && frame.command == HM_CMD_LLMAC_ACK && frame.data_len == 4 && frame.data[0] == 1)
        {
            _radioMAC = (frame.data[1] << 16) | (frame.data[2] << 8) | frame.data[3];
            _detectState = (_radioMAC == 0 || (_radioMAC & 0xffff) == 0xffff) ? 32 : 40;
            xSemaphoreGive(_detectWaitFrameDataSemaphore);
        }
        break;

    case 31:
        if (frame.destination == HM_DST_TRX && frame.command == HM_CMD_TRX_ACK && frame.data_len == 6)
        {
            _radioMAC = (frame.data[3] << 16) | (frame.data[4] << 8) | frame.data[5];
            _detectState = 41;
            xSemaphoreGive(_detectWaitFrameDataSemaphore);
        }
        break;

    case 32:
        if (frame.destination == HM_DST_HMIP && frame.command == HM_CMD_HMIP_ACK && frame.data_len == 4 && frame.data[0] == 1)
        {
            _radioMAC = (frame.data[1] << 16) | (frame.data[2] << 8) | frame.data[3];
            _detectState = 40;
            xSemaphoreGive(_detectWaitFrameDataSemaphore);
        }
        break;

    case 40:
        if (frame.destination == HM_DST_LLMAC && frame.command == HM_CMD_LLMAC_ACK && frame.data_len == 11 && frame.data[0] == 1)
        {
            memcpy(_serial, frame.data + 1, 10);
            _detectState = 50;
            xSemaphoreGive(_detectWaitFrameDataSemaphore);
        }
        break;

    case 41:
        if (frame.destination == HM_DST_HMSYSTEM && frame.command == HM_CMD_HMSYSTEM_ACK && frame.data_len == 11 && frame.data[0] == 2)
        {
            memcpy(_serial, frame.data + 1, 10);
            _detectState = 255;
            xSemaphoreGive(_detectWaitFrameDataSemaphore);
        }
        break;

    case 50:
        if (frame.destination == HM_DST_TRX && frame.command == HM_CMD_TRX_ACK && frame.data_len == 2 && frame.data[0] == 1)
        {
            _radioModuleType = (radio_module_type_t)frame.data[1];
            _detectState = 255;
            xSemaphoreGive(_detectWaitFrameDataSemaphore);
        }
        break;
    }
}

const char *RadioModuleConnector::getSerial()
{
    return _radioModuleType == RADIO_MODULE_RPI_RF_MOD ? (_sgtin + 14) : _serial;
}

uint32_t RadioModuleConnector::getRadioMAC()
{
    return _radioMAC;
}

const char *RadioModuleConnector::getSGTIN()
{
    return _sgtin;
}

const uint8_t *RadioModuleConnector::getFirmwareVersion()
{
    return _firmwareVersion;
}

radio_module_type_t RadioModuleConnector::getRadioModuleType()
{
    return _radioModuleType;
}
