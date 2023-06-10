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

namespace MIDIlib
{
    class SerialMIDI : public Base
    {
        public:
        class HWA
        {
            public:
            virtual bool init()               = 0;
            virtual bool deInit()             = 0;
            virtual bool write(uint8_t& data) = 0;
            virtual bool read(uint8_t& data)  = 0;
        };

        SerialMIDI(HWA& hwa)
            : Base(_transport)
            , _transport(*this)
            , _hwa(hwa)
        {}

        private:
        class Transport : public Base::Transport
        {
            public:
            Transport(SerialMIDI& serialMIDI)
                : _serialMIDI(serialMIDI)
            {}

            bool init() override;
            bool deInit() override;
            bool beginTransmission(messageType_t type) override;
            bool write(uint8_t data) override;
            bool endTransmission() override;
            bool read(uint8_t& data) override;

            private:
            SerialMIDI& _serialMIDI;
        } _transport;

        SerialMIDI::HWA& _hwa;
    };
}    // namespace MIDIlib