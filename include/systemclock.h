/* 
 *  systemclock.h is part of the HB-RF-ETH firmware - https://github.com/alexreinert/HB-RF-ETH
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

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "rtc.h"

class SystemClock
{
private:
    Rtc *_rtc;
    struct timeval _lastSyncTime = { .tv_sec = 0, .tv_usec = 0 };
    TaskHandle_t _tHandle = NULL;

public:
    SystemClock(Rtc *rtc);

    void start();
    void stop();

    void setTime(struct timeval *tv);
    struct timeval getTime();
    struct timeval getLastSyncTime();
    struct tm getLocalTime();
};
