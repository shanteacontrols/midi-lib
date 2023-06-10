/*
    Copyright 2017-2022 Igor Petrovic

    Permission is hereby granted, free of charge, to any person obtaining
    a copy of this software and associated documentation files (the "Software"),
    to deal in the Software without restriction, including without limitation
    the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
    sell copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
    OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
    THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "MIDI/transport/BLE.h"

using namespace MIDIlib;

bool BLEMIDI::Transport::init()
{
    _bleMIDI.useRecursiveParsing(true);
    return _bleMIDI._hwa.init();
}

bool BLEMIDI::Transport::deInit()
{
    return _bleMIDI._hwa.deInit();
}

bool BLEMIDI::Transport::beginTransmission(messageType_t type)
{
    // timestamp is 13-bit according to midi ble spec
    auto time = _bleMIDI._hwa.time() & 0x01FFF;

    _txBuffer.data[0] = (time >> 7) | 0x80;      // 6 bits plus MSB
    _txBuffer.data[1] = (time & 0x7F) | 0x80;    // 7 bits plus MSB
    _lowTimestamp     = _txBuffer.data[1];
    _txBuffer.size    = 2;

    return true;
}

bool BLEMIDI::Transport::write(uint8_t data)
{
    _txBuffer.data[_txBuffer.size++] = data;

    if (_txBuffer.size >= _txBuffer.data.size())
    {
        bool retVal    = _bleMIDI._hwa.write(_txBuffer);
        _txBuffer.size = 1;    // keep header
        return retVal;
    }

    return true;
}

bool BLEMIDI::Transport::endTransmission()
{
    return _bleMIDI._hwa.write(_txBuffer);
}

bool BLEMIDI::Transport::read(uint8_t& data)
{
    if (!_rxIndex)
    {
        bleMIDIPacket_t packet;

        if (!_bleMIDI._hwa.read(packet))
        {
            return false;
        }

        if (packet.size > MIDI_BLE_MAX_PACKET_SIZE)
        {
            return false;
        }

        size_t index           = 0;
        bool   searchTimestamp = true;

        // ignore header
        while (++index < packet.size)
        {
            if (searchTimestamp)
            {
                if (!(packet.data[index] & 0x80))
                {
                    if (index == 1)
                    {
                        // sysex continuation, store
                        _rxBuffer[_rxIndex++] = packet.data[index];
                    }
                    else
                    {
                        // something is wrong
                        break;
                    }
                }

                // start filling buffer from the next byte
                searchTimestamp = false;
            }
            else
            {
                _rxBuffer[_rxIndex++] = packet.data[index];

                if (index < (packet.size - 1))
                {
                    if (packet.data[index + 1] & 0x80)
                    {
                        // for the next time
                        searchTimestamp = true;
                    }
                }
            }
        }
    }

    if (_rxIndex)
    {
        data = _rxBuffer[_retrieveIndex++];

        if (_retrieveIndex == _rxIndex)
        {
            _retrieveIndex = 0;
            _rxIndex       = 0;
        }

        return true;
    }

    return false;
}