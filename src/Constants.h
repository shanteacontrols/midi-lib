/*
    Copyright 2016 Francois Best
    Copyright 2017 Igor Petrovic

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

///
/// \brief Value used to listen incoming MIDI messages on all channels.
///
#define MIDI_CHANNEL_OMNI       17

///
/// \brief Value used to turn the listening of incoming MIDI messages off.
///
#define MIDI_CHANNEL_OFF        18

///
/// \brief Value used to signal the invalid MIDI channel.
///
#define MIDI_CHANNEL_INVALID    255

///
/// \brief Values defining MIDI Pitch Bend range.
/// @{
///

#define MIDI_PITCHBEND_MIN      -8192
#define MIDI_PITCHBEND_MAX      8191

/// @}

///
/// \brief Maximum 7-bit MIDI value.
///
#define MIDI_7_BIT_VALUE_MAX    127

///
/// \brief Maximum 14-bit MIDI value.
///
#define MIDI_14_BIT_VALUE_MAX   16383