/* 
 *  radiomoduledetector_utils.h is part of the HB-RF-ETH firmware - https://github.com/alexreinert/HB-RF-ETH
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

#include "esp_log.h"

#define sem_take(__sem, __timeout) (xSemaphoreTake(__sem, __timeout * 1000 / portTICK_PERIOD_MS) == pdTRUE)
#define sem_give(__sem) xSemaphoreGive(__sem)
#define sem_init(__sem) __sem = xSemaphoreCreateBinary();

#define log_frame(__text, __buffer, __len) \
    ESP_LOGD(TAG, __text);                 \
    ESP_LOG_BUFFER_HEX_LEVEL(TAG, __buffer, __len, ESP_LOG_DEBUG);

