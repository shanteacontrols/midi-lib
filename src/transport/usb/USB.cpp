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

#include "USB.h"

using namespace MIDIlib;

bool USBMIDI::Transport::init()
{
    _txIndex = 0;
    _rxIndex = 0;
    _usbMIDI.useRecursiveParsing(true);

    return _usbMIDI._hwa.init();
}

bool USBMIDI::Transport::deInit()
{
    return _usbMIDI._hwa.deInit();
}

bool USBMIDI::Transport::beginTransmission(messageType_t type)
{
    _activeType          = type;
    _txBuffer[USB_EVENT] = usbMIDIHeader(_cin, static_cast<uint8_t>(type));
    _txIndex             = 0;

    return true;
}

bool USBMIDI::Transport::write(uint8_t data)
{
    bool returnValue = true;

    if (_activeType != messageType_t::systemExclusive)
    {
        _txBuffer[_txIndex + 1] = data;
    }
    else if (data == 0xF0)
    {
        // start of sysex
        _txBuffer[USB_EVENT] = usbMIDIHeader(_cin, static_cast<uint8_t>(systemEvent_t::sysExStart));
        _txBuffer[USB_DATA1] = data;
    }
    else
    {
        auto i = _txIndex % 3;

        if (data == 0xF7)
        {
            // End of sysex:
            // this event has sysExStop1byte as event index with added count of how many bytes there are in USB packet.
            // Add 0x10 since event is shifted 4 bytes to the left.

            _txBuffer[USB_EVENT] = usbMIDIHeader(_cin, (static_cast<uint8_t>(systemEvent_t::sysExStop1byte) + (0x10 * i)));
        }

        switch (i)
        {
        case 0:
        {
            _txBuffer[USB_DATA1] = data;
            _txBuffer[USB_DATA2] = 0;
            _txBuffer[USB_DATA3] = 0;
        }
        break;

        case 1:
        {
            _txBuffer[USB_DATA2] = data;
            _txBuffer[USB_DATA3] = 0;
        }
        break;

        case 2:
        {
            _txBuffer[USB_DATA3] = data;

            if (data != 0xF7)
            {
                returnValue = endTransmission();
            }
        }
        break;

        default:
            break;
        }
    }

    _txIndex++;

    return returnValue;
}

bool USBMIDI::Transport::endTransmission()
{
    return _usbMIDI._hwa.write(_txBuffer);
}

bool USBMIDI::Transport::read(uint8_t& data)
{
    if (!_rxIndex)
    {
        USBMIDI::usbMIDIPacket_t packet;

        if (!_usbMIDI._hwa.read(packet))
        {
            return false;
        }

        // We already have entire message here.
        // MIDIEvent.Event is CIN, see midi10.pdf.
        // Shift CIN four bytes left to get messageType_t.
        uint8_t midiMessage = packet[USB_EVENT] << 4;

        switch (midiMessage)
        {
        // 1 byte messages
        case static_cast<uint8_t>(systemEvent_t::sysCommon1byte):
        case static_cast<uint8_t>(systemEvent_t::singleByte):
        {
            _rxBuffer[_rxIndex++] = packet[USB_DATA1];
        }
        break;

        // 2 byte messages
        case static_cast<uint8_t>(systemEvent_t::sysCommon2byte):
        case static_cast<uint8_t>(messageType_t::programChange):
        case static_cast<uint8_t>(messageType_t::afterTouchChannel):
        case static_cast<uint8_t>(messageType_t::sysCommonTimeCodeQuarterFrame):
        case static_cast<uint8_t>(messageType_t::sysCommonSongSelect):
        case static_cast<uint8_t>(systemEvent_t::sysExStop2byte):
        {
            _rxBuffer[_rxIndex++] = packet[USB_DATA2];
            _rxBuffer[_rxIndex++] = packet[USB_DATA1];
        }
        break;

        // 3 byte messages
        case static_cast<uint8_t>(messageType_t::noteOn):
        case static_cast<uint8_t>(messageType_t::noteOff):
        case static_cast<uint8_t>(messageType_t::controlChange):
        case static_cast<uint8_t>(messageType_t::pitchBend):
        case static_cast<uint8_t>(messageType_t::afterTouchPoly):
        case static_cast<uint8_t>(messageType_t::sysCommonSongPosition):
        case static_cast<uint8_t>(systemEvent_t::sysExStart):
        case static_cast<uint8_t>(systemEvent_t::sysExStop3byte):
        {
            _rxBuffer[_rxIndex++] = packet[USB_DATA3];
            _rxBuffer[_rxIndex++] = packet[USB_DATA2];
            _rxBuffer[_rxIndex++] = packet[USB_DATA1];
        }
        break;

        default:
            return false;
        }
    }

    if (_rxIndex)
    {
        data = _rxBuffer[_rxIndex - 1];
        _rxIndex--;

        return true;
    }

    return false;
}