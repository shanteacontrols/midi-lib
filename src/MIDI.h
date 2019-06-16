/*
    Copyright 2016 Francois Best
    Copyright 2017-2019 Igor Petrovic

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
#include "Helpers.h"

class MIDI
{
    public:
    ///
    /// \brief Enumeration holding various types of MIDI messages.
    ///
    enum class messageType_t : uint8_t
    {
        noteOff = 0x80,                          ///< Note Off
        noteOn = 0x90,                           ///< Note On
        controlChange = 0xB0,                    ///< Control Change / Channel Mode
        programChange = 0xC0,                    ///< Program Change
        afterTouchChannel = 0xD0,                ///< Channel (monophonic) AfterTouch
        afterTouchPoly = 0xA0,                   ///< Polyphonic AfterTouch
        pitchBend = 0xE0,                        ///< Pitch Bend
        systemExclusive = 0xF0,                  ///< System Exclusive
        sysCommonTimeCodeQuarterFrame = 0xF1,    ///< System Common - MIDI Time Code Quarter Frame
        sysCommonSongPosition = 0xF2,            ///< System Common - Song Position Pointer
        sysCommonSongSelect = 0xF3,              ///< System Common - Song Select
        sysCommonTuneRequest = 0xF6,             ///< System Common - Tune Request
        sysRealTimeClock = 0xF8,                 ///< System Real Time - Timing Clock
        sysRealTimeStart = 0xFA,                 ///< System Real Time - Start
        sysRealTimeContinue = 0xFB,              ///< System Real Time - Continue
        sysRealTimeStop = 0xFC,                  ///< System Real Time - Stop
        sysRealTimeActiveSensing = 0xFE,         ///< System Real Time - Active Sensing
        sysRealTimeSystemReset = 0xFF,           ///< System Real Time - System Reset
        invalid = 0x00                           ///< For notifying errors
    };

    ///
    /// \brief Enumeration holding two different types of MIDI interfaces.
    ///
    enum class interface_t : uint8_t
    {
        din,
        usb
    };

    ///
    /// \brief Holds various types of MIDI Thru filtering.
    ///
    enum class filterMode_t : uint8_t
    {
        off,
        fullUSB,
        fullDIN,
        fullAll,
        channelUSB,
        channelDIN,
        channelAll
    };

    ///
    /// \brief Holds various types of MIDI Note Off message
    ///
    enum class noteOffType_t : uint8_t
    {
        noteOnZeroVel,
        standardNoteOff
    };

    ///
    /// \brief List off all possible MIDI notes.
    ///
    enum class note_t : uint8_t
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
        AMOUNT
    };

    ///
    /// \brief USB MIDI event packet.
    ///
    /// Used to encapsulate sent and received MIDI messages from a USB MIDI interface.
    ///
    typedef struct
    {
        uint8_t Event;    ///< MIDI event type, constructed with the \ref GET_USB_MIDI_EVENT() macro.
        uint8_t Data1;    ///< First byte of data in the MIDI event.
        uint8_t Data2;    ///< Second byte of data in the MIDI event.
        uint8_t Data3;    ///< Third byte of data in the MIDI event.
    } USBMIDIpacket_t;

    ///
    /// \brief Enumeration holding USB-specific values for SysEx/System Common messages.
    ///
    /// Normally, USB MIDI CIN (cable index number) is just messageType_t shifted left by four bytes,
    /// however, SysEx/System Common messages have different values so they're grouped in special enumeration.
    ///
    enum class usbMIDIsystemCin_t : uint8_t
    {
        sysCommon1byteCin = 0x50,
        sysCommon2byteCin = 0x20,
        sysCommon3byteCin = 0x30,
        singleByte = 0xF0,
        sysExStartCin = 0x40,
        sysExStop1byteCin = sysCommon1byteCin,
        sysExStop2byteCin = 0x60,
        sysExStop3byteCin = 0x70
    };

    ///
    /// \brief Structure used to convert two 7-bit values to single 14-bit value and vice versa.
    ///
    typedef struct
    {
        uint8_t  high;
        uint8_t  low;
        uint16_t value;

        void split14bit()
        {
            uint8_t newHigh = (value >> 8) & 0xFF;
            uint8_t newLow = value & 0xFF;
            newHigh = (newHigh << 1) & 0x7F;

            if ((newLow >> 7) & 0x01)
                newHigh |= 0x01;
            else
                newHigh &= ~0x01;

            newLow &= 0x7F;
            high = newHigh;
            low = newLow;
        }

        void mergeTo14bit()
        {
            if (high & 0x01)
                low |= (1 << 7);
            else
                low &= ~(1 << 7);

            high >>= 1;

            uint16_t joined;
            joined = high;
            joined <<= 8;
            joined |= low;

            value = joined;
        }
    } encDec_14bit_t;

    public:
    MIDI() {}
    void           handleUARTread(bool (*fptr)(uint8_t& data));
    void           handleUARTwrite(bool (*fptr)(uint8_t data));
    void           handleUSBread(bool (*fptr)(USBMIDIpacket_t& USBMIDIpacket));
    void           handleUSBwrite(bool (*fptr)(USBMIDIpacket_t& USBMIDIpacket));
    void           setChannelSendZeroStart(bool state);
    void           sendNoteOn(uint8_t inNoteNumber, uint8_t inVelocity, uint8_t inChannel);
    void           sendNoteOff(uint8_t inNoteNumber, uint8_t inVelocity, uint8_t inChannel);
    void           sendProgramChange(uint8_t inProgramNumber, uint8_t inChannel);
    void           sendControlChange(uint8_t inControlNumber, uint8_t inControlValue, uint8_t inChannel);
    void           sendPitchBend(uint16_t inPitchValue, uint8_t inChannel);
    void           sendAfterTouch(uint8_t inPressure, uint8_t inChannel, uint8_t inNoteNumber);
    void           sendAfterTouch(uint8_t inPressure, uint8_t inChannel);
    void           sendSysEx(uint16_t inLength, const uint8_t* inArray, bool inArrayContainsBoundaries);
    void           sendTimeCodeQuarterFrame(uint8_t inTypeNibble, uint8_t inValuesNibble);
    void           sendTimeCodeQuarterFrame(uint8_t inData);
    void           sendSongPosition(uint16_t inBeats);
    void           sendSongSelect(uint8_t inSongNumber);
    void           sendTuneRequest();
    void           sendRealTime(messageType_t inType);
    void           setNoteOffMode(noteOffType_t type);
    void           setRunningStatusState(bool state);
    bool           getRunningStatusState();
    noteOffType_t  getNoteOffMode();
    static uint8_t getOctaveFromNote(int8_t note);
    static note_t  getTonicFromNote(int8_t note);
    void           send(messageType_t inType, uint8_t inData1, uint8_t inData2, uint8_t inChannel);
    bool           read(interface_t type, filterMode_t filterMode = filterMode_t::off);
    bool           parse(interface_t type);
    messageType_t  getType(interface_t type);
    uint8_t        getChannel(interface_t type);
    uint8_t        getData1(interface_t type);
    uint8_t        getData2(interface_t type);
    uint8_t*       getSysExArray(interface_t type);
    uint16_t       getSysExArrayLength(interface_t type);
    uint8_t        getInputChannel();
    bool           setInputChannel(uint8_t inChannel);
    messageType_t  getTypeFromStatusByte(uint8_t inStatus);
    uint8_t        getChannelFromStatusByte(uint8_t inStatus);
    bool           isChannelMessage(messageType_t inType);
    void           useRecursiveParsing(bool state);
    bool           getRecursiveParseState();

    ///
    /// \brief Holds decoded data of a MIDI message.
    ///
    typedef struct
    {
        uint8_t       channel;                              ///< MIDI channel on which the message was received (1-16)
        messageType_t type;                                 ///< The type of the message
        uint8_t       data1;                                ///< First data byte (0-127)
        uint8_t       data2;                                ///< Second data byte (0-127, 0 if message length is 2 bytes)
        uint8_t       sysexArray[MIDI_SYSEX_ARRAY_SIZE];    ///< SysEx array buffer
        bool          valid;                                ///< Message valid/invalid (validity means the message respects the MIDI norm)
    } message_t;

    ///
    /// \brief Decoded MIDI messages for USB and DIN interfaces.
    ///
    message_t dinMessage,
        usbMessage;

    private:
    void    thruFilter(uint8_t inChannel, interface_t type, filterMode_t filterMode);
    bool    inputFilter(uint8_t inChannel, interface_t type);
    void    resetInput();
    uint8_t getStatus(messageType_t inType, uint8_t inChannel);

    bool useRunningStatus = false;
    bool recursiveParseState = false;

    bool zeroStartChannel = false;

    uint8_t mRunningStatus_RX;
    uint8_t mRunningStatus_TX;

    uint8_t  mInputChannel;
    uint8_t  mPendingMessage[3];
    uint16_t dinPendingMessageExpectedLenght;
    uint16_t dinPendingMessageIndex;
    uint16_t sysExArrayLength = 0;

    noteOffType_t noteOffMode = noteOffType_t::noteOnZeroVel;

    bool (*sendUARTreadCallback)(uint8_t& data) = nullptr;
    bool (*sendUARTwriteCallback)(uint8_t data) = nullptr;
    bool (*sendUSBreadCallback)(USBMIDIpacket_t& USBMIDIpacket) = nullptr;
    bool (*sendUSBwriteCallback)(USBMIDIpacket_t& USBMIDIpacket) = nullptr;

    USBMIDIpacket_t usbMIDIpacket;
};