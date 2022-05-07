/* 
 *  rtc.cpp is part of the HB-RF-ETH firmware - https://github.com/alexreinert/HB-RF-ETH
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

#include <stdint.h>
#include "rtc.h"
#include "esp_log.h"

static const char *TAG = "RTC";

static uint8_t bcd2bin(uint8_t val)
{
    return val - 6 * (val >> 4);
}

static uint8_t bin2bcd(uint8_t val)
{
    return val + 6 * (val / 10);
}

const uint8_t daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

static bool _isDriverInstalled = false;

static void i2c_master_init()
{
    if (_isDriverInstalled)
        return;

    i2c_config_t i2c_config = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = HM_SDA_PIN,
        .scl_io_num = HM_SCL_PIN,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master{.clk_speed = 100000}};
    i2c_param_config(I2C_NUM_0, &i2c_config);
    i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0);
    _isDriverInstalled = true;
}

Rtc *Rtc::detect()
{
    RtcDS3231 *ds3231 = new RtcDS3231();
    if (ds3231->begin())
    {
        ESP_LOGI(TAG, "DS3231 RTC found and initialized.");
        return ds3231;
    }
    else
    {
        delete ds3231;
    }

    RtcRX8130 *rx9130 = new RtcRX8130();
    if (rx9130->begin())
    {
        ESP_LOGI(TAG, "RX9130 RTC found and initialized.");
        return rx9130;
    }
    else
    {
        delete rx9130;
    }

    ESP_LOGE(TAG, "No RTC found.");
    return NULL;
}

Rtc::Rtc(uint8_t address, uint8_t reg_start) : _address(address), _reg_start(reg_start)
{
    i2c_master_init();
}

Rtc::~Rtc()
{
}

bool Rtc::begin()
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (_address << 1) | I2C_MASTER_WRITE, true);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, 50 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);

    return ret == ESP_OK;
}

struct timeval Rtc::GetTime()
{
    struct timeval res = {};

    uint8_t rawData[7] = {0};

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, _address << 1 | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, _reg_start, true);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, _address << 1 | I2C_MASTER_READ, true);
    i2c_master_read(cmd, rawData, sizeof(rawData) - 1, I2C_MASTER_ACK);
    i2c_master_read_byte(cmd, rawData + sizeof(rawData) - 1, I2C_MASTER_NACK);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, 50 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);

    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Could not read time from RTC");
        res.tv_sec = 0;
        res.tv_usec = 0;
        return res;
    }

    res.tv_sec = bcd2bin(rawData[0]);         // seconds
    res.tv_sec += bcd2bin(rawData[1]) * 60;   // minutes
    res.tv_sec += bcd2bin(rawData[2]) * 3600; // hours

    uint16_t days = bcd2bin(rawData[4]);
    uint8_t month = bcd2bin(rawData[5]);
    uint8_t year = bcd2bin(rawData[6]);

    for (uint8_t i = 1; i < month; ++i)
    {
        days += daysInMonth[i - 1];
    }

    if (month > 2 && year % 4 == 0)
        days++;

    days += 365 * year + (year + 3) / 4 - 1;

    res.tv_sec += days * 86400;

    res.tv_sec += 10957 * 86400; // epoch diff 1970 vs. 2000

    return res;
}

void Rtc::SetTime(struct timeval now)
{
    now.tv_sec -= 10957 * 86400; // epoch diff 1970 vs. 2000

    uint8_t seconds = now.tv_sec % 60;
    now.tv_sec /= 60;
    uint8_t minutes = now.tv_sec % 60;
    now.tv_sec /= 60;
    uint8_t hours = now.tv_sec % 24;

    uint16_t days = now.tv_sec / 24;

    uint8_t leap;
    uint8_t year;

    for (year = 0;; year++)
    {
        leap = year % 4 == 0;
        if (days < 365 + leap)
            break;
        days -= 365 + leap;
    }

    uint8_t month;
    for (month = 1;; month++)
    {
        uint8_t daysPerMonth = daysInMonth[month - 1];
        if (leap && month == 2)
            ++daysPerMonth;
        if (days < daysPerMonth)
            break;
        days -= daysPerMonth;
    }
    days++;

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, _address << 1 | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, _reg_start, true);
    i2c_master_write_byte(cmd, bin2bcd(seconds), true);
    i2c_master_write_byte(cmd, bin2bcd(minutes), true);
    i2c_master_write_byte(cmd, bin2bcd(hours), true);
    i2c_master_write_byte(cmd, 0, true);
    i2c_master_write_byte(cmd, bin2bcd(days), true);
    i2c_master_write_byte(cmd, bin2bcd(month), true);
    i2c_master_write_byte(cmd, bin2bcd(year), true);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, 50 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);

    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Could not write time to RTC");
    }
}

RtcDS3231::RtcDS3231() : Rtc::Rtc(0x68, 0)
{
}

RtcRX8130::RtcRX8130() : Rtc::Rtc(0x32, 0x10)
{
}

bool RtcRX8130::begin()
{
    if (Rtc::begin())
    {
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, _address << 1 | I2C_MASTER_WRITE, true);
        i2c_master_write_byte(cmd, 0x1f, true);
        i2c_master_write_byte(cmd, 0x31, true);
        i2c_master_stop(cmd);
        esp_err_t ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, 50 / portTICK_RATE_MS);
        i2c_cmd_link_delete(cmd);

        return ret == ESP_OK;
    }
    else
    {
        return false;
    }
}
