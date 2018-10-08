/*
    Copyright 2016 Francois Best
    Copyright 2017-2018 Igor Petrovic

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

#include <stddef.h>
#include <inttypes.h>

///
/// \brief Extracts lower 7 bits from 14-bit value.
/// \param [in] value   14-bit value.
/// \returns    Lower 7 bits.
///
#define lowByte_7bit(value)             ((value) & 0x7F)

///
/// \brief Extracts upper 7 bits from 14-bit value.
/// \param [in] value   14-bit value.
/// \returns    Upper 7 bits.
///
#define highByte_7bit(value)            ((value >> 7) & 0x7f)

///
/// \brief Constructs a USB MIDI event ID from a given MIDI command and a virtual MIDI cable index.
///
/// This can then be used to create and decode MIDI event packets.
/// \param [in] virtualcable    Index of the virtual MIDI cable the event relates to.
/// \param [in] command         MIDI command to send through the virtual MIDI cable.
/// \returns    Constructed USB MIDI event ID.
///
#define GET_USB_MIDI_EVENT(virtualcable, command)  (((virtualcable) << 4) | ((command) >> 4))
