/*
    Copyright 2016 Francois Best
    Copyright 2017 Igor Petroviæ

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

#ifdef USBMIDI

#include "Descriptors.h"
#include "DataTypes.h"
#include "Helpers.h"

#define MIDI_CHANNEL_OMNI       0
#define MIDI_CHANNEL_OFF        17 // and over

#define MIDI_PITCHBEND_MIN      -8192
#define MIDI_PITCHBEND_MAX      8191

//usb
void EVENT_USB_Device_ConfigurationChanged(void);

///
/// \brief UART and USB MIDI communication.
/// \ingroup midi
/// @{
///
class MIDI
{
    public:
    MIDI();
    bool init(midiInterfaceType_t type);
    void handleUARTread(int16_t(*fptr)());
    void handleUARTwrite(int8_t(*fptr)(uint8_t data));

    //MIDI output
    public:
    void sendNoteOn(uint8_t inNoteNumber, uint8_t inVelocity, uint8_t inChannel = 0);
    void sendNoteOff(uint8_t inNoteNumber, uint8_t inVelocity, uint8_t inChannel = 0);
    void sendProgramChange(uint8_t inProgramNumber, uint8_t inChannel = 0);
    void sendControlChange(uint8_t inControlNumber, uint8_t inControlValue, uint8_t inChannel = 0);
    void sendPitchBend(int16_t inPitchValue, uint8_t inChannel);
    void sendPolyPressure(uint8_t inNoteNumber, uint8_t inPressure, uint8_t inChannel = 0);
    void sendAfterTouch(uint8_t inPressure, uint8_t inChannel = 0);
    void sendSysEx(uint16_t inLength, const uint8_t* inArray, bool inArrayContainsBoundaries);
    void sendTimeCodeQuarterFrame(uint8_t inTypeNibble, uint8_t inValuesNibble);
    void sendTimeCodeQuarterFrame(uint8_t inData);
    void sendSongPosition(uint16_t inBeats);
    void sendSongSelect(uint8_t inSongNumber);
    void sendTuneRequest();
    void sendRealTime(midiMessageType_t inType);
    void setNoteOffMode(noteOffType_t type);
    void enableRunningStatus();
    void disableRunningStatus();
    bool runningStatusEnabled();

    void enableUSB();
    void enableDIN();
    void disableUSB();
    void disableDIN();

    void setNoteChannel(uint8_t channel);
    void setCCchannel(uint8_t channel);
    void setProgramChangeChannel(uint8_t channel);

    noteOffType_t getNoteOffMode();

    private:
    void send(midiMessageType_t inType, uint8_t inData1, uint8_t inData2, uint8_t inChannel);

    //MIDI input

    public:
    bool read(midiInterfaceType_t type);
    midiMessageType_t getType(midiInterfaceType_t type) const;
    uint8_t  getChannel(midiInterfaceType_t type) const;
    uint8_t getData1(midiInterfaceType_t type) const;
    uint8_t getData2(midiInterfaceType_t type) const;
    uint8_t* getSysExArray(midiInterfaceType_t type);
    uint16_t getSysExArrayLength(midiInterfaceType_t type);
    bool check() const;
    uint8_t getInputChannel() const;
    void setInputChannel(uint8_t inChannel);
    midiMessageType_t getTypeFromStatusByte(uint8_t inStatus);
    uint8_t getChannelFromStatusByte(uint8_t inStatus);
    bool isChannelMessage(midiMessageType_t inType);

    private:
    bool read(uint8_t inChannel, midiInterfaceType_t type);
    bool addSysExArrayByte(midiInterfaceType_t type);

    public:
    midiFilterMode_t getFilterMode() const;
    bool getThruState() const;

    void turnThruOn(midiFilterMode_t inThruFilterMode = Full);
    void turnThruOff();
    void setThruFilterMode(midiFilterMode_t inThruFilterMode);

    private:
    void thruFilter(uint8_t inChannel);
    bool parse(midiInterfaceType_t type);
    bool inputFilter(uint8_t inChannel, midiInterfaceType_t type);
    void resetInput();
    uint8_t getStatus(midiMessageType_t inType, uint8_t inChannel) const;

    int16_t             (*sendUARTreadCallback)();
    int8_t              (*sendUARTwriteCallback)(uint8_t data);

    bool                usbEnabled,
                        dinEnabled;

    bool                mThruActivated;
    midiFilterMode_t    mThruFilterMode;
    bool                useRunningStatus;
    bool                use1byteParsing;

    uint8_t             mRunningStatus_RX;
    uint8_t             mRunningStatus_TX;
    uint8_t             mInputChannel;
    uint8_t             mPendingMessage[3];
    uint16_t            dinPendingMessageExpectedLenght;
    uint16_t            dinPendingMessageIndex;
    uint16_t            sysExArrayLength;
    Message             dinMessage,
                        usbMessage;
    noteOffType_t       noteOffMode;
    uint8_t             noteChannel_,
                        ccChannel_,
                        programChangeChannel_,
                        aftertouchChannel_;
};

extern MIDI midi;
extern volatile bool MIDIevent_in;
extern volatile bool MIDIevent_out;

#endif
/// @}