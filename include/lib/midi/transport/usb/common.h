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

#include <array>
#include <inttypes.h>

namespace lib::midi::usb
{
    struct Packet
    {
        std::array<uint8_t, 4> data = {};

        enum packetElement_t
        {
            USB_EVENT,    ///< MIDI event type, first byte in USB MIDI packet.
            USB_DATA1,    ///< First byte of data in USB MIDI packet.
            USB_DATA2,    ///< Second byte of data in USB MIDI packet.
            USB_DATA3     ///< Third byte of data in USB MIDI packet.
        };
    };

    class Hwa
    {
        public:
        virtual bool init()                = 0;
        virtual bool deInit()              = 0;
        virtual bool write(Packet& packet) = 0;
        virtual bool read(Packet& packet)  = 0;
    };
}    // namespace lib::midi::usb