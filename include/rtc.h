/* 
 *  rtc.h is part of the HB-RF-ETH firmware - https://github.com/alexreinert/HB-RF-ETH
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

#pragma once

#include <stdint.h>
#include <sys/time.h>
#include "driver/i2c.h"
#include "pins.h"

class Rtc
{
protected:
    Rtc(uint8_t address, uint8_t reg_start);
    i2c_port_t _i2c_port;
    const uint8_t _address;
    const uint8_t _reg_start;

public:
    static Rtc* detect();

    virtual bool begin();
    virtual ~Rtc();
    struct timeval GetTime();
    void SetTime(struct timeval now);
};

class RtcDS3231 : public Rtc
{
public:
    RtcDS3231();
};

class RtcRX8130 : public Rtc
{
public:
    RtcRX8130();
    bool begin();
};
