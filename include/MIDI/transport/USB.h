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

#pragma once

#include "MIDI/MIDI.h"
#include <array>
#include <inttypes.h>

namespace MIDIlib
{
    class USBMIDI : public Base
    {
        public:
        using usbMIDIPacket_t = std::array<uint8_t, 4>;

        enum usbPacketElement_t
        {
            USB_EVENT,    ///< MIDI event type, first byte in USB MIDI packet.
            USB_DATA1,    ///< First byte of data in USB MIDI packet.
            USB_DATA2,    ///< Second byte of data in USB MIDI packet.
            USB_DATA3     ///< Third byte of data in USB MIDI packet.
        };

        class HWA
        {
            public:
            virtual bool init()                         = 0;
            virtual bool deInit()                       = 0;
            virtual bool write(usbMIDIPacket_t& packet) = 0;
            virtual bool read(usbMIDIPacket_t& packet)  = 0;
        };

        USBMIDI(HWA& hwa, uint8_t cin = 0)
            : Base(_transport)
            , _transport(*this, cin)
            , _hwa(hwa)
        {}

        private:
        class Transport : public Base::Transport
        {
            public:
            Transport(USBMIDI& usbMIDI, uint8_t cin)
                : _usbMIDI(usbMIDI)
                , CIN(cin)
            {}

            bool init() override;
            bool deInit() override;
            bool beginTransmission(messageType_t type) override;
            bool write(uint8_t data) override;
            bool endTransmission() override;
            bool read(uint8_t& data) override;

            private:
            /// Used to construct a USB MIDI header from a given MIDI event and a virtual MIDI cable index.
            static constexpr uint8_t usbMIDIHeader(uint8_t virtualcable, uint8_t event)
            {
                return ((virtualcable << 4) | (event >> 4));
            }

            /// Enumeration holding USB-specific events for SysEx/System Common messages.
            enum class systemEvent_t : uint8_t
            {
                SYS_COMMON1BYTE  = 0x50,
                SYS_COMMON2BYTE  = 0x20,
                SYS_COMMON3BYTE  = 0x30,
                SINGLE_BYTE      = 0xF0,
                SYS_EX_START     = 0x40,
                SYS_EX_STOP1BYTE = 0x50,
                SYS_EX_STOP2BYTE = 0x60,
                SYS_EX_STOP3BYTE = 0x70
            };

            USBMIDI&        _usbMIDI;
            const uint8_t   CIN;
            uint8_t         _rxIndex     = 0;
            uint8_t         _rxBuffer[3] = {};
            usbMIDIPacket_t _txBuffer    = {};
            uint8_t         _txIndex     = 0;
            messageType_t   _activeType  = messageType_t::INVALID;
        } _transport;

        HWA& _hwa;
    };
}    // namespace MIDIlib
