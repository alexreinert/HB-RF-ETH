/* 
 *  ntpclient.cpp is part of the HB-RF-ETH firmware - https://github.com/alexreinert/HB-RF-ETH
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

#include "ntpclient.h"
#include "esp_sntp.h"

static Settings *_settings;
static SystemClock *_clk;

static void _time_sync_notification_cb(struct timeval *tv)
{
    _clk->setTime(tv);
}

NtpClient::NtpClient(Settings *settings, SystemClock *clk)
{
    _settings = settings;
    _clk = clk;
}

void NtpClient::start()
{
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, _settings->getNtpServer());
    sntp_set_time_sync_notification_cb(_time_sync_notification_cb);
    sntp_init();
}

void NtpClient::stop()
{
    sntp_stop();
}