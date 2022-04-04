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

#include "../../MIDI.h"

#ifndef MIDI_BLE_MAX_PACKET_SIZE
#define MIDI_BLE_MAX_PACKET_SIZE 64
#endif

namespace MIDIlib
{
    class BLEMIDI : public Base
    {
        public:
        struct bleMIDIPacket_t
        {
            std::array<uint8_t, MIDI_BLE_MAX_PACKET_SIZE> data = {};
            size_t                                        size = 0;
        };

        class HWA
        {
            public:
            virtual bool     init()                         = 0;
            virtual bool     deInit()                       = 0;
            virtual bool     write(bleMIDIPacket_t& packet) = 0;
            virtual bool     read(bleMIDIPacket_t& packet)  = 0;
            virtual uint32_t time()                         = 0;
        };

        BLEMIDI(HWA& hwa)
            : Base(_transport)
            , _transport(*this)
            , _hwa(hwa)
        {}

        private:
        class Transport : public Base::Transport
        {
            public:
            Transport(BLEMIDI& bleMIDI)
                : _bleMIDI(bleMIDI)
            {}

            bool init() override;
            bool deInit() override;
            bool beginTransmission(messageType_t type) override;
            bool write(uint8_t data) override;
            bool endTransmission() override;
            bool read(uint8_t& data) override;

            private:
            BLEMIDI&                                      _bleMIDI;
            bleMIDIPacket_t                               _txBuffer      = {};
            size_t                                        _rxIndex       = 0;
            size_t                                        _retrieveIndex = 0;
            std::array<uint8_t, MIDI_BLE_MAX_PACKET_SIZE> _rxBuffer      = {};
            uint8_t                                       _lowTimestamp  = 0;
        } _transport;

        BLEMIDI::HWA& _hwa;
    };
}    // namespace MIDIlib