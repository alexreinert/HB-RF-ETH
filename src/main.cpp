/* 
 *  main.cpp is part of the HB-RF-ETH firmware - https://github.com/alexreinert/HB-RF-ETH
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
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"

#include "pins.h"
#include "led.h"
#include "settings.h"
#include "rtc.h"
#include "systemclock.h"
#include "dcf.h"
#include "ntpclient.h"
#include "gps.h"
#include "ethernet.h"
#include "pushbuttonhandler.h"
#include "radiomoduleconnector.h"
#include "rawuartudplistener.h"
#include "webui.h"
#include "mdnsserver.h"
#include "ntpserver.h"
#include "esp_ota_ops.h"
#include "updatecheck.h"

static const char *TAG = "HB-RF-ETH";

extern "C"
{
    void app_main(void);
}

void app_main()
{
    Settings settings;

    LED powerLED(LED_PWR_PIN);
    LED statusLED(LED_STATUS_PIN);

    LED redLED(HM_RED_PIN);
    LED greenLED(HM_GREEN_PIN);
    LED blueLED(HM_BLUE_PIN);

    LED::start(&settings);

    powerLED.setState(LED_STATE_BLINK);
    statusLED.setState(LED_STATE_BLINK_INV);

    redLED.setState(LED_STATE_OFF);
    greenLED.setState(LED_STATE_OFF);
    blueLED.setState(LED_STATE_OFF);

    SysInfo sysInfo;

    PushButtonHandler pushButton;
    pushButton.handleStartupFactoryReset(&powerLED, &statusLED, &settings);

    RadioModuleConnector radioModuleConnector(&redLED, &greenLED, &blueLED);
    radioModuleConnector.start();

    Ethernet ethernet(&settings);
    ethernet.start();

    setenv("TZ", "UTC0", 1);
    tzset();

    RawUartUdpListener rawUartUdpLister(&radioModuleConnector);
    rawUartUdpLister.start();

    Rtc *rtc = NULL;
    rtc = Rtc::detect();

    SystemClock clk(rtc);
    clk.start();

    DCF dcf(&settings, &clk);
    NtpClient ntpClient(&settings, &clk);
    GPS gps(&settings, &clk);

    switch (settings.getTimesource())
    {
    case TIMESOURCE_NTP:
        ntpClient.start();
        break;
    case TIMESOURCE_GPS:
        gps.start();
        break;
    case TIMESOURCE_DCF:
        dcf.start();
        break;
    }

    MDns mdns;
    mdns.start(&settings);

    NtpServer ntpServer(&clk);
    ntpServer.start();

    UpdateCheck updateCheck(&sysInfo, &statusLED);
    updateCheck.start();

    WebUI webUI(&settings, &statusLED, &sysInfo, &updateCheck, &rawUartUdpLister);
    webUI.start();

    powerLED.setState(LED_STATE_ON);
    statusLED.setState(LED_STATE_OFF);

    esp_ota_mark_app_valid_cancel_rollback();

    vTaskSuspend(NULL);
}
