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

#include "common.h"
#include "lib/midi/midi.h"

namespace lib::midi::ble
{
    class Ble : public Base
    {
        public:
        Ble(Hwa& hwa)
            : Base(_transport)
            , _transport(*this)
            , _hwa(hwa)
        {}

        private:
        class Transport : public lib::midi::Transport
        {
            public:
            Transport(Ble& ble)
                : _ble(ble)
            {}

            bool init() override;
            bool deInit() override;
            bool beginTransmission(messageType_t type) override;
            bool write(uint8_t data) override;
            bool endTransmission() override;
            bool read(uint8_t& data) override;

            private:
            Ble&                                          _ble;
            Packet                                        _txBuffer      = {};
            size_t                                        _rxIndex       = 0;
            size_t                                        _retrieveIndex = 0;
            std::array<uint8_t, MIDI_BLE_MAX_PACKET_SIZE> _rxBuffer      = {};
            uint8_t                                       _lowTimestamp  = 0;
        } _transport;

        Hwa& _hwa;
    };
}    // namespace lib::midi::ble