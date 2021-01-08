/* 
 *  ethernet.cpp is part of the HB-RF-ETH firmware - https://github.com/alexreinert/HB-RF-ETH
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
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_netif_init());

    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_event_loop_create_default());

    _netif_cfg = ESP_NETIF_DEFAULT_ETH();
    _eth_netif = esp_netif_new(&_netif_cfg);

    if (settings->getUseDHCP())
    {
        ESP_ERROR_CHECK_WITHOUT_ABORT(esp_netif_dhcpc_start(_eth_netif));
    }
    else
    {
        ESP_ERROR_CHECK_WITHOUT_ABORT(esp_netif_dhcpc_stop(_eth_netif));

        esp_netif_ip_info_t ipInfo;
        ipInfo.ip.addr = settings->getLocalIP().addr;
        ipInfo.netmask.addr = settings->getNetmask().addr;
        ipInfo.gw.addr = settings->getGateway().addr;
        ESP_ERROR_CHECK_WITHOUT_ABORT(esp_netif_set_ip_info(_eth_netif, &ipInfo));

        esp_netif_dns_info_t dnsInfo;
        dnsInfo.ip.type = ESP_IPADDR_TYPE_V4;
        dnsInfo.ip.u_addr.ip4.addr = settings->getDns1().addr;
        if (dnsInfo.ip.u_addr.ip4.addr != IPADDR_ANY && dnsInfo.ip.u_addr.ip4.addr != IPADDR_NONE)
        {
            ESP_ERROR_CHECK_WITHOUT_ABORT(esp_netif_set_dns_info(_eth_netif, ESP_NETIF_DNS_MAIN, &dnsInfo));
        }

        dnsInfo.ip.u_addr.ip4.addr = settings->getDns2().addr;
        if (dnsInfo.ip.u_addr.ip4.addr != IPADDR_ANY && dnsInfo.ip.u_addr.ip4.addr != IPADDR_NONE)
        {
            ESP_ERROR_CHECK_WITHOUT_ABORT(esp_netif_set_dns_info(_eth_netif, ESP_NETIF_DNS_BACKUP, &dnsInfo));
        }
    }

    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_eth_set_default_handlers(_eth_netif));
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
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_netif_attach(_eth_netif, esp_eth_new_netif_glue(_eth_handle)));
}

void Ethernet::start()
{
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_eth_start(_eth_handle));
}

void Ethernet::stop()
{
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_eth_stop(_eth_handle));
}

void Ethernet::getNetworkSettings(ip4_addr_t *ip, ip4_addr_t *netmask, ip4_addr_t *gateway, ip4_addr_t *dns1, ip4_addr_t *dns2)
{
    if (_isConnected)
    {
        esp_netif_ip_info_t ipInfo;
        esp_netif_get_ip_info(_eth_netif, &ipInfo);
        ip->addr = ipInfo.ip.addr;
        netmask->addr = ipInfo.netmask.addr;
        gateway->addr = ipInfo.gw.addr;

        esp_netif_dns_info_t dnsInfo;
        esp_netif_get_dns_info(_eth_netif, ESP_NETIF_DNS_MAIN, &dnsInfo);
        dns1->addr = dnsInfo.ip.u_addr.ip4.addr;
        esp_netif_get_dns_info(_eth_netif, ESP_NETIF_DNS_BACKUP, &dnsInfo);
        dns2->addr = dnsInfo.ip.u_addr.ip4.addr;
    }
    else
    {
        ip->addr = 0;
        netmask->addr = 0;
        gateway->addr = 0;
        dns1->addr = 0;
        dns2->addr = 0;
    }
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
        ESP_ERROR_CHECK_WITHOUT_ABORT(esp_netif_set_hostname(_eth_netif, _settings->getHostname()));
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
    const esp_netif_ip_info_t *ip_info = &event->ip_info;

    _isConnected = true;
    ESP_LOGI(TAG, "IPv4: " IPSTR, IP2STR(&ip_info->ip));
}
