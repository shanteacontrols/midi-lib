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
/// \brief MIDI library main class.
/// @{
///

class MIDI
{
    public:
    MIDI() {}
    void handleUARTread(bool(*fptr)(uint8_t &data));
    void handleUARTwrite(bool(*fptr)(uint8_t data));
    void handleUSBread(bool(*fptr)(USBMIDIpacket_t& USBMIDIpacket));
    void handleUSBwrite(bool(*fptr)(USBMIDIpacket_t& USBMIDIpacket));
    void setChannelSendZeroStart(bool state);
    void sendNoteOn(uint8_t inNoteNumber, uint8_t inVelocity, uint8_t inChannel);
    void sendNoteOff(uint8_t inNoteNumber, uint8_t inVelocity, uint8_t inChannel);
    void sendProgramChange(uint8_t inProgramNumber, uint8_t inChannel);
    void sendControlChange(uint8_t inControlNumber, uint8_t inControlValue, uint8_t inChannel);
    void sendPitchBend(uint16_t inPitchValue, uint8_t inChannel);
    void sendAfterTouch(uint8_t inPressure, uint8_t inChannel, uint8_t inNoteNumber);
    void sendAfterTouch(uint8_t inPressure, uint8_t inChannel);
    void sendSysEx(uint16_t inLength, const uint8_t* inArray, bool inArrayContainsBoundaries);
    void sendTimeCodeQuarterFrame(uint8_t inTypeNibble, uint8_t inValuesNibble);
    void sendTimeCodeQuarterFrame(uint8_t inData);
    void sendSongPosition(uint16_t inBeats);
    void sendSongSelect(uint8_t inSongNumber);
    void sendTuneRequest();
    void sendRealTime(midiMessageType_t inType);
    void setNoteOffMode(noteOffType_t type);
    void setRunningStatusState(bool state);
    bool getRunningStatusState();
    noteOffType_t getNoteOffMode();
    static uint8_t getOctaveFromNote(int8_t note);
    static note_t getTonicFromNote(int8_t note);
    void send(midiMessageType_t inType, uint8_t inData1, uint8_t inData2, uint8_t inChannel);
    bool read(midiInterfaceType_t type, midiFilterMode_t filterMode = THRU_OFF);
    bool parse(midiInterfaceType_t type);
    midiMessageType_t getType(midiInterfaceType_t type);
    uint8_t getChannel(midiInterfaceType_t type);
    uint8_t getData1(midiInterfaceType_t type);
    uint8_t getData2(midiInterfaceType_t type);
    uint8_t* getSysExArray(midiInterfaceType_t type);
    uint16_t getSysExArrayLength(midiInterfaceType_t type);
    uint8_t getInputChannel();
    bool setInputChannel(uint8_t inChannel);
    midiMessageType_t getTypeFromStatusByte(uint8_t inStatus);
    uint8_t getChannelFromStatusByte(uint8_t inStatus);
    bool isChannelMessage(midiMessageType_t inType);
    void useRecursiveParsing(bool state);
    bool getRecursiveParseState();

    ///
    /// \brief Decoded MIDI messages for USB and DIN interfaces.
    ///
    MIDImessage_t       dinMessage,
                        usbMessage;

    private:
    void thruFilter(uint8_t inChannel, midiInterfaceType_t type, midiFilterMode_t filterMode);
    bool inputFilter(uint8_t inChannel, midiInterfaceType_t type);
    void resetInput();
    uint8_t getStatus(midiMessageType_t inType, uint8_t inChannel);

    bool                useRunningStatus = false;
    bool                recursiveParseState = false;

    bool                zeroStartChannel = false;

    uint8_t             mRunningStatus_RX,
                        mRunningStatus_TX;

    uint8_t             mInputChannel;
    uint8_t             mPendingMessage[3];
    uint16_t            dinPendingMessageExpectedLenght;
    uint16_t            dinPendingMessageIndex;
    uint16_t            sysExArrayLength = 0;

    noteOffType_t       noteOffMode = noteOffType_noteOnZeroVel;

    bool                (*sendUARTreadCallback)(uint8_t &data) = nullptr;
    bool                (*sendUARTwriteCallback)(uint8_t data) = nullptr;
    bool                (*sendUSBreadCallback)(USBMIDIpacket_t& USBMIDIpacket) = nullptr;
    bool                (*sendUSBwriteCallback)(USBMIDIpacket_t& USBMIDIpacket) = nullptr;

    USBMIDIpacket_t     usbMIDIpacket;
};

/// @}