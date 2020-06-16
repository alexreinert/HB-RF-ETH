/* 
 *  streamparser.cpp is part of the HB-RF-ETH firmware - https://github.com/alexreinert/HB-RF-ETH
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

#include "streamparser.h"
#include <stdint.h>

unsigned char _buffer[2048];
uint16_t _buffer_pos;
uint16_t _frame_pos;
uint16_t _frame_length;
state_t _state;
bool _is_escaped;
bool _decode_escaped;
std::function<void(unsigned char *buffer, uint16_t len)> _processor;

StreamParser::StreamParser(bool decode_escaped, std::function<void(unsigned char *buffer, uint16_t len)> processor) : _buffer_pos(0), _state(NO_DATA), _is_escaped(false), _decode_escaped(decode_escaped), _processor(processor)
{
}

void StreamParser::Append(unsigned char chr)
{
    switch (chr)
    {
    case 0xfd:
        _buffer_pos = 0;
        _is_escaped = false;
        _state = RECEIVE_LENGTH_HIGH_BYTE;
        break;

    case 0xfc:
        _is_escaped = true;
        if (_decode_escaped)
            return;
        break;

    default:
        if (_is_escaped && _decode_escaped)
            chr |= 0x80;

        switch (_state)
        {
        case NO_DATA:
        case FRAME_COMPLETE:
            return; // Do nothing until the first frame prefix occurs

        case RECEIVE_LENGTH_HIGH_BYTE:
            _frame_length = (_is_escaped ? chr | 0x80 : chr) << 8;
            _state = RECEIVE_LENGTH_LOW_BYTE;
            break;

        case RECEIVE_LENGTH_LOW_BYTE:
            _frame_length |= (_is_escaped ? chr | 0x80 : chr);
            _frame_length += 2; // handle crc as frame data
            _frame_pos = 0;
            _state = RECEIVE_FRAME_DATA;
            break;

        case RECEIVE_FRAME_DATA:
            _frame_pos++;
            _state = (_frame_pos == _frame_length) ? FRAME_COMPLETE : RECEIVE_FRAME_DATA;
            break;
        }
        _is_escaped = false;
    }

    _buffer[_buffer_pos++] = chr;

    if (_buffer_pos == sizeof(_buffer))
        _state = FRAME_COMPLETE;

    if (_state == FRAME_COMPLETE)
    {
        _processor(_buffer, _buffer_pos);
        _state = NO_DATA;
    }
}

void StreamParser::Append(unsigned char *buffer, uint16_t len)
{
    int i;
    for (i = 0; i < len; i++)
    {
        Append(buffer[i]);
    }
}

void StreamParser::Flush()
{
    _state = NO_DATA;
    _buffer_pos = 0;
    _is_escaped = false;
}
