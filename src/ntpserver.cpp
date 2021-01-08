/* 
 *  ntpserver.cpp is part of the HB-RF-ETH firmware - https://github.com/alexreinert/HB-RF-ETH
 *  
 *  Copyright 2021 Alexander Reinert
 *  
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *  
 *      http://www.apache.org/licenses/LICENSE-2.0
 *  
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#include "ntpserver.h"
#include <string.h>
#include "esp_log.h"
#include "udphelper.h"

static const char *TAG = "NtpServer";

void _ntp_udpQueueHandlerTask(void *parameter)
{
    ((NtpServer *)parameter)->_udpQueueHandler();
}

void _ntp_udpReceivePaket(void *arg, udp_pcb *pcb, pbuf *pb, const ip_addr_t *addr, uint16_t port)
{
    while (pb != NULL)
    {
        pbuf *this_pb = pb;
        pb = pb->next;
        this_pb->next = NULL;
        if (!((NtpServer *)arg)->_udpReceivePacket(this_pb, addr, port))
        {
            pbuf_free(this_pb);
        }
    }
}

NtpServer::NtpServer(SystemClock *clk) : _clk(clk)
{
}

inline tstamp convertToNtp(struct timeval *tv)
{
    tv->tv_sec = htonl(tv->tv_sec + 2208988800L);
    tv->tv_usec = htonl(tv->tv_usec / 1e6 * 4294967296.);

    return (unsigned long long)tv->tv_sec + (((unsigned long long)tv->tv_usec) << 32);
}

void NtpServer::handlePacket(pbuf *pb, ip4_addr_t addr, uint16_t port)
{
    struct timeval tv = _clk->getTime();
    tstamp recv = convertToNtp(&tv);

    struct timeval lastSync = _clk->getLastSyncTime();
    if (lastSync.tv_sec < 1577836800l) // 2020-01-01 00:00:00 GMT
    {
        ESP_LOGE(TAG, "Ignoring ntp request because local time is not set");
        return;
    }

    if (pb->len != sizeof(ntp_packet_t))
    {
        ESP_LOGE(TAG, "Ignoring packet with bad length %d (should be %d)", pb->len, sizeof(ntp_packet_t));
        return;
    }

    ntp_packet_t ntp;
    memcpy(&ntp, pb->payload, sizeof(ntp));

    pbuf *resp_pb = pbuf_alloc_reference(&ntp, sizeof(ntp_packet_t), PBUF_REF);

    ip_addr_t resp_addr;
    resp_addr.type = IPADDR_TYPE_V4;
    resp_addr.u_addr.ip4 = addr;

    ntp.flags = 4 << 3 | 4; // version 4, mode server
    ntp.stratum = 15;       // Prefer other ntp server over us
    ntp.poll = 8;
    ntp.precision = 0;               // precision 2^0=1sec because of RTC values;
    ntp.delay = htonl(1 << 16);      // 1sec
    ntp.dispersion = htonl(1 << 16); // 1sec
    memset(ntp.ref_id, 0, sizeof(ntp.ref_id));
    ntp.orig_time = ntp.trns_time;
    ntp.recv_time = recv;
    ntp.ref_time = convertToNtp(&lastSync);
    tv = _clk->getTime();
    ntp.trns_time = convertToNtp(&tv);

    _udp_sendto(_pcb, resp_pb, &resp_addr, port);
    pbuf_free(resp_pb);
}

void NtpServer::start()
{
    _udp_queue = xQueueCreate(32, sizeof(udp_event_t *));
    xTaskCreate(_ntp_udpQueueHandlerTask, "NTPServer_UDP_QueueHandler", 4096, this, 10, &_tHandle);

    _pcb = udp_new();
    udp_recv(_pcb, &_ntp_udpReceivePaket, (void *)this);

    _udp_bind(_pcb, IP4_ADDR_ANY, 123);
}

void NtpServer::stop()
{
    _udp_disconnect(_pcb);
    udp_recv(_pcb, NULL, NULL);
    _udp_remove(_pcb);
    _pcb = NULL;

    vTaskDelete(_tHandle);
}

void NtpServer::_udpQueueHandler()
{
    udp_event_t *event = NULL;

    for (;;)
    {
        if (xQueueReceive(_udp_queue, &event, portMAX_DELAY) == pdTRUE)
        {
            handlePacket(event->pb, event->addr, event->port);
            pbuf_free(event->pb);
            free(event);
        }
    }

    vTaskDelete(NULL);
}

bool NtpServer::_udpReceivePacket(pbuf *pb, const ip_addr_t *addr, uint16_t port)
{
    udp_event_t *e = (udp_event_t *)malloc(sizeof(udp_event_t));
    if (!e)
    {
        return false;
    }

    e->pb = pb;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-arith"

    ip_hdr *iphdr = reinterpret_cast<ip_hdr *>(pb->payload - UDP_HLEN - IP_HLEN);
    e->addr.addr = iphdr->src.addr;

    udp_hdr *udphdr = reinterpret_cast<udp_hdr *>(pb->payload - UDP_HLEN);
    e->port = ntohs(udphdr->src);

#pragma GCC diagnostic pop

    if (xQueueSend(_udp_queue, &e, portMAX_DELAY) != pdPASS)
    {
        free((void *)(e));
        return false;
    }
    return true;
}