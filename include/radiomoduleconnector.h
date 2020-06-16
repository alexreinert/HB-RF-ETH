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
#define _Atomic(X) std::atomic< X >

class FrameHandler
{
public:
    virtual void handleFrame(unsigned char *buffer, uint16_t len) = 0;
};

class RadioModuleConnector
{
private:
    LED *_redLED;
    LED *_greenLED;
    LED *_blueLED;
    StreamParser *_streamParser;
    std::atomic<FrameHandler*> _frameHandler = ATOMIC_VAR_INIT(0);
    QueueHandle_t _uart_queue;
    TaskHandle_t _tHandle = NULL;

    void _handleFrame(unsigned char *buffer, uint16_t len);

public:
    RadioModuleConnector(LED *redLED, LED *greenLed, LED *blueLed);

    void start();
    void stop();

    void setLED(bool red, bool green, bool blue);

    void setFrameHandler(FrameHandler *handler);

    void resetModule();

    void sendFrame(unsigned char *buffer, uint16_t len);

    void _serialQueueHandler();
};
