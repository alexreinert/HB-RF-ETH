/* 
 *  settings.cpp is part of the HB-RF-ETH firmware - https://github.com/alexreinert/HB-RF-ETH
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

#include "settings.h"
#include "nvs.h"
#include "nvs_flash.h"
#include <string.h>

Settings::Settings()
{
  load();
}

static const char *TAG = "Settings";
static const char *NVS_NAMESPACE = "HB-RF-ETH";

#define GET_IP_ADDR(handle, name, var, defaultValue)  \
  if (nvs_get_u32(handle, name, &var.addr) != ESP_OK) \
  {                                                   \
    var.addr = defaultValue;                          \
  }

#define GET_INT(handle, name, var, defaultValue) \
  if (nvs_get_i32(handle, name, &var) != ESP_OK) \
  {                                              \
    var = defaultValue;                          \
  }

#define GET_BOOL(handle, name, var, defaultValue)          \
  int8_t __##var##_temp;                                   \
  if (nvs_get_i8(handle, name, &__##var##_temp) != ESP_OK) \
  {                                                        \
    var = defaultValue;                                    \
  }                                                        \
  else                                                     \
  {                                                        \
    var = (__##var##_temp != 0);                           \
  }

#define SET_IP_ADDR(handle, name, var) ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_set_u32(handle, name, var.addr));
#define SET_INT(handle, name, var) ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_set_i32(handle, name, var));
#define SET_STR(handle, name, var) ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_set_str(handle, name, var));
#define SET_BOOL(handle, name, var) ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_set_i8(handle, name, var ? 1 : 0));

void Settings::load()
{
  uint32_t handle;

  esp_err_t err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
  {
    ESP_ERROR_CHECK(nvs_flash_erase());
    err = nvs_flash_init();
  }
  ESP_ERROR_CHECK(err);

  ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle));

  size_t adminPasswordLength = sizeof(_adminPassword);
  if (nvs_get_str(handle, "adminPassword", _adminPassword, &adminPasswordLength) != ESP_OK)
  {
    snprintf(_adminPassword, sizeof(_adminPassword), "admin");
  }

  size_t hostnameLength = sizeof(_hostname);
  if (nvs_get_str(handle, "hostname", _hostname, &hostnameLength) != ESP_OK)
  {
    uint8_t baseMac[6];
    esp_read_mac(baseMac, ESP_MAC_ETH);
    snprintf(_hostname, sizeof(_hostname), "HB-RF-ETH-%02X%02X%02X", baseMac[3], baseMac[4], baseMac[5]);
  }

  GET_BOOL(handle, "useDHCP", _useDHCP, true);
  GET_IP_ADDR(handle, "localIP", _localIP, IPADDR_ANY);
  GET_IP_ADDR(handle, "netmask", _netmask, IPADDR_ANY);
  GET_IP_ADDR(handle, "gateway", _gateway, IPADDR_ANY);
  GET_IP_ADDR(handle, "dns1", _dns1, IPADDR_ANY);
  GET_IP_ADDR(handle, "dns2", _dns2, IPADDR_ANY);

  GET_INT(handle, "timesource", _timesource, TIMESOURCE_NTP);
  
  GET_INT(handle, "dcfOffset", _dcfOffset, 40000);

  GET_INT(handle, "gpsBaudrate", _gpsBaudrate, 9600);

  size_t ntpServerLength = sizeof(_ntpServer);
  if (nvs_get_str(handle, "ntpServer", _ntpServer, &ntpServerLength) != ESP_OK)
  {
    snprintf(_ntpServer, sizeof(_ntpServer), "pool.ntp.org");
  }

  GET_INT(handle, "ledBrightness", _ledBrightness, 100);

  nvs_close(handle);
}

void Settings::save()
{
  uint32_t handle;

  ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle));

  SET_STR(handle, "adminPassword", _adminPassword);

  SET_STR(handle, "hostname", _hostname);
  SET_BOOL(handle, "useDHCP", _useDHCP);
  SET_IP_ADDR(handle, "localIP", _localIP);
  SET_IP_ADDR(handle, "netmask", _netmask);
  SET_IP_ADDR(handle, "gateway", _gateway);
  SET_IP_ADDR(handle, "dns1", _dns1);
  SET_IP_ADDR(handle, "dns2", _dns2);

  SET_INT(handle, "timesource", _timesource);

  SET_INT(handle, "dcfOffset", _dcfOffset);

  SET_INT(handle, "gpsBaudrate", _gpsBaudrate);

  SET_STR(handle, "ntpServer", _ntpServer);

  SET_INT(handle, "ledBrightness", _ledBrightness);

  nvs_close(handle);
}

void Settings::clear()
{
  uint32_t handle;

  ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle));
  ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_erase_all(handle));
  ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_commit(handle));
  nvs_close(handle);

  load();
}

char *Settings::getAdminPassword()
{
  return _adminPassword;
}

void Settings::setAdminPassword(char *adminPassword)
{
  snprintf(_adminPassword, sizeof(_adminPassword), adminPassword);
}

char *Settings::getHostname()
{
  return _hostname;
}

bool Settings::getUseDHCP()
{
  return _useDHCP;
}

ip4_addr_t Settings::getLocalIP()
{
  return _localIP;
}

ip4_addr_t Settings::getNetmask()
{
  return _netmask;
}

ip4_addr_t Settings::getGateway()
{
  return _gateway;
}

ip4_addr_t Settings::getDns1()
{
  return _dns1;
}

ip4_addr_t Settings::getDns2()
{
  return _dns2;
}

void Settings::setNetworkSettings(char *hostname, bool useDHCP, ip4_addr_t localIP, ip4_addr_t netmask, ip4_addr_t gateway, ip4_addr_t dns1, ip4_addr_t dns2)
{
  snprintf(_hostname, sizeof(_hostname), hostname);
  _useDHCP = useDHCP;
  _localIP = localIP;
  _netmask = netmask;
  _gateway = gateway;
  _dns1 = dns1;
  _dns2 = dns2;
}

int Settings::getDcfOffset()
{
  return _dcfOffset;
}

void Settings::setDcfOffset(int dcfOffset)
{
  _dcfOffset = dcfOffset;
}

int Settings::getGpsBaudrate()
{
  return _gpsBaudrate;
}

void Settings::setGpsBaudrate(int gpsBaudrate)
{
  _gpsBaudrate = gpsBaudrate;
}

timesource_t Settings::getTimesource()
{
  return (timesource_t)_timesource;
}

void Settings::setTimesource(timesource_t timesource)
{
  _timesource = timesource;
}

char *Settings::getNtpServer()
{
  return _ntpServer;
}

void Settings::setNtpServer(char *ntpServer)
{
  snprintf(_ntpServer, sizeof(_ntpServer), ntpServer);
}

int Settings::getLEDBrightness()
{
  return _ledBrightness;
}

void Settings::setLEDBrightness(int ledBrightness)
{
  _ledBrightness = ledBrightness;
}
