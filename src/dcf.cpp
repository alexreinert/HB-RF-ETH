/* 
 *  dcf.cpp is part of the HB-RF-ETH firmware - https://github.com/alexreinert/HB-RF-ETH
 *  
 *  Copyright 2022 Alexander Reinert
 *  
 *  The HB-RF-ETH firmware is licensed under a
 *  Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International License.
 *  
 *  You should have received a copy of the license along with this
 *  work.  If not, see <http://creativecommons.org/licenses/by-nc-sa/4.0/>.
 *  
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *  
 */

#include "dcf.h"
#include <sys/time.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "pins.h"

static SystemClock *_clk;
static Settings *_settings;

static uint64_t _buffer = 0;
static uint8_t _bufferPos = 0;

static int64_t _previousSecondMark = 0;
static int64_t _secondMark = 0;

static QueueHandle_t _flank_queue;
static TaskHandle_t _queueHandlerTask;

static const char *TAG = "DCF";

typedef struct
{
    int64_t flankTime;
    int state;
} flank_event_t;

inline uint8_t bcd2bin(uint8_t val)
{
    return val - 6 * (val >> 4);
}

DCF::DCF(Settings *settings, SystemClock *clk)
{
    _settings = settings;
    _clk = clk;
}

bool checkParity(uint8_t start, uint8_t end)
{
    int parity = 0;

    for (int pos = start; pos <= end; pos++)
    {
        parity ^= (int)((_buffer >> pos) & 1);
    }

    return parity == 0;
}

int is_leap(unsigned int y)
{
    return (y % 4) == 0 && ((y % 100) != 0 || ((y + 1900) % 400) == 0);
}

time_t dcf2epoch(struct tm *dcf_tm, uint8_t tz)
{
    static const unsigned ndays[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

    time_t res = 0;
    int i;

    for (i = 70; i < dcf_tm->tm_year; ++i)
    {
        res += is_leap(i) ? 366 : 365;
    }

    for (i = 0; i < dcf_tm->tm_mon; ++i)
    {
        res += ndays[i];
        if (i == 1 && is_leap(dcf_tm->tm_year))
        {
            res++;
        }
    }

    res += dcf_tm->tm_mday - 1;
    res *= 24;
    res += dcf_tm->tm_hour;
    res -= tz ^ 3;
    res *= 60;
    res += dcf_tm->tm_min;
    res *= 60;

    return res;
}

static void IRAM_ATTR onPinChange(void *arg)
{
    int64_t flankTime = esp_timer_get_time();
    int state = gpio_get_level(DCF_PIN);

    flank_event_t event;
    event.flankTime = flankTime;
    event.state = state;

    BaseType_t xHigherPriorityTaskWokenByPost = pdFALSE;
    xQueueSendFromISR(_flank_queue, &event, &xHigherPriorityTaskWokenByPost);

    if (xHigherPriorityTaskWokenByPost)
    {
        portYIELD_FROM_ISR();
    }
}

static void handlePinChange(int64_t flankTime, int state)
{
    if (state)
    {
        // end of pulse
        int64_t secondLength = _secondMark - _previousSecondMark;
        int64_t pulseLength = flankTime - _secondMark;

        uint64_t pulseValue;
        if (pulseLength > 80000 && pulseLength < 135000)
        {
            pulseValue = 0;
        }
        else if (pulseLength > 180000 && pulseLength < 235000)
        {
            pulseValue = 1;
        }
        else
        {
            ESP_LOGE(TAG, "Invalid pulse: %llu %llu", secondLength / 1000ULL, pulseLength / 1000ULL);
            return; // invalid pulse length
        }

        if (secondLength > 970000 && secondLength < 1035000)
        {
            if (_bufferPos < 59)
            {
                _bufferPos++;
                _buffer |= (pulseValue << _bufferPos);
            }
        }
        else if (secondLength > 1970000 && secondLength < 2035000)
        {
            uint8_t timezone = (uint8_t)((_buffer >> 17) & 3);

            if (_bufferPos < 58 || _bufferPos > 59 || (int)(_buffer & 1) != 0 || (int)((_buffer >> 20) & 1) != 1 || timezone == 0 || timezone == 3 || !checkParity(21, 28) || !checkParity(29, 35) || !checkParity(36, 58))
            {
                ESP_LOGE(TAG, "Received invalid frame");
            }
            else
            {
                struct tm dcf_tm;
                dcf_tm.tm_year = 100 + bcd2bin((int)((_buffer >> 50) & 0xff));
                dcf_tm.tm_mon = bcd2bin((int)((_buffer >> 45) & 0x1f)) - 1;
                dcf_tm.tm_mday = bcd2bin((int)((_buffer >> 36) & 0x3f));
                dcf_tm.tm_hour = bcd2bin((int)((_buffer >> 29) & 0x3f));
                dcf_tm.tm_min = bcd2bin((int)((_buffer >> 21) & 0x7f));
                dcf_tm.tm_sec = 0;

                struct timeval tv;
                tv.tv_sec = dcf2epoch(&dcf_tm, timezone);
                tv.tv_usec = esp_timer_get_time() - _secondMark + _settings->getDcfOffset();

                ESP_LOGI(TAG, "Updated time to %02d-%02d-%02d %02d:%02d:%02d.%06ld %s", dcf_tm.tm_year + 1900, dcf_tm.tm_mon + 1, dcf_tm.tm_mday, dcf_tm.tm_hour, dcf_tm.tm_min, dcf_tm.tm_sec, tv.tv_usec, timezone == 2 ? "CET" : "CEST");
                _clk->setTime(&tv);
            }

            _buffer = pulseValue;
            _bufferPos = 0;
        }
        else
        {
            ESP_LOGE(TAG, "Invalid second: %llu %llu", secondLength / 1000ULL, pulseLength / 1000ULL);
            return; // invalid second length
        }
    }
    else
    {
        // start of pulse
        _previousSecondMark = _secondMark;
        _secondMark = flankTime;
    }
}

static void flankEventQueueHandler(void *arg)
{
    flank_event_t event;

    for (;;)
    {
        if (xQueueReceive(_flank_queue, &event, portMAX_DELAY) == pdTRUE)
        {
            handlePinChange(event.flankTime, event.state);
        }
    }

    vTaskDelete(NULL);
}

void DCF::start()
{
    _flank_queue = xQueueCreate(8, sizeof(flank_event_t));
    xTaskCreate(flankEventQueueHandler, "DFC_FlankEvent_QueueHandler", 4096, NULL, 17, &_queueHandlerTask);

    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_ANYEDGE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = 1ULL << DCF_PIN;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(DCF_PIN, onPinChange, NULL);
}

void DCF::stop()
{
    gpio_isr_handler_remove(DCF_PIN);
    xQueueReset(_flank_queue);
    vTaskDelete(_queueHandlerTask);
}
