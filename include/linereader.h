/* 
 *  linereader.h is part of the HB-RF-ETH firmware - https://github.com/alexreinert/HB-RF-ETH
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
#include <functional>

class LineReader
{
private:
    unsigned char _buffer[1024];
    std::function<void(unsigned char *buffer, uint16_t len)> _processor;
    uint16_t _buffer_pos;

public:
    LineReader(std::function<void(unsigned char *buffer, uint16_t len)> processor);

    void Append(unsigned char chr);
    void Append(unsigned char *buffer, uint16_t len);
    void Flush();
};
