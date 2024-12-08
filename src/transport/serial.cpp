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

#include "lib/midi/transport/serial/serial.h"

using namespace lib::midi::serial;

bool Serial::Transport::init()
{
    _serial.useRecursiveParsing(true);
    return _serial._hwa.init();
}

bool Serial::Transport::deInit()
{
    return _serial._hwa.deInit();
}

bool Serial::Transport::beginTransmission(messageType_t type)
{
    // nothing to do
    return true;
}

bool Serial::Transport::write(uint8_t data)
{
    Packet packet = { data };

    return _serial._hwa.write(packet);
}

bool Serial::Transport::endTransmission()
{
    // nothing to do
    return true;
}

bool Serial::Transport::read(uint8_t& data)
{
    Packet packet = {};

    if (!_serial._hwa.read(packet))
    {
        return false;
    }

    data = packet.data;

    return true;
}