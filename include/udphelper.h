/* 
 *  udphelper.h is part of the HB-RF-ETH firmware - https://github.com/alexreinert/HB-RF-ETH
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

typedef struct
{
  struct tcpip_api_call_data call;
  udp_pcb *pcb;
  const ip_addr_t *addr;
  uint16_t port;
  struct pbuf *pb;
  err_t err;
} udp_api_call_t;

typedef struct
{
  pbuf *pb;
  ip4_addr_t addr;
  uint16_t port;
} udp_event_t;

static err_t _udp_remove_api(struct tcpip_api_call_data *api_call_msg)
{
  udp_api_call_t *msg = (udp_api_call_t *)api_call_msg;
  msg->err = 0;
  udp_remove(msg->pcb);
  return msg->err;
}

static void _udp_remove(struct udp_pcb *pcb)
{
  udp_api_call_t msg;
  msg.pcb = pcb;
  tcpip_api_call(_udp_remove_api, (struct tcpip_api_call_data *)&msg);
}

static err_t _udp_bind_api(struct tcpip_api_call_data *api_call_msg)
{
  udp_api_call_t *msg = (udp_api_call_t *)api_call_msg;
  msg->err = udp_bind(msg->pcb, msg->addr, msg->port);
  return msg->err;
}

static err_t _udp_bind(struct udp_pcb *pcb, const ip_addr_t *addr, u16_t port)
{
  udp_api_call_t msg;
  msg.pcb = pcb;
  msg.addr = addr;
  msg.port = port;
  tcpip_api_call(_udp_bind_api, (struct tcpip_api_call_data *)&msg);
  return msg.err;
}

static err_t _udp_disconnect_api(struct tcpip_api_call_data *api_call_msg)
{
  udp_api_call_t *msg = (udp_api_call_t *)api_call_msg;
  msg->err = 0;
  udp_disconnect(msg->pcb);
  return msg->err;
}

static void _udp_disconnect(struct udp_pcb *pcb)
{
  udp_api_call_t msg;
  msg.pcb = pcb;
  tcpip_api_call(_udp_disconnect_api, (struct tcpip_api_call_data *)&msg);
}

static err_t _udp_sendto_api(struct tcpip_api_call_data *api_call_msg)
{
  udp_api_call_t *msg = (udp_api_call_t *)api_call_msg;
  msg->err = udp_sendto(msg->pcb, msg->pb, msg->addr, msg->port);
  return msg->err;
}

static err_t _udp_sendto(struct udp_pcb *pcb, struct pbuf *pb, const ip_addr_t *addr, u16_t port)
{
  udp_api_call_t msg;
  msg.pcb = pcb;
  msg.addr = addr;
  msg.port = port;
  msg.pb = pb;
  tcpip_api_call(_udp_sendto_api, (struct tcpip_api_call_data *)&msg);
  return msg.err;
}
