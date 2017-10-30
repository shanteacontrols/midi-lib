/*

Copyright 2016 Francois Best
Copyright 2017 Igor Petrović

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

#pragma once

#include "Helpers.h"

enum midiInterfaceType_t
{
    dinInterface,
    usbInterface
};

enum midiVelocity_t
{
    velocityOn = 127,
    velocityOff = 0
};

enum midiMessageType_t
{
    midiMessageNoteOff              = 0x80, //Note Off
    midiMessageNoteOn               = 0x90, //Note On
    midiMessageControlChange        = 0xB0, //Control Change / Channel Mode
    midiMessageProgramChange        = 0xC0, //Program Change
    midiMessageAfterTouchChannel    = 0xD0, //Channel (monophonic) AfterTouch
    midiMessageAfterTouchPoly       = 0xA0, //Polyphonic AfterTouch
    midiMessagePitchBend            = 0xE0, //Pitch Bend
    midiMessageSystemExclusive      = 0xF0, //System Exclusive
    midiMessageTimeCodeQuarterFrame = 0xF1, //System Common - MIDI Time Code Quarter Frame
    midiMessageSongPosition         = 0xF2, //System Common - Song Position Pointer
    midiMessageSongSelect           = 0xF3, //System Common - Song Select
    midiMessageTuneRequest          = 0xF6, //System Common - Tune Request
    midiMessageClock                = 0xF8, //System Real Time - Timing Clock
    midiMessageStart                = 0xFA, //System Real Time - Start
    midiMessageContinue             = 0xFB, //System Real Time - Continue
    midiMessageStop                 = 0xFC, //System Real Time - Stop
    midiMessageActiveSensing        = 0xFE, //System Real Time - Active Sensing
    midiMessageSystemReset          = 0xFF, //System Real Time - System Reset
    midiMessageInvalidType          = 0x00  //For notifying errors
};

enum usbMIDIsystemCin_t
{
    //normally, usb midi cin (cable index number) is just midiMessageType shifted left by four bytes
    //system common/exclusive messages have a bit convulted pattern so they're grouped in different enum
    sysCommon1byteCin = 0x50,
    sysCommon2byteCin = 0x20,
    sysCommon3byteCin = 0x30,
    sysExStartCin = 0x40,
    sysExStop1byteCin = sysCommon1byteCin,
    sysExStop2byteCin = 0x60,
    sysExStop3byteCin = 0x70
};

enum midiFilterMode_t
{
    Off,                //thru disabled (nothing passes through)
    Full,               //fully enabled Thru (every incoming message is sent back)
    SameChannel,        //only the messages on the Input Channel will be sent back
    DifferentChannel    //all the messages but the ones on the Input Channel will be sent back
};

typedef enum
{
    noteOffType_noteOnZeroVel,
    noteOffType_offChannel
} noteOffType_t;

//decoded data of a MIDI message
struct Message
{
    //MIDI channel on which the message was received (1-16)
    uint8_t channel;

    //the type of the message
    midiMessageType_t type;

    //first data byte (0-127)
    uint8_t data1;

    //second data byte (0-127, 0 if message length is 2 bytes)
    uint8_t data2;

    //sysex array length is stocked on 16 bits, in data1 (LSB) and data2 (MSB)
    uint8_t sysexArray[MIDI_SYSEX_ARRAY_SIZE];

    //message valid/invalid (no channel consideration here, validity means the message respects the MIDI norm)
    bool valid;
};

typedef struct
{
    uint8_t high;
    uint8_t low;
    uint16_t value;

    void encodeTo14bit()
    {
        uint8_t newHigh = (value >> 8) & 0xFF;
        uint8_t newLow = value & 0xFF;
        newHigh = (newHigh << 1) & 0x7F;
        bitWrite(newHigh, 0, bitRead(newLow, 7));
        newLow = lowByte_7bit(newLow);
        high = newHigh;
        low = newLow;
    }

    uint16_t decode14bit()
    {
        bitWrite(low, 7, bitRead(high, 0));
        high >>= 1;

        uint16_t joined;
        joined = high;
        joined <<= 8;
        joined |= low;

        return joined;
    }
} encDec_14bit;