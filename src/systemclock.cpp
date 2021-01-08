/* 
 *  systemclock.cpp is part of the HB-RF-ETH firmware - https://github.com/alexreinert/HB-RF-ETH
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

#include "systemclock.h"
#include <sys/time.h>
#include "esp_log.h"

static const char *TAG = "SystemClock";

#define get_tzname(isdst) isdst > 0 ? *(tzname + 1) : *tzname

void updateRtcTask(void *parameter)
{
    Rtc *_rtc = (Rtc *)parameter;

    for (;;)
    {
        if (ulTaskNotifyTake(pdTRUE, portMAX_DELAY) != 1)
            continue;

        struct timeval tv;
        gettimeofday(&tv, NULL);
        _rtc->SetTime(tv);

        struct tm now;
        localtime_r(&tv.tv_sec, &now);

        ESP_LOGI(TAG, "Updated RTC to %02d-%02d-%02d %02d:%02d:%02d %s", now.tm_year + 1900, now.tm_mon + 1, now.tm_mday, now.tm_hour, now.tm_min, now.tm_sec, get_tzname(now.tm_isdst));
    }

    vTaskDelete(NULL);
}

SystemClock::SystemClock(Rtc *rtc) : _rtc(rtc)
{
}

void SystemClock::start(void)
{
    if (_rtc)
    {
        struct timeval tv = _rtc->GetTime();
        settimeofday(&tv, NULL);
        _lastSyncTime = tv;

        time_t nowtime = tv.tv_sec;
        struct tm *now = localtime(&nowtime);

        ESP_LOGI(TAG, "Updated time from RTC to %02d-%02d-%02d %02d:%02d:%02d %s", now->tm_year + 1900, now->tm_mon + 1, now->tm_mday, now->tm_hour, now->tm_min, now->tm_sec, get_tzname(now->tm_isdst));

        xTaskCreate(updateRtcTask, "SystemClock_RtcUpdateTask", 4096, _rtc, 10, &_tHandle);
    }
}

void SystemClock::stop(void)
{
    if (_tHandle != NULL)
    {
        vTaskDelete(_tHandle);
        _tHandle = NULL;
    }
}

void SystemClock::setTime(struct timeval *tv)
{
    settimeofday(tv, NULL);
    _lastSyncTime = *tv;

    if (_tHandle != NULL)
    {
        xTaskNotifyGive(_tHandle);
    }
}

struct timeval SystemClock::getTime()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv;
}

struct timeval SystemClock::getLastSyncTime()
{
    return _lastSyncTime;
}

struct tm SystemClock::getLocalTime(void)
{
    time_t now;
    time(&now);

    struct tm info;
    localtime_r(&now, &info);

    return info;
}
