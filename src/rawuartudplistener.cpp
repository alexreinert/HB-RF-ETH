/* 
 *  rawuartudplistener.cpp is part of the HB-RF-ETH firmware - https://github.com/alexreinert/HB-RF-ETH
 *  
 *  Copyright 2020 Alexander Reinert
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

#include "rawuartudplistener.h"
#include "hmframe.h"
#include "esp_log.h"
#include <string.h>
#include "udphelper.h"

static const char *TAG = "RawUartUdpListener";

#define PROTOCOL_VERSION 1

void _raw_uart_udpQueueHandlerTask(void *parameter)
{
  ((RawUartUdpListener *)parameter)->_udpQueueHandler();
}

void _raw_uart_udpReceivePaket(void *arg, udp_pcb *pcb, pbuf *pb, const ip_addr_t *addr, uint16_t port)
{
  while (pb != NULL)
  {
    pbuf *this_pb = pb;
    pb = pb->next;
    this_pb->next = NULL;
    if (!((RawUartUdpListener *)arg)->_udpReceivePacket(this_pb, addr, port))
    {
      pbuf_free(this_pb);
    }
  }
}

RawUartUdpListener::RawUartUdpListener(RadioModuleConnector *radioModuleConnector) : _radioModuleConnector(radioModuleConnector)
{
  atomic_init(&_connectionStarted, false);
  atomic_init(&_remotePort, (ushort)0);
  atomic_init(&_remoteAddress, 0u);
  atomic_init(&_counter, 0);
}

void RawUartUdpListener::handlePacket(pbuf *pb, ip4_addr_t addr, uint16_t port)
{
  size_t length = pb->len;
  unsigned char *data = (unsigned char *)(pb->payload);
  unsigned char response_buffer[2];

  if (length < 4)
  {
    ESP_LOGE(TAG, "Received invalid raw-uart packet, length %d", length);
    return;
  }

  if (data[0] != 0 && (addr.addr != atomic_load(&_remoteAddress) || port != atomic_load(&_remotePort)))
  {
    ESP_LOGE(TAG, "Received raw-uart packet from invalid address.");
    return;
  }

  if (*((uint16_t *)(data + length - 2)) != htons(HMFrame::crc(data, length - 2)))
  {
    ESP_LOGE(TAG, "Received raw-uart packet with invalid crc.");
    return;
  }

  _lastReceivedKeepAlive = esp_timer_get_time();

  switch (data[0])
  {
  case 0: // connect
    if (length != 5)
    {
      ESP_LOGE(TAG, "Received invalid raw-uart connect packet, length %d", length);
      return;
    }

    if (data[2] == PROTOCOL_VERSION)
    {
      atomic_store(&_remotePort, (ushort)0);
      atomic_store(&_connectionStarted, false);
      atomic_store(&_remoteAddress, addr.addr);
      atomic_store(&_remotePort, port);
      _radioModuleConnector->setLED(true, true, false);
      response_buffer[0] = PROTOCOL_VERSION;
      response_buffer[1] = data[1];
      sendMessage(0, response_buffer, 2);
    }
    break;

  case 1: // disconnect
    atomic_store(&_remotePort, (ushort)0);
    atomic_store(&_connectionStarted, false);
    atomic_store(&_remoteAddress, 0u);
    _radioModuleConnector->setLED(false, false, false);
    break;

  case 2: // keep alive
    break;

  case 3: // LED
    if (length != 5)
    {
      ESP_LOGE(TAG, "Received invalid raw-uart LED packet, length %d", length);
      return;
    }

    _radioModuleConnector->setLED(data[2] & 1, data[2] & 2, data[2] & 4);
    break;

  case 4: // Reset
    if (length != 4)
    {
      ESP_LOGE(TAG, "Received invalid raw-uart reset packet, length %d", length);
      return;
    }

    _radioModuleConnector->resetModule();
    break;

  case 5: // Start connection
    if (length != 4)
    {
      ESP_LOGE(TAG, "Received invalid raw-uart startconn packet, length %d", length);
      return;
    }

    atomic_store(&_connectionStarted, true);
    break;

  case 6: // End connection
    if (length != 4)
    {
      ESP_LOGE(TAG, "Received invalid raw-uart endconn packet, length %d", length);
      return;
    }

    atomic_store(&_connectionStarted, false);
    break;

  case 7: // Frame
    if (length < 5)
    {
      ESP_LOGE(TAG, "Received invalid raw-uart frame packet, length %d", length);
      return;
    }

    _radioModuleConnector->sendFrame(&data[2], length - 4);
    break;

  default:
    ESP_LOGE(TAG, "Received invalid raw-uart packet with unknown type %d", data[0]);
    break;
  }
}

void RawUartUdpListener::sendMessage(unsigned char command, unsigned char *buffer, size_t len)
{
  uint16_t port = atomic_load(&_remotePort);
  uint32_t address = atomic_load(&_remoteAddress);

  pbuf *pb = pbuf_alloc(PBUF_TRANSPORT, len + 4, PBUF_RAM);
  unsigned char *sendBuffer = (unsigned char *)pb->payload;

  ip_addr_t addr;
  addr.type = IPADDR_TYPE_V4;
  addr.u_addr.ip4.addr = address;

  if (!port)
    return;

  sendBuffer[0] = command;
  sendBuffer[1] = (unsigned char)atomic_fetch_add(&_counter, 1);

  if (len)
    memcpy(sendBuffer + 2, buffer, len);

  *((uint16_t *)(sendBuffer + len + 2)) = htons(HMFrame::crc(sendBuffer, len + 2));

  _udp_sendto(_pcb, pb, &addr, port);
  pbuf_free(pb);
}

void RawUartUdpListener::handleFrame(unsigned char *buffer, uint16_t len)
{
  if (!atomic_load(&_connectionStarted))
    return;

  if (len > (1500 - 28 - 4))
  {
    ESP_LOGE(TAG, "Received oversized frame from radio module, length %d", len);
    return;
  }

  sendMessage(7, buffer, len);
}

void RawUartUdpListener::start()
{
  _udp_queue = xQueueCreate(32, sizeof(udp_event_t *));
  xTaskCreate(_raw_uart_udpQueueHandlerTask, "RawUartUdpListener_UDP_QueueHandler", 4096, this, 15, &_tHandle);

  _pcb = udp_new();
  udp_recv(_pcb, &_raw_uart_udpReceivePaket, (void *)this);

  _udp_bind(_pcb, IP4_ADDR_ANY, 3008);

  _radioModuleConnector->setFrameHandler(this);
}

void RawUartUdpListener::stop()
{
  _udp_disconnect(_pcb);
  udp_recv(_pcb, NULL, NULL);
  _udp_remove(_pcb);
  _pcb = NULL;

  _radioModuleConnector->setFrameHandler(NULL);
  vTaskDelete(_tHandle);
}

void RawUartUdpListener::_udpQueueHandler()
{
  udp_event_t *event = NULL;
  int64_t nextKeepAliveSentOut = esp_timer_get_time();

  for (;;)
  {
    if (xQueueReceive(_udp_queue, &event, (portTickType)(100 / portTICK_PERIOD_MS)) == pdTRUE)
    {
      handlePacket(event->pb, event->addr, event->port);
      pbuf_free(event->pb);
      free(event);
    }

    if (atomic_load(&_remotePort) != 0)
    {
      int64_t now = esp_timer_get_time();

      if (now > _lastReceivedKeepAlive + 1500000)
      { // 1.5 sec
        atomic_store(&_remotePort, (ushort)0);
        atomic_store(&_connectionStarted, false);
        atomic_store(&_remoteAddress, 0u);
        _radioModuleConnector->setLED(true, false, false);
        ESP_LOGE(TAG, "Connection timed out");
      }

      if (now > nextKeepAliveSentOut)
      {
        nextKeepAliveSentOut = now + 1000000; // 1sec
        sendMessage(2, NULL, 0);
      }
    }
  }

  vTaskDelete(NULL);
}

bool RawUartUdpListener::_udpReceivePacket(pbuf *pb, const ip_addr_t *addr, uint16_t port)
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

/*
Index 0 - Type: 0-Connect, 1-Disconnect, 2-KeepAlive, 3-LED, 4-StartConn, 5-StopConn, 6-Reset, 7-Frame
Index 1 - Counter
Index 2..n-2 - Payload
Index n-2,n-1 - CRC16

Payload:
  Keepalive: Empty
  Connect: Empty
  LED: 1 Byte: Bit 0 R, Bit 1 G, Bit 2 B
  Reset: Empty
  Frame: Frame-Data
*/
