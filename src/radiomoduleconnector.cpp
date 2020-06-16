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

#include "RadioModuleConnector.h"
#include "driver/gpio.h"
#include "pins.h"

void serialQueueHandlerTask(void *parameter)
{
    ((RadioModuleConnector *)parameter)->_serialQueueHandler();
}

RadioModuleConnector::RadioModuleConnector(LED *redLED, LED *greenLed, LED *blueLed) : _redLED(redLED), _greenLED(greenLed), _blueLED(blueLed)
{
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = 1ULL << HM_BTN_PIN;
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
        .use_ref_tick = false
        };
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
                _streamParser->Append(buffer, event.size);
                break;
            case UART_FIFO_OVF:
            case UART_BUFFER_FULL:
                uart_flush_input(UART_NUM_1);
                xQueueReset(_uart_queue);
                _streamParser->Flush();
                break;
            case UART_BREAK:
            case UART_PARITY_ERR:
            case UART_FRAME_ERR:
                _streamParser->Flush();
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
