/* 
 *  ntpserver.h is part of the HB-RF-ETH firmware - https://github.com/alexreinert/HB-RF-ETH
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

#include "lwip/opt.h"
#include "lwip/inet.h"
#include "lwip/udp.h"
#include "lwip/priv/tcpip_priv.h"
#include "systemclock.h"

typedef unsigned long long tstamp;

typedef struct ntp_packet
{
    uint8_t  flags;
    uint8_t  stratum;
    uint8_t  poll;
    int8_t   precision;
    uint32_t delay;
    uint32_t dispersion;
    char     ref_id[4];
    tstamp   ref_time;
    tstamp   orig_time;
    tstamp   recv_time;
    tstamp   trns_time;
} ntp_packet_t;

class NtpServer {
  private:
    SystemClock* _clk;
    udp_pcb* _pcb;
    QueueHandle_t _udp_queue;
    TaskHandle_t _tHandle = NULL;

    void handlePacket(pbuf *pb, ip4_addr_t addr, uint16_t port);

  public: 
    NtpServer(SystemClock* clk); 

    void start();
    void stop();

    void _udpQueueHandler();
    bool _udpReceivePacket(pbuf *pb, const ip_addr_t *addr, uint16_t port);
};
