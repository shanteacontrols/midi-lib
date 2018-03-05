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

#include "Helpers.h"

#ifndef MIDI_SYSEX_ARRAY_SIZE
#warning Please define MIDI_SYSEX_ARRAY_SIZE. Setting size to 1 byte.
#define MIDI_SYSEX_ARRAY_SIZE 1
#endif

///
/// \brief Enumeration holding two different types of MIDI interfaces.
///
typedef enum
{
    dinInterface,
    usbInterface
} midiInterfaceType_t;

///
/// \brief Enumeration holding various types of MIDI messages.
///
typedef enum
{
    midiMessageNoteOff              = 0x80, ///< Note Off
    midiMessageNoteOn               = 0x90, ///< Note On
    midiMessageControlChange        = 0xB0, ///< Control Change / Channel Mode
    midiMessageProgramChange        = 0xC0, ///< Program Change
    midiMessageAfterTouchChannel    = 0xD0, ///< Channel (monophonic) AfterTouch
    midiMessageAfterTouchPoly       = 0xA0, ///< Polyphonic AfterTouch
    midiMessagePitchBend            = 0xE0, ///< Pitch Bend
    midiMessageSystemExclusive      = 0xF0, ///< System Exclusive
    midiMessageTimeCodeQuarterFrame = 0xF1, ///< System Common - MIDI Time Code Quarter Frame
    midiMessageSongPosition         = 0xF2, ///< System Common - Song Position Pointer
    midiMessageSongSelect           = 0xF3, ///< System Common - Song Select
    midiMessageTuneRequest          = 0xF6, ///< System Common - Tune Request
    midiMessageClock                = 0xF8, ///< System Real Time - Timing Clock
    midiMessageStart                = 0xFA, ///< System Real Time - Start
    midiMessageContinue             = 0xFB, ///< System Real Time - Continue
    midiMessageStop                 = 0xFC, ///< System Real Time - Stop
    midiMessageActiveSensing        = 0xFE, ///< System Real Time - Active Sensing
    midiMessageSystemReset          = 0xFF, ///< System Real Time - System Reset
    midiMessageInvalidType          = 0x00  ///< For notifying errors
} midiMessageType_t;

///
/// \brief Enumeration holding USB-specific values for SysEx messages.
///
/// Normally, USB MIDI CIN (cable index number) is just midiMessageType_t shifted left by four bytes,
/// however, System Common/SysEx messages have different values so they're grouped in special enumeration.
///
typedef enum
{
    sysCommon1byteCin   = 0x50,
    sysCommon2byteCin   = 0x20,
    sysCommon3byteCin   = 0x30,
    sysExStartCin       = 0x40,
    sysExStop1byteCin   = sysCommon1byteCin,
    sysExStop2byteCin   = 0x60,
    sysExStop3byteCin   = 0x70
} usbMIDIsystemCin_t;

///
/// \brief Holds various types of MIDI Thru filtering.
///
typedef enum
{
    THRU_OFF,
    THRU_FULL_USB,
    THRU_FULL_DIN,
    THRU_FULL_ALL,
    THRU_CHANNEL_USB,
    THRU_CHANNEL_DIN,
    THRU_CHANNEL_ALL
} midiFilterMode_t;

///
/// \brief Holds various types of MIDI Note Off message
///
typedef enum
{
    noteOffType_noteOnZeroVel,
    noteOffType_standardNoteOff
} noteOffType_t;

///
/// \brief List off all possible MIDI notes.
///
typedef enum
{
    C,
    C_SHARP,
    D,
    D_SHARP,
    E,
    F,
    F_SHARP,
    G,
    G_SHARP,
    A,
    A_SHARP,
    B,
    MIDI_NOTES
} note_t;

///
/// \brief Holds decoded data of a MIDI message.
///
typedef struct
{
    uint8_t channel;                            ///< MIDI channel on which the message was received (1-16)
    midiMessageType_t type;                     ///< The type of the message
    uint8_t data1;                              ///< First data byte (0-127)
    uint8_t data2;                              ///< Second data byte (0-127, 0 if message length is 2 bytes)
    uint8_t sysexArray[MIDI_SYSEX_ARRAY_SIZE];  ///< SysEx array buffer
    bool valid;                                 ///< Message valid/invalid (validity means the message respects the MIDI norm)
} MIDImessage_t;

///
/// \brief USB MIDI event packet.
///
/// Used to encapsulate sent and received MIDI messages from a USB MIDI interface.
///
typedef struct
{
    uint8_t Event; ///< MIDI event type, constructed with the \ref GET_USB_MIDI_EVENT() macro.

    uint8_t Data1; ///< First byte of data in the MIDI event.
    uint8_t Data2; ///< Second byte of data in the MIDI event.
    uint8_t Data3; ///< Third byte of data in the MIDI event.
} USBMIDIpacket_t;

///
/// \brief Structure used to convert two 7-bit values to single 14-bit value and vice versa.
///
typedef struct
{
    uint8_t high;
    uint8_t low;
    uint16_t value;

    void split14bit()
    {
        uint8_t newHigh = (value >> 8) & 0xFF;
        uint8_t newLow = value & 0xFF;
        newHigh = (newHigh << 1) & 0x7F;

        if ((newLow >> 7) & 0x01)
        {
            newHigh |= 0x01;
        }
        else
        {
            newHigh &= ~0x01;
        }

        newLow = lowByte_7bit(newLow);
        high = newHigh;
        low = newLow;
    }

    void mergeTo14bit()
    {
        if (high & 0x01)
        {
            low |= (1 << 7);
        }
        else
        {
            low &= ~(1 << 7);
        }

        high >>= 1;

        uint16_t joined;
        joined = high;
        joined <<= 8;
        joined |= low;

        value = joined;
    }
} encDec_14bit_t;
