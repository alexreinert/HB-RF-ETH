/* 
 *  gps.h is part of the HB-RF-ETH firmware - https://github.com/alexreinert/HB-RF-ETH
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

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/uart.h"
#include "systemclock.h"
#include "settings.h"
#include "linereader.h"

class GPS
{
private:
    Settings *_settings;
    SystemClock *_clk;
    TaskHandle_t _tHandle = NULL;
    QueueHandle_t _uart_queue;
    LineReader *_lineReader;
    uint64_t _nextSync = 0;

public:
    GPS(Settings *settings, SystemClock *clk);

    void start(void);
    void stop(void);

    void _gpsSerialQueueHandler();
    void _handleLine(unsigned char *buffer, uint16_t len);
};
