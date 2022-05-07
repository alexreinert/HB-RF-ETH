/* 
 *  settings.h is part of the HB-RF-ETH firmware - https://github.com/alexreinert/HB-RF-ETH
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

#include <stdio.h>
#include <lwip/ip4_addr.h>

typedef enum
{
    TIMESOURCE_NTP = 0,
    TIMESOURCE_DCF = 1,
    TIMESOURCE_GPS = 2
} timesource_t;

class Settings
{
private:
  char _adminPassword[33] = {0};

  char _hostname[33] = {0};
  bool _useDHCP;
  ip4_addr_t _localIP;
  ip4_addr_t _netmask;
  ip4_addr_t _gateway;
  ip4_addr_t _dns1;
  ip4_addr_t _dns2;

  int _timesource;

  int _dcfOffset;

  int _gpsBaudrate;

  char _ntpServer[65] = {0};

  int _ledBrightness;

public:
  Settings();
  void load();
  void save();
  void clear();

  char *getAdminPassword();
  void setAdminPassword(char* password);

  char *getHostname();
  bool getUseDHCP();
  ip4_addr_t getLocalIP();
  ip4_addr_t getNetmask();
  ip4_addr_t getGateway();
  ip4_addr_t getDns1();
  ip4_addr_t getDns2();

  void setNetworkSettings(char *hostname, bool useDHCP, ip4_addr_t localIP, ip4_addr_t netmask, ip4_addr_t gateway, ip4_addr_t dns1, ip4_addr_t dns2);

  timesource_t getTimesource();
  void setTimesource(timesource_t timesource);

  int getDcfOffset();
  void setDcfOffset(int offset);

  int getGpsBaudrate();
  void setGpsBaudrate(int baudrate);

  char *getNtpServer();
  void setNtpServer(char *ntpServer);

  int getLEDBrightness();
  void setLEDBrightness(int brightness);
};