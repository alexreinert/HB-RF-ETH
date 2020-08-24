/* 
 *  radiomoduleconnector.h is part of the HB-RF-ETH firmware - https://github.com/alexreinert/HB-RF-ETH
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

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/uart.h"
#include "led.h"
#include "streamparser.h"
#include <atomic>
#define _Atomic(X) std::atomic<X>

typedef enum
{
    RADIO_MODULE_NONE = 0,
    RADIO_MODULE_HM_MOD_RPI_PCB = 3,
    RADIO_MODULE_RPI_RF_MOD = 4,
} radio_module_type_t;

class FrameHandler
{
public:
    virtual void handleFrame(unsigned char *buffer, uint16_t len) = 0;
};

class RadioModuleConnector : private FrameHandler
{
private:
    LED *_redLED;
    LED *_greenLED;
    LED *_blueLED;
    StreamParser *_streamParser;
    std::atomic<FrameHandler *> _frameHandler = ATOMIC_VAR_INIT(0);
    QueueHandle_t _uart_queue;
    TaskHandle_t _tHandle = NULL;

    void sendFrame(uint8_t counter, uint8_t destination, uint8_t command, unsigned char *data, uint data_len);
    void _handleFrame(unsigned char *buffer, uint16_t len);

    void detectRadioModule();
    void handleFrame(unsigned char *buffer, uint16_t len);

    char _serial[11] = {0};
    uint32_t _radioMAC;
    char _sgtin[25] = {0};
    uint8_t _firmwareVersion[3];
    radio_module_type_t _radioModuleType;

    int _detectState;
    int _detectRetryCount;
    int _detectMsgCounter;
    SemaphoreHandle_t _detectWaitFrameDataSemaphore;

public:
    RadioModuleConnector(LED *redLED, LED *greenLed, LED *blueLed);

    void start();
    void stop();

    void setLED(bool red, bool green, bool blue);

    void setFrameHandler(FrameHandler *handler);

    void resetModule();

    void sendFrame(unsigned char *buffer, uint16_t len);

    const char *getSerial();
    uint32_t getRadioMAC();
    const char *getSGTIN();
    const uint8_t *getFirmwareVersion();
    radio_module_type_t getRadioModuleType();

    void _serialQueueHandler();
};
