/* 
 *  ethernet.cpp is part of the HB-RF-ETH firmware - https://github.com/alexreinert/HB-RF-ETH
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

#include "ethernet.h"
#include "pins.h"

static const char *TAG = "Ethernet";

void _handleETHEvent(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    reinterpret_cast<Ethernet *>(arg)->_handleETHEvent(event_base, event_id, event_data);
}

void _handleIPEvent(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    reinterpret_cast<Ethernet *>(arg)->_handleIPEvent(event_base, event_id, event_data);
}

Ethernet::Ethernet(Settings *settings) : _settings(settings), _isConnected(false)
{
    tcpip_adapter_init();

    if (settings->getUseDHCP())
    {
        ESP_ERROR_CHECK_WITHOUT_ABORT(tcpip_adapter_dhcpc_start(TCPIP_ADAPTER_IF_ETH));
    }
    else
    {
        ESP_ERROR_CHECK_WITHOUT_ABORT(tcpip_adapter_dhcpc_stop(TCPIP_ADAPTER_IF_ETH));

        tcpip_adapter_ip_info_t ipInfo;
        ipInfo.ip = settings->getLocalIP();
        ipInfo.netmask = settings->getNetmask();
        ipInfo.gw = settings->getGateway();
        ESP_ERROR_CHECK_WITHOUT_ABORT(tcpip_adapter_set_ip_info(TCPIP_ADAPTER_IF_ETH, &ipInfo));

        tcpip_adapter_dns_info_t dnsInfo;
        dnsInfo.ip.type = IPADDR_TYPE_V4;
        dnsInfo.ip.u_addr.ip4.addr = settings->getDns1().addr;
        if (dnsInfo.ip.u_addr.ip4.addr != IPADDR_ANY && dnsInfo.ip.u_addr.ip4.addr != IPADDR_NONE)
        {
            ESP_ERROR_CHECK_WITHOUT_ABORT(tcpip_adapter_set_dns_info(TCPIP_ADAPTER_IF_ETH, TCPIP_ADAPTER_DNS_MAIN, &dnsInfo));
        }

        dnsInfo.ip.u_addr.ip4.addr = settings->getDns2().addr;
        if (dnsInfo.ip.u_addr.ip4.addr != IPADDR_ANY && dnsInfo.ip.u_addr.ip4.addr != IPADDR_NONE)
        {
            ESP_ERROR_CHECK_WITHOUT_ABORT(tcpip_adapter_set_dns_info(TCPIP_ADAPTER_IF_ETH, TCPIP_ADAPTER_DNS_BACKUP, &dnsInfo));
        }
    }

    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_event_loop_create_default());

    ESP_ERROR_CHECK_WITHOUT_ABORT(tcpip_adapter_set_default_eth_handlers());
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID, &::_handleETHEvent, (void *)this));
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, &::_handleIPEvent, (void *)this));

    _phy_config = ETH_PHY_DEFAULT_CONFIG();
    _phy_config.phy_addr = ETH_PHY_ADDR;
    _phy_config.reset_gpio_num = ETH_POWER_PIN;
    _phy = esp_eth_phy_new_lan8720(&_phy_config);

    _mac_config = ETH_MAC_DEFAULT_CONFIG();
    _mac_config.smi_mdc_gpio_num = ETH_MDC_PIN;
    _mac_config.smi_mdio_gpio_num = ETH_MDIO_PIN;
    _mac = esp_eth_mac_new_esp32(&_mac_config);

    _eth_config = ETH_DEFAULT_CONFIG(_mac, _phy);
    _eth_handle = NULL;
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_eth_driver_install(&_eth_config, &_eth_handle));
}

void Ethernet::start()
{
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_eth_start(_eth_handle));
}

void Ethernet::stop()
{
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_eth_stop(_eth_handle));
}

bool Ethernet::getIsConnected()
{
    return _isConnected;
}

void Ethernet::_handleETHEvent(esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    esp_eth_handle_t eth_handle = *(esp_eth_handle_t *)event_data;
    uint8_t mac_addr[6] = {0};

    switch (event_id)
    {
    case ETHERNET_EVENT_CONNECTED:
        ESP_ERROR_CHECK_WITHOUT_ABORT(esp_eth_ioctl(eth_handle, ETH_CMD_G_MAC_ADDR, mac_addr));
        ESP_LOGI(TAG, "Link Up");
        ESP_LOGI(TAG, "HW Addr %02x:%02x:%02x:%02x:%02x:%02x", mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
        break;
    case ETHERNET_EVENT_DISCONNECTED:
        _isConnected = false;
        ESP_LOGI(TAG, "Link Down");
        break;
    case ETHERNET_EVENT_START:
        ESP_LOGI(TAG, "Started");
        ESP_ERROR_CHECK_WITHOUT_ABORT(tcpip_adapter_set_hostname(TCPIP_ADAPTER_IF_ETH, _settings->getHostname()));
        break;
    case ETHERNET_EVENT_STOP:
        _isConnected = false;
        ESP_LOGI(TAG, "Stopped");
        break;
    default:
        break;
    }
}

void Ethernet::_handleIPEvent(esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
    const tcpip_adapter_ip_info_t *ip_info = &event->ip_info;

    _isConnected = true;
    ESP_LOGI(TAG, "IPv4: " IPSTR, IP2STR(&ip_info->ip));
}
