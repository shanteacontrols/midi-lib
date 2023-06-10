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

#include "MIDI/transport/Serial.h"

using namespace MIDIlib;

bool SerialMIDI::Transport::init()
{
    _serialMIDI.useRecursiveParsing(true);
    return _serialMIDI._hwa.init();
}

bool SerialMIDI::Transport::deInit()
{
    return _serialMIDI._hwa.deInit();
}

bool SerialMIDI::Transport::beginTransmission(messageType_t type)
{
    // nothing to do
    return true;
}

bool SerialMIDI::Transport::write(uint8_t data)
{
    return _serialMIDI._hwa.write(data);
}

bool SerialMIDI::Transport::endTransmission()
{
    // nothing to do
    return true;
}

bool SerialMIDI::Transport::read(uint8_t& data)
{
    return _serialMIDI._hwa.read(data);
}