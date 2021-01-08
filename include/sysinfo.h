/* 
 *  sysinfo.h is part of the HB-RF-ETH firmware - https://github.com/alexreinert/HB-RF-ETH
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
