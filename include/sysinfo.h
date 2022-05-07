/* 
 *  sysinfo.h is part of the HB-RF-ETH firmware - https://github.com/alexreinert/HB-RF-ETH
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

typedef enum
{
    BOARD_TYPE_REV_1_8_PUB = 0,
    BOARD_TYPE_REV_1_8_SK = 1,
    BOARD_TYPE_REV_1_10_PUB = 2,
    BOARD_TYPE_REV_1_10_SK = 3,
    BOARD_TYPE_UNKNOWN = 255
} board_type_t;

class SysInfo
{
public:
    SysInfo();
    double getCpuUsage();
    double getMemoryUsage();
    const char* getCurrentVersion();
    const char *getSerialNumber();
    board_type_t getBoardType();
};
