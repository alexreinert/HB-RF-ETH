/* 
 *  mdnsserver.cpp is part of the HB-RF-ETH firmware - https://github.com/alexreinert/HB-RF-ETH
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

#include "mdnsserver.h"
#include "mdns.h"
#include "esp_system.h"

void MDns::start(Settings* settings)
{
    ESP_ERROR_CHECK_WITHOUT_ABORT(mdns_init());
    ESP_ERROR_CHECK_WITHOUT_ABORT(mdns_hostname_set(settings->getHostname()));

    ESP_ERROR_CHECK_WITHOUT_ABORT(mdns_service_add(NULL, "_http", "_tcp", 80, NULL, 0));
    ESP_ERROR_CHECK_WITHOUT_ABORT(mdns_service_add(NULL, "_raw-uart", "_udp", 3008, NULL, 0));
    ESP_ERROR_CHECK_WITHOUT_ABORT(mdns_service_add(NULL, "_ntp", "_udp", 123, NULL, 0));
}

void MDns::stop()
{
    mdns_free();
}
