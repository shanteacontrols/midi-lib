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

#include "Constants.h"
#include "DataTypes.h"
#include "Helpers.h"

///
/// \brief Decoded MIDI messages for USB and DIN interfaces.
///
extern MIDImessage_t            dinMessage,
                                usbMessage;

///
/// \brief MIDI library main class.
/// @{
///

class MIDI
{
    public:
    MIDI();

    static void handleUARTread(int16_t(*fptr)());
    static void handleUARTwrite(int8_t(*fptr)(uint8_t data));

    static void handleUSBread(bool(*fptr)(USBMIDIpacket_t& USBMIDIpacket));
    static void handleUSBwrite(bool(*fptr)(USBMIDIpacket_t& USBMIDIpacket));

    static void setChannelSendZeroStart(bool state);

    //MIDI output
    public:
    static void sendNoteOn(uint8_t inNoteNumber, uint8_t inVelocity, uint8_t inChannel);
    static void sendNoteOff(uint8_t inNoteNumber, uint8_t inVelocity, uint8_t inChannel);
    static void sendProgramChange(uint8_t inProgramNumber, uint8_t inChannel);
    static void sendControlChange(uint8_t inControlNumber, uint8_t inControlValue, uint8_t inChannel);
    static void sendPitchBend(uint16_t inPitchValue, uint8_t inChannel);
    static void sendAfterTouch(uint8_t inPressure, uint8_t inChannel, uint8_t inNoteNumber);
    static void sendAfterTouch(uint8_t inPressure, uint8_t inChannel);
    static void sendSysEx(uint16_t inLength, const uint8_t* inArray, bool inArrayContainsBoundaries);
    static void sendTimeCodeQuarterFrame(uint8_t inTypeNibble, uint8_t inValuesNibble);
    static void sendTimeCodeQuarterFrame(uint8_t inData);
    static void sendSongPosition(uint16_t inBeats);
    static void sendSongSelect(uint8_t inSongNumber);
    static void sendTuneRequest();
    static void sendRealTime(midiMessageType_t inType);
    static void setNoteOffMode(noteOffType_t type);

    static void setRunningStatusState(bool state);
    static bool getRunningStatusState();

    static noteOffType_t getNoteOffMode();

    static uint8_t getOctaveFromNote(int8_t note);
    static note_t getTonicFromNote(int8_t note);

    private:
    static void send(midiMessageType_t inType, uint8_t inData1, uint8_t inData2, uint8_t inChannel);

    //MIDI input

    public:
    static bool read(midiInterfaceType_t type, midiFilterMode_t filterMode = THRU_OFF);
    static midiMessageType_t getType(midiInterfaceType_t type);
    static uint8_t getChannel(midiInterfaceType_t type);
    static uint8_t getData1(midiInterfaceType_t type);
    static uint8_t getData2(midiInterfaceType_t type);
    static uint8_t* getSysExArray(midiInterfaceType_t type);
    static uint16_t getSysExArrayLength(midiInterfaceType_t type);
    static uint8_t getInputChannel();
    static bool setInputChannel(uint8_t inChannel);
    static midiMessageType_t getTypeFromStatusByte(uint8_t inStatus);
    static uint8_t getChannelFromStatusByte(uint8_t inStatus);
    static bool isChannelMessage(midiMessageType_t inType);
    static void setOneByteParseDINstate(bool state);
    static bool getOneByteParseDINstate();
    static void setDINvalidityCheckState(bool state);

    private:
    static void thruFilter(uint8_t inChannel, midiInterfaceType_t type, midiFilterMode_t filterMode);
    static bool parse(midiInterfaceType_t type);
    static bool inputFilter(uint8_t inChannel, midiInterfaceType_t type);
    static void resetInput();
    static uint8_t getStatus(midiMessageType_t inType, uint8_t inChannel);
};

/// @}