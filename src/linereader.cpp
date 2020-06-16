/* 
 *  linereader.cpp is part of the HB-RF-ETH firmware - https://github.com/alexreinert/HB-RF-ETH
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

#include "linereader.h"
#include <stdint.h>

LineReader::LineReader(void (*processor)(char *buffer, uint8_t len)) : _processor(processor), _buffer_pos(0)
{
}

void LineReader::AppendChar(char chr)
{
    switch (chr)
    {
    case '\r':
        return;

    case '\n':
        _buffer[_buffer_pos++] = 0;
        _processor(_buffer, _buffer_pos);
        _buffer_pos = 0;
        break;

    default:
        _buffer[_buffer_pos++] = chr;
        break;
    }
}

void LineReader::Reset()
{
    _buffer_pos = 0;
}
