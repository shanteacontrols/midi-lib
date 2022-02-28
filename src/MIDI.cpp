/*
    Copyright 2016 Francois Best
    Copyright 2017-2020 Igor Petrovic

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

#include "MIDI.h"

bool MIDIlib::init(interface_t interface)
{
    if (hwa.init(interface))
    {
        switch (interface)
        {
        case interface_t::din:
        {
            _dinEnabled = true;
        }
        break;

        case interface_t::usb:
        {
            _usbEnabled = true;
        }
        break;

        default:
        {
            _dinEnabled = true;
            _usbEnabled = true;
        }
        break;
        }

        return true;
    }

    return false;
}

bool MIDIlib::deInit(interface_t interface)
{
    switch (interface)
    {
    case interface_t::din:
    {
        _dinEnabled = false;
    }
    break;

    case interface_t::usb:
    {
        _usbEnabled = false;
    }
    break;

    default:
    {
        _dinEnabled = false;
        _usbEnabled = false;
    }
    break;
    }

    return hwa.deInit(interface);
}

///
/// \brief Generate and send a MIDI message from the values given.
///
/// This is an internal method, use it only if you need to send raw data.
/// \param [in] inType      The message type (see type defines for reference)
/// \param [in] inData1     The first data byte.
/// \param [in] inData2     The second data byte (if the message contains only 1 data byte, set this one to 0).
/// \param inChannel        The output channel on which the message will be sent (values from 1 to 16). Note: you cannot send to OMNI.
///
void MIDIlib::send(messageType_t inType, uint8_t inData1, uint8_t inData2, uint8_t inChannel)
{
    bool channelValid = true;

    // test if channel is valid
    if (_zeroStartChannel)
    {
        if (inChannel >= 16)
        {
            channelValid = false;
        }
        else
        {
            inChannel++;    // normalize channel
        }
    }
    else
    {
        if ((inChannel > 16) || !inChannel)
        {
            channelValid = false;
        }
    }

    if (!channelValid || (inType < messageType_t::noteOff))
    {
        if (_useRunningStatus)
        {
            _mRunningStatusTX = static_cast<uint8_t>(messageType_t::invalid);
        }

        return;    // don't send anything
    }

    if (inType <= messageType_t::pitchBend)
    {
        // channel messages
        // protection: remove MSBs on data
        inData1 &= 0x7f;
        inData2 &= 0x7f;

        const uint8_t inStatus = status(inType, inChannel);

        if (_useRunningStatus)
        {
            if (_mRunningStatusTX != inStatus)
            {
                // new message, memorize and send header
                _mRunningStatusTX = inStatus;
                dinWrite(_mRunningStatusTX);
            }
        }
        else
        {
            // don't care about running status, send the status byte
            dinWrite(inStatus);
        }

        // send data
        dinWrite(inData1);

        if ((inType != messageType_t::programChange) && (inType != messageType_t::afterTouchChannel))
        {
            dinWrite(inData2);
        }

        uint8_t event = static_cast<uint8_t>(inType) >> static_cast<uint8_t>(4);

        usbMIDIPacket_t usbPacket;

        usbPacket[USB_EVENT] = event;
        usbPacket[USB_DATA1] = inStatus;
        usbPacket[USB_DATA2] = inData1;
        usbPacket[USB_DATA3] = inData2;

        usbWrite(usbPacket);
    }
    else if (inType >= messageType_t::sysCommonTuneRequest && inType <= messageType_t::sysRealTimeSystemReset)
    {
        sendRealTime(inType);    // system real-time and 1 byte
    }
}

///
/// \brief Send a Note On message.
///
/// \param inNoteNumber [in]    Pitch value in the MIDI format (0 to 127).
/// \param inVelocity [in]      Note attack velocity (0 to 127).
/// \param inChannel [in]       The channel on which the message will be sent (1 to 16).
///
void MIDIlib::sendNoteOn(uint8_t inNoteNumber, uint8_t inVelocity, uint8_t inChannel)
{
    send(messageType_t::noteOn, inNoteNumber, inVelocity, inChannel);
}

///
/// \brief Send a Note Off message.
///
/// If note off mode is set to noteOffType_t::standardNoteOff, Note Off message will be sent.
/// If mode is set to noteOffType_noteOnZeroVel, Note On will be sent will velocity 0.
/// \param inNoteNumber [in]    Pitch value in the MIDI format (0 to 127).
/// \param inVelocity [in]      Release velocity (0 to 127).
/// \param inChannel [in]       The channel on which the message will be sent (1 to 16).
///
void MIDIlib::sendNoteOff(uint8_t inNoteNumber, uint8_t inVelocity, uint8_t inChannel)
{
    if (_noteOffMode == noteOffType_t::standardNoteOff)
    {
        send(messageType_t::noteOff, inNoteNumber, inVelocity, inChannel);
    }
    else
    {
        send(messageType_t::noteOn, inNoteNumber, inVelocity, inChannel);
    }
}

///
/// \brief Send a Program Change message.
/// \param inProgramNumber [in]     The program to select (0 to 127).
/// \param inChannel [in]           The channel on which the message will be sent (1 to 16).
///
void MIDIlib::sendProgramChange(uint8_t inProgramNumber, uint8_t inChannel)
{
    send(messageType_t::programChange, inProgramNumber, 0, inChannel);
}

///
/// \brief Send a Control Change message.
/// \param inControlNumber [in]     The controller number (0 to 127).
/// \param inControlValue [in]      The value for the specified controller (0 to 127).
/// \param inChannel [in]           The channel on which the message will be sent (1 to 16).
///
void MIDIlib::sendControlChange(uint8_t inControlNumber, uint8_t inControlValue, uint8_t inChannel)
{
    send(messageType_t::controlChange, inControlNumber, inControlValue, inChannel);
}

///
/// \brief Send a Polyphonic AfterTouch message (applies to a specified note).
/// \param inPressure [in]    The amount of AfterTouch to apply (0 to 127).
/// \param inChannel [in]     The channel on which the message will be sent (1 to 16).
/// \param inNoteNumber [in]  The note to apply AfterTouch to (0 to 127).
///
void MIDIlib::sendAfterTouch(uint8_t inPressure, uint8_t inChannel, uint8_t inNoteNumber)
{
    send(messageType_t::afterTouchPoly, inNoteNumber, inPressure, inChannel);
}

///
/// \brief Send a Monophonic AfterTouch message (applies to all notes).
/// \param inPressure [in]    The amount of AfterTouch to apply (0 to 127).
/// \param inChannel [in]     The channel on which the message will be sent (1 to 16).
///
void MIDIlib::sendAfterTouch(uint8_t inPressure, uint8_t inChannel)
{
    send(messageType_t::afterTouchChannel, inPressure, 0, inChannel);
}

///
/// \brief Send a Pitch Bend message using a signed integer value.
/// \param inPitchValue [in]  The amount of bend to send (0-16383).
/// \param inChannel [in]     The channel on which the message will be sent (1 to 16).
///
void MIDIlib::sendPitchBend(uint16_t inPitchValue, uint8_t inChannel)
{
    auto split = Split14bit(inPitchValue & 0x3FFF);

    send(messageType_t::pitchBend, split.low(), split.high(), inChannel);
}

///
/// \brief Send a System Exclusive message.
/// \param inLength [in]                    The size of the array to send.
/// \param inArray [in]                     The byte array containing the data to send
/// \param inArrayContainsBoundaries [in]   When set to 'true', 0xF0 & 0xF7 bytes (start & stop SysEx)
///                                         will not be sent and therefore must be included in the array.
/// \param interface_t [in]                 Specifies on which interface given SysEx message will be sent.
///                                         If left undefined, message will be sent on both USB and DIN interface.
///
void MIDIlib::sendSysEx(uint16_t inLength, const uint8_t* inArray, bool inArrayContainsBoundaries, interface_t interface)
{
    if (interface == interface_t::all || interface == interface_t::din)
    {
        if (!inArrayContainsBoundaries)
        {
            dinWrite(0xf0);
        }

        for (uint16_t i = 0; i < inLength; ++i)
        {
            dinWrite(inArray[i]);
        }

        if (!inArrayContainsBoundaries)
        {
            dinWrite(0xf7);
        }

        if (_useRunningStatus)
        {
            _mRunningStatusTX = static_cast<uint8_t>(messageType_t::invalid);
        }
    }

    if (interface == interface_t::all || interface == interface_t::usb)
    {
        usbMIDIPacket_t usbPacket;

        if (!inArrayContainsBoundaries)
        {
            // append sysex start (0xF0) and stop (0xF7) bytes to array
            bool firstByte = true;
            bool startSent = false;

            while (inLength > 3)
            {
                if (firstByte)
                {
                    usbPacket[USB_EVENT] = usbMIDIEvent(0, static_cast<uint8_t>(usbMIDIsystemCin_t::sysExStartCin));
                    usbPacket[USB_DATA1] = static_cast<uint8_t>(messageType_t::systemExclusive);
                    usbPacket[USB_DATA2] = inArray[0];
                    usbPacket[USB_DATA3] = inArray[1];

                    usbWrite(usbPacket);

                    firstByte = false;
                    startSent = true;
                    inArray += 2;
                    inLength -= 2;
                }
                else
                {
                    usbPacket[USB_EVENT] = usbMIDIEvent(0, static_cast<uint8_t>(usbMIDIsystemCin_t::sysExStartCin));
                    usbPacket[USB_DATA1] = inArray[0];
                    usbPacket[USB_DATA2] = inArray[1];
                    usbPacket[USB_DATA3] = inArray[2];

                    usbWrite(usbPacket);

                    inArray += 3;
                    inLength -= 3;
                }
            }

            if (inLength == 3)
            {
                if (startSent)
                {
                    usbPacket[USB_EVENT] = usbMIDIEvent(0, static_cast<uint8_t>(usbMIDIsystemCin_t::sysExStartCin));
                    usbPacket[USB_DATA1] = inArray[0];
                    usbPacket[USB_DATA2] = inArray[1];
                    usbPacket[USB_DATA3] = inArray[2];

                    usbWrite(usbPacket);

                    usbPacket[USB_EVENT] = usbMIDIEvent(0, static_cast<uint8_t>(usbMIDIsystemCin_t::sysExStop1byteCin));
                    usbPacket[USB_DATA1] = 0xF7;
                    usbPacket[USB_DATA2] = 0;
                    usbPacket[USB_DATA3] = 0;

                    usbWrite(usbPacket);
                }
                else
                {
                    usbPacket[USB_EVENT] = usbMIDIEvent(0, static_cast<uint8_t>(usbMIDIsystemCin_t::sysExStartCin));
                    usbPacket[USB_DATA1] = static_cast<uint8_t>(messageType_t::systemExclusive);
                    usbPacket[USB_DATA2] = inArray[0];
                    usbPacket[USB_DATA3] = inArray[1];

                    usbWrite(usbPacket);

                    usbPacket[USB_EVENT] = usbMIDIEvent(0, static_cast<uint8_t>(usbMIDIsystemCin_t::sysExStop2byteCin));
                    usbPacket[USB_DATA1] = inArray[2];
                    usbPacket[USB_DATA2] = 0xF7;
                    usbPacket[USB_DATA3] = 0;

                    usbWrite(usbPacket);
                }
            }
            else if (inLength == 2)
            {
                if (startSent)
                {
                    usbPacket[USB_EVENT] = usbMIDIEvent(0, static_cast<uint8_t>(usbMIDIsystemCin_t::sysExStop3byteCin));
                    usbPacket[USB_DATA1] = inArray[0];
                    usbPacket[USB_DATA2] = inArray[1];
                    usbPacket[USB_DATA3] = 0xF7;

                    usbWrite(usbPacket);
                }
                else
                {
                    usbPacket[USB_EVENT] = usbMIDIEvent(0, static_cast<uint8_t>(usbMIDIsystemCin_t::sysExStartCin));
                    usbPacket[USB_DATA1] = static_cast<uint8_t>(messageType_t::systemExclusive);
                    usbPacket[USB_DATA2] = inArray[0];
                    usbPacket[USB_DATA3] = inArray[1];

                    usbWrite(usbPacket);

                    usbPacket[USB_EVENT] = usbMIDIEvent(0, static_cast<uint8_t>(usbMIDIsystemCin_t::sysExStop1byteCin));
                    usbPacket[USB_DATA1] = 0xF7;
                    usbPacket[USB_DATA2] = 0;
                    usbPacket[USB_DATA3] = 0;

                    usbWrite(usbPacket);
                }
            }
            else if (inLength == 1)
            {
                if (startSent)
                {
                    usbPacket[USB_EVENT] = usbMIDIEvent(0, static_cast<uint8_t>(usbMIDIsystemCin_t::sysExStop2byteCin));
                    usbPacket[USB_DATA1] = inArray[0];
                    usbPacket[USB_DATA2] = 0xF7;
                    usbPacket[USB_DATA3] = 0;

                    usbWrite(usbPacket);
                }
                else
                {
                    usbPacket[USB_EVENT] = usbMIDIEvent(0, static_cast<uint8_t>(usbMIDIsystemCin_t::sysExStop3byteCin));
                    usbPacket[USB_DATA1] = 0xF0;
                    usbPacket[USB_DATA2] = inArray[0];
                    usbPacket[USB_DATA3] = 0xF7;

                    usbWrite(usbPacket);
                }
            }
        }
        else
        {
            while (inLength > 3)
            {
                usbPacket[USB_EVENT] = usbMIDIEvent(0, static_cast<uint8_t>(usbMIDIsystemCin_t::sysExStartCin));
                usbPacket[USB_DATA1] = inArray[0];
                usbPacket[USB_DATA2] = inArray[1];
                usbPacket[USB_DATA3] = inArray[2];

                usbWrite(usbPacket);

                inArray += 3;
                inLength -= 3;
            }

            if (inLength == 3)
            {
                usbPacket[USB_EVENT] = usbMIDIEvent(0, static_cast<uint8_t>(usbMIDIsystemCin_t::sysExStop3byteCin));
                usbPacket[USB_DATA1] = inArray[0];
                usbPacket[USB_DATA2] = inArray[1];
                usbPacket[USB_DATA3] = inArray[2];

                usbWrite(usbPacket);
            }
            else if (inLength == 2)
            {
                usbPacket[USB_EVENT] = usbMIDIEvent(0, static_cast<uint8_t>(usbMIDIsystemCin_t::sysExStop2byteCin));
                usbPacket[USB_DATA1] = inArray[0];
                usbPacket[USB_DATA2] = inArray[1];
                usbPacket[USB_DATA3] = 0;

                usbWrite(usbPacket);
            }
            else if (inLength == 1)
            {
                usbPacket[USB_EVENT] = usbMIDIEvent(0, static_cast<uint8_t>(usbMIDIsystemCin_t::sysExStop1byteCin));
                usbPacket[USB_DATA1] = inArray[0];
                usbPacket[USB_DATA2] = 0;
                usbPacket[USB_DATA3] = 0;

                usbWrite(usbPacket);
            }
        }
    }
}

///
/// \brief Send a Tune Request message.
///
/// When a MIDI unit receives this message, it should tune its oscillators (if equipped with any).
///
void MIDIlib::sendTuneRequest()
{
    dinWrite(static_cast<uint8_t>(messageType_t::sysCommonTuneRequest));

    if (_useRunningStatus)
    {
        _mRunningStatusTX = static_cast<uint8_t>(messageType_t::invalid);
    }
}

///
/// \brief Send a MIDI Time Code Quarter Frame.
/// \param inTypeNibble [in]    MTC type.
/// \param inValuesNibble [in]  MTC data.
/// See MIDI Specification for more information.
///
void MIDIlib::sendTimeCodeQuarterFrame(uint8_t inTypeNibble, uint8_t inValuesNibble)
{
    uint8_t data = (((inTypeNibble & 0x07) << 4) | (inValuesNibble & 0x0f));
    sendTimeCodeQuarterFrame(data);
}

///
/// \brief Send a MIDI Time Code Quarter Frame.
/// \param inData [in]  If you want to encode directly the nibbles in your program,
///                     you can send the byte here.
///
void MIDIlib::sendTimeCodeQuarterFrame(uint8_t inData)
{
    dinWrite(static_cast<uint8_t>(messageType_t::sysCommonTimeCodeQuarterFrame));
    dinWrite(inData);

    if (_useRunningStatus)
    {
        _mRunningStatusTX = static_cast<uint8_t>(messageType_t::invalid);
    }
}

///
/// \brief Send a Song Position Pointer message.
/// \param inBeats [in]     The number of beats since the start of the song.
///
void MIDIlib::sendSongPosition(uint16_t inBeats)
{
    dinWrite(static_cast<uint8_t>(messageType_t::sysCommonSongPosition));
    dinWrite(inBeats & 0x7f);
    dinWrite((inBeats >> 7) & 0x7f);

    if (_useRunningStatus)
    {
        _mRunningStatusTX = static_cast<uint8_t>(messageType_t::invalid);
    }
}

///
/// \brief Send a Song Select message.
/// \param inSongNumber [in]    Song to select (0-127).
///
void MIDIlib::sendSongSelect(uint8_t inSongNumber)
{
    dinWrite(static_cast<uint8_t>(messageType_t::sysCommonSongSelect));
    dinWrite(inSongNumber & 0x7f);

    if (_useRunningStatus)
    {
        _mRunningStatusTX = static_cast<uint8_t>(messageType_t::invalid);
    }
}

///
/// \brief Send a Real Time (one byte) message.
/// \param inType [in]  The available Real Time types are:
///                     Start, Stop, Continue, Clock, ActiveSensing and SystemReset.
///
void MIDIlib::sendRealTime(messageType_t inType)
{
    switch (inType)
    {
    case messageType_t::sysRealTimeClock:
    case messageType_t::sysRealTimeStart:
    case messageType_t::sysRealTimeStop:
    case messageType_t::sysRealTimeContinue:
    case messageType_t::sysRealTimeActiveSensing:
    case messageType_t::sysRealTimeSystemReset:
    {
        dinWrite(static_cast<uint8_t>(inType));

        usbMIDIPacket_t usbPacket;

        usbPacket[USB_EVENT] = static_cast<uint8_t>(messageType_t::systemExclusive) >> 4;
        usbPacket[USB_DATA1] = static_cast<uint8_t>(inType);
        usbPacket[USB_DATA2] = 0;
        usbPacket[USB_DATA3] = 0;

        usbWrite(usbPacket);
    }
    break;

    default:
        // invalid Real Time marker
        break;
    }
}

///
/// \brief Sends transport control messages.
/// Based on MIDI specification for transport control.
///
void MIDIlib::sendMMC(uint8_t deviceID, messageType_t mmc)
{
    switch (mmc)
    {
    case messageType_t::mmcPlay:
    case messageType_t::mmcStop:
    case messageType_t::mmcPause:
    case messageType_t::mmcRecordStart:
    case messageType_t::mmcRecordStop:
        break;

    default:
        return;
    }

    uint8_t mmcArray[6] = { 0xF0, 0x7F, 0x7F, 0x06, 0x00, 0xF7 };
    mmcArray[2]         = deviceID;
    mmcArray[4]         = static_cast<uint8_t>(mmc);

    sendSysEx(6, mmcArray, true);
}

void MIDIlib::sendNRPN(uint16_t inParameterNumber, uint16_t inValue, uint8_t inChannel, bool value14bit)
{
    auto inParameterNumberSplit = Split14bit(inParameterNumber);

    sendControlChange(99, inParameterNumberSplit.high(), inChannel);
    sendControlChange(98, inParameterNumberSplit.low(), inChannel);

    if (!value14bit)
    {
        sendControlChange(6, inValue, inChannel);
    }
    else
    {
        auto inValueSplit = Split14bit(inValue);

        sendControlChange(6, inValueSplit.high(), inChannel);
        sendControlChange(38, inValueSplit.low(), inChannel);
    }
}

void MIDIlib::sendControlChange14bit(uint16_t inControlNumber, uint16_t inControlValue, uint8_t inChannel)
{
    auto inControlValueSplit = Split14bit(inControlValue);

    sendControlChange(inControlNumber, inControlValueSplit.high(), inChannel);
    sendControlChange(inControlNumber + 32, inControlValueSplit.low(), inChannel);
}

///
/// \brief Enable or disable running status.
///
/// Applies to DIN MIDI only (outgoing messages).
/// \param [in] state   True when enabling running status, false otherwise.
///
void MIDIlib::setRunningStatusState(bool state)
{
    _useRunningStatus = state;
}

///
/// \brief Returns current running status state for outgoing DIN MIDI messages.
/// \returns True if running status is enabled, false otherwise.
///
bool MIDIlib::runningStatusState()
{
    return _useRunningStatus;
}

///
/// \brief Calculates MIDI status byte for a given message type and channel.
/// \param inType [in]      MIDI message type.
/// \param inChannel [in]   MIDI channel.
/// \returns Calculated status byte.
///
uint8_t MIDIlib::status(messageType_t inType, uint8_t inChannel)
{
    return (static_cast<uint8_t>(inType) | ((inChannel - 1) & 0x0f));
}

///
/// \brief Reads UART or USB and tries to parse MIDI message.
///
/// A valid message is a message that matches the input channel.
/// If the Thru is enabled and the message matches the filter,
/// it is sent back on the MIDI output.
/// \param type [in]        MIDI interface which is being read (USB or UART). See interface_t.
/// \param filterMode [in]  Thru filter mode. See filterMode_t.
/// \returns True on successful read.
///
bool MIDIlib::read(interface_t type, filterMode_t filterMode)
{
    if (_mInputChannel == MIDI_CHANNEL_OFF)
    {
        return false;    // MIDI Input disabled
    }

    if (!parse(type))
    {
        return false;
    }

    const bool channelMatch = inputFilter(_mInputChannel, type);
    thruFilter(_mInputChannel, type, filterMode);

    return channelMatch;
}

///
/// \brief Handles parsing of MIDI messages.
/// \param type [in] MIDI interface from which messages are being parsed.
///
bool MIDIlib::parse(interface_t type)
{
    if (type == interface_t::din)
    {
        uint8_t data = 0;

        if (!dinRead(data))
        {
            return false;    // no data available
        }

        const uint8_t extracted = data;

        if (_dinPendingMessageIndex == 0)
        {
            // start a new pending message
            _mPendingMessage[0] = extracted;

            // check for running status first (din only)
            if (isChannelMessage(typeFromStatusByte(_mRunningStatusRX)))
            {
                // Only channel messages allow Running Status.
                // If the status byte is not received, prepend it to the pending message.
                if (extracted < 0x80)
                {
                    _mPendingMessage[0]     = _mRunningStatusRX;
                    _mPendingMessage[1]     = extracted;
                    _dinPendingMessageIndex = 1;
                }

                // Else: well, we received another status byte,
                // so the running status does not apply here.
                // It will be updated upon completion of this message.
            }

            switch (typeFromStatusByte(_mPendingMessage[0]))
            {
            // 1 byte messages
            case messageType_t::sysRealTimeStart:
            case messageType_t::sysRealTimeContinue:
            case messageType_t::sysRealTimeStop:
            case messageType_t::sysRealTimeClock:
            case messageType_t::sysRealTimeActiveSensing:
            case messageType_t::sysRealTimeSystemReset:
            case messageType_t::sysCommonTuneRequest:
            {
                // handle the message type directly here.
                _dinMessage.type    = typeFromStatusByte(_mPendingMessage[0]);
                _dinMessage.channel = 0;
                _dinMessage.data1   = 0;
                _dinMessage.data2   = 0;
                _dinMessage.valid   = true;

                // we still need to reset these
                _dinPendingMessageIndex          = 0;
                _dinPendingMessageExpectedLenght = 0;

                return true;
            }
            break;

            // 2 bytes messages
            case messageType_t::programChange:
            case messageType_t::afterTouchChannel:
            case messageType_t::sysCommonTimeCodeQuarterFrame:
            case messageType_t::sysCommonSongSelect:
            {
                _dinPendingMessageExpectedLenght = 2;
            }
            break;

            // 3 bytes messages
            case messageType_t::noteOn:
            case messageType_t::noteOff:
            case messageType_t::controlChange:
            case messageType_t::pitchBend:
            case messageType_t::afterTouchPoly:
            case messageType_t::sysCommonSongPosition:
            {
                _dinPendingMessageExpectedLenght = 3;
            }
            break;

            case messageType_t::systemExclusive:
            {
                // the message can be any length between 3 and MIDI_SYSEX_ARRAY_SIZE
                _dinPendingMessageExpectedLenght = MIDI_SYSEX_ARRAY_SIZE;
                _mRunningStatusRX                = static_cast<uint8_t>(messageType_t::invalid);
                _dinMessage.sysexArray[0]        = static_cast<uint8_t>(messageType_t::systemExclusive);
            }
            break;

            case messageType_t::invalid:
            default:
            {
                resetInput();

                return false;
            }
            break;
            }

            if (_dinPendingMessageIndex >= (_dinPendingMessageExpectedLenght - 1))
            {
                // reception complete
                _dinMessage.type    = typeFromStatusByte(_mPendingMessage[0]);
                _dinMessage.channel = channelFromStatusByte(_mPendingMessage[0], _zeroStartChannel);
                _dinMessage.data1   = _mPendingMessage[1];

                // save data2 only if applicable
                if (_dinPendingMessageExpectedLenght == 3)
                {
                    _dinMessage.data2 = _mPendingMessage[2];
                }
                else
                {
                    _dinMessage.data2 = 0;
                }

                _dinPendingMessageIndex          = 0;
                _dinPendingMessageExpectedLenght = 0;
                _dinMessage.valid                = true;

                return true;
            }

            // waiting for more data
            _dinPendingMessageIndex++;

            if (!_recursiveParseState)
            {
                // message is not complete.
                return false;
            }

            return parse(interface_t::din);
        }

        // first, test if this is a status byte
        if (extracted >= 0x80)
        {
            // Reception of status bytes in the middle of an uncompleted message
            // are allowed only for interleaved Real Time message or EOX.
            switch (extracted)
            {
            case static_cast<uint8_t>(messageType_t::sysRealTimeClock):
            case static_cast<uint8_t>(messageType_t::sysRealTimeStart):
            case static_cast<uint8_t>(messageType_t::sysRealTimeContinue):
            case static_cast<uint8_t>(messageType_t::sysRealTimeStop):
            case static_cast<uint8_t>(messageType_t::sysRealTimeActiveSensing):
            case static_cast<uint8_t>(messageType_t::sysRealTimeSystemReset):
            {
                // Here we will have to extract the one-byte message,
                // pass it to the structure for being read outside
                // the MIDI class, and recompose the message it was
                // interleaved into without killing the running status..
                // This is done by leaving the pending message as is,
                // it will be completed on next calls.
                _dinMessage.type    = static_cast<messageType_t>(extracted);
                _dinMessage.data1   = 0;
                _dinMessage.data2   = 0;
                _dinMessage.channel = 0;
                _dinMessage.valid   = true;

                return true;
            }
            break;

            // end of sysex
            case 0xF7:
            {
                if (_dinMessage.sysexArray[0] == static_cast<uint8_t>(messageType_t::systemExclusive))
                {
                    // store the last byte (EOX)
                    _dinMessage.sysexArray[_dinPendingMessageIndex++] = 0xf7;
                    _dinMessage.type                                  = messageType_t::systemExclusive;

                    // get length
                    _dinMessage.data1   = _dinPendingMessageIndex & 0xff;    // LSB
                    _dinMessage.data2   = _dinPendingMessageIndex >> 8;      // MSB
                    _dinMessage.channel = 0;
                    _dinMessage.valid   = true;

                    resetInput();
                    return true;
                }

                // error
                resetInput();
                return false;
            }
            break;

            default:
                break;
            }
        }

        // add extracted data byte to pending message
        if (_mPendingMessage[0] == static_cast<uint8_t>(messageType_t::systemExclusive))
        {
            _dinMessage.sysexArray[_dinPendingMessageIndex] = extracted;
        }
        else
        {
            _mPendingMessage[_dinPendingMessageIndex] = extracted;
        }

        // now we are going to check if we have reached the end of the message
        if (_dinPendingMessageIndex >= (_dinPendingMessageExpectedLenght - 1))
        {
            //"FML" case: fall down here with an overflown SysEx..
            // This means we received the last possible data byte that can fit the buffer.
            // If this happens, try increasing MIDI_SYSEX_ARRAY_SIZE.
            if (_mPendingMessage[0] == static_cast<uint8_t>(messageType_t::systemExclusive))
            {
                resetInput();
                return false;
            }

            _dinMessage.type = typeFromStatusByte(_mPendingMessage[0]);

            if (isChannelMessage(_dinMessage.type))
            {
                _dinMessage.channel = channelFromStatusByte(_mPendingMessage[0], _zeroStartChannel);
            }
            else
            {
                _dinMessage.channel = 0;
            }

            _dinMessage.data1 = _mPendingMessage[1];

            // save data2 only if applicable
            if (_dinPendingMessageExpectedLenght == 3)
            {
                _dinMessage.data2 = _mPendingMessage[2];
            }
            else
            {
                _dinMessage.data2 = 0;
            }

            // reset local variables
            _dinPendingMessageIndex          = 0;
            _dinPendingMessageExpectedLenght = 0;

            _dinMessage.valid = true;

            // activate running status (if enabled for the received type)
            switch (_dinMessage.type)
            {
            case messageType_t::noteOff:
            case messageType_t::noteOn:
            case messageType_t::afterTouchPoly:
            case messageType_t::controlChange:
            case messageType_t::programChange:
            case messageType_t::afterTouchChannel:
            case messageType_t::pitchBend:
            {
                // running status enabled: store it from received message
                _mRunningStatusRX = _mPendingMessage[0];
            }
            break;

            default:
            {
                // no running status
                _mRunningStatusRX = static_cast<uint8_t>(messageType_t::invalid);
            }
            break;
            }

            return true;
        }

        // update the index of the pending message
        _dinPendingMessageIndex++;

        if (!_recursiveParseState)
        {
            return false;    // message is not complete.
        }

        return parse(interface_t::din);
    }

    if (type == interface_t::usb)
    {
        if (!usbRead(_usbMIDIpacket))
        {
            return false;    // nothing received
        }

        // We already have entire message here.
        // MIDIEvent.Event is CIN, see midi10.pdf.
        // Shift CIN four bytes left to get messageType_t.
        uint8_t midiMessage = _usbMIDIpacket[USB_EVENT] << 4;

        switch (midiMessage)
        {
        // 1 byte messages
        case static_cast<uint8_t>(usbMIDIsystemCin_t::sysCommon1byteCin):
        case static_cast<uint8_t>(usbMIDIsystemCin_t::singleByte):
        {
            if (_usbMIDIpacket[USB_DATA1] != 0xF7)
            {
                // this isn't end of sysex, it's 1byte system common message

                // case messageType_t::sysRealTimeClock:
                // case messageType_t::sysRealTimeStart:
                // case messageType_t::sysRealTimeContinue:
                // case messageType_t::sysRealTimeStop:
                // case messageType_t::sysRealTimeActiveSensing:
                // case messageType_t::sysRealTimeSystemReset:
                _usbMessage.type    = static_cast<messageType_t>(_usbMIDIpacket[USB_DATA1]);
                _usbMessage.channel = 0;
                _usbMessage.data1   = 0;
                _usbMessage.data2   = 0;
                _usbMessage.valid   = true;

                return true;
            }

            // end of sysex
            _usbMessage.sysexArray[_sysExArrayLength] = _usbMIDIpacket[USB_DATA1];
            _sysExArrayLength++;
            _usbMessage.type    = messageType_t::systemExclusive;
            _usbMessage.channel = 0;
            _usbMessage.valid   = true;

            return true;
        }
        break;

        // 2 byte messages
        case static_cast<uint8_t>(usbMIDIsystemCin_t::sysCommon2byteCin):
        case static_cast<uint8_t>(messageType_t::programChange):
        {
            // case static_cast<uint8_t>(messageType_t::afterTouchChannel):
            // case static_cast<uint8_t>(messageType_t::sysCommonTimeCodeQuarterFrame):
            // case static_cast<uint8_t>(messageType_t::sysCommonSongSelect):
            _usbMessage.type    = static_cast<messageType_t>(_usbMIDIpacket[USB_DATA1]);
            _usbMessage.channel = channelFromStatusByte(_usbMIDIpacket[USB_DATA1], _zeroStartChannel);
            _usbMessage.data1   = _usbMIDIpacket[USB_DATA2];
            _usbMessage.data2   = 0;
            _usbMessage.valid   = true;

            return true;
        }
        break;

        // 3 byte messages
        case static_cast<uint8_t>(messageType_t::noteOn):
        case static_cast<uint8_t>(messageType_t::noteOff):
        case static_cast<uint8_t>(messageType_t::controlChange):
        case static_cast<uint8_t>(messageType_t::pitchBend):
        case static_cast<uint8_t>(messageType_t::afterTouchPoly):
        case static_cast<uint8_t>(messageType_t::sysCommonSongPosition):
        {
            _usbMessage.type    = static_cast<messageType_t>(midiMessage);
            _usbMessage.channel = channelFromStatusByte(_usbMIDIpacket[USB_DATA1], _zeroStartChannel);
            _usbMessage.data1   = _usbMIDIpacket[USB_DATA2];
            _usbMessage.data2   = _usbMIDIpacket[USB_DATA3];
            _usbMessage.valid   = true;

            return true;
        }
        break;

        // sysex
        case static_cast<uint8_t>(usbMIDIsystemCin_t::sysExStartCin):
        {
            // the message can be any length between 3 and MIDI_SYSEX_ARRAY_SIZE
            if (_usbMIDIpacket[USB_DATA1] == 0xF0)
            {
                // this is a new sysex message, reset length
                _sysExArrayLength = 0;
            }

            _usbMessage.sysexArray[_sysExArrayLength] = _usbMIDIpacket[USB_DATA1];
            _sysExArrayLength++;
            _usbMessage.sysexArray[_sysExArrayLength] = _usbMIDIpacket[USB_DATA2];
            _sysExArrayLength++;
            _usbMessage.sysexArray[_sysExArrayLength] = _usbMIDIpacket[USB_DATA3];
            _sysExArrayLength++;

            return false;
        }
        break;

        case static_cast<uint8_t>(usbMIDIsystemCin_t::sysExStop2byteCin):
        {
            _usbMessage.sysexArray[_sysExArrayLength] = _usbMIDIpacket[USB_DATA1];
            _sysExArrayLength++;
            _usbMessage.sysexArray[_sysExArrayLength] = _usbMIDIpacket[USB_DATA2];
            _sysExArrayLength++;
            _usbMessage.type    = messageType_t::systemExclusive;
            _usbMessage.channel = 0;
            _usbMessage.valid   = true;

            return true;
        }
        break;

        case static_cast<uint8_t>(usbMIDIsystemCin_t::sysExStop3byteCin):
        {
            if (_usbMIDIpacket[USB_DATA1] == 0xF0)
            {
                // sysex message with 1 byte of payload
                _sysExArrayLength = 0;
            }

            _usbMessage.sysexArray[_sysExArrayLength] = _usbMIDIpacket[USB_DATA1];
            _sysExArrayLength++;
            _usbMessage.sysexArray[_sysExArrayLength] = _usbMIDIpacket[USB_DATA2];
            _sysExArrayLength++;
            _usbMessage.sysexArray[_sysExArrayLength] = _usbMIDIpacket[USB_DATA3];
            _sysExArrayLength++;
            _usbMessage.type    = messageType_t::systemExclusive;
            _usbMessage.channel = 0;
            _usbMessage.valid   = true;

            return true;
        }
        break;

        default:
            return false;
        }
    }
    else
    {
        return false;
    }
}

///
/// \brief Check if the received message is on the listened channel.
/// \param [in] inChannel   MIDI channel of the received message.
/// \param type [in]        MIDI interface which is being checked.
/// \returns True if channel matches listened channel.
///
bool MIDIlib::inputFilter(uint8_t inChannel, interface_t type)
{
    switch (type)
    {
    case interface_t::din:
    {
        if (_dinMessage.type == messageType_t::invalid)
        {
            return false;
        }

        // first, check if the received message is Channel
        if (_dinMessage.type >= messageType_t::noteOff && _dinMessage.type <= messageType_t::pitchBend)
        {
            // then we need to know if we listen to it
            if ((_dinMessage.channel == inChannel) || (inChannel == MIDI_CHANNEL_OMNI))
            {
                return true;
            }

            return false;    // we don't listen to this channel
        }

        // system messages are always received
        return true;
    }
    break;

    case interface_t::usb:
    {
        if (_usbMessage.type == messageType_t::invalid)
        {
            return false;
        }

        // first, check if the received message is Channel
        if (_usbMessage.type >= messageType_t::noteOff && _usbMessage.type <= messageType_t::pitchBend)
        {
            // then we need to know if we listen to it
            if ((_usbMessage.channel == inChannel) || (inChannel == MIDI_CHANNEL_OMNI))
            {
                return true;
            }

            return false;    // we don't listen to this channel
        }

        // system messages are always received
        return true;
    }
    break;

    default:
        return false;
    }
}

///
/// \brief Reset attributes for incoming message (DIN MIDI only).
///
void MIDIlib::resetInput()
{
    _dinPendingMessageIndex          = 0;
    _dinPendingMessageExpectedLenght = 0;
    _mRunningStatusRX                = static_cast<uint8_t>(messageType_t::invalid);
}

///
/// \brief Retrieves the MIDI message type of the last received message.
/// \param type [in]    MIDI interface which is being checked.
/// \returns MIDI message type of the last received message.
///
MIDIlib::messageType_t MIDIlib::type(interface_t type)
{
    // get the last received message's type
    switch (type)
    {
    case interface_t::din:
        return _dinMessage.type;

    case interface_t::usb:
        return _usbMessage.type;

    default:
        return messageType_t::invalid;
    }
}

///
/// \brief Retrieves the MIDI channel of the last received message.
/// \param type [in]    MIDI interface which is being checked.
/// \returns MIDI channel of the last received message. For non-channel messages, this will return 0.
///
uint8_t MIDIlib::channel(interface_t type)
{
    switch (type)
    {
    case interface_t::din:
        return _dinMessage.channel;

    case interface_t::usb:
        return _usbMessage.channel;

    default:
        return MIDI_CHANNEL_INVALID;
    }
}

///
/// \brief Retrieves the first data byte of the last received message.
/// \param type [in]    MIDI interface which is being checked.
/// \returns First data byte of the last received message.
///
uint8_t MIDIlib::data1(interface_t type)
{
    switch (type)
    {
    case interface_t::din:
        return _dinMessage.data1;

    case interface_t::usb:
        return _usbMessage.data1;

    default:
        return 0;
    }
}

///
/// \brief Retrieves the second data byte of the last received message.
/// \param type [in]    MIDI interface which is being checked.
/// \returns Second data byte of the last received message.
///
uint8_t MIDIlib::data2(interface_t type)
{
    switch (type)
    {
    case interface_t::din:
        return _dinMessage.data2;

    case interface_t::usb:
        return _usbMessage.data2;

    default:
        return 0;
    }
}

///
/// \brief Retrieves memory location in which SysEx array is being stored.
/// \param [in] type    MIDI interface from which SysEx array is located.
/// \returns Pointer to SysEx array.
///
uint8_t* MIDIlib::sysExArray(interface_t type)
{
    // get the System Exclusive byte array
    switch (type)
    {
    case interface_t::din:
        return _dinMessage.sysexArray;

    case interface_t::usb:
        return _usbMessage.sysexArray;

    default:
        return nullptr;
    }
}

///
/// \brief Checks size of SysEx array.
/// \param [in] type    MIDI interface on which SysEx size is being checked.
/// \returns Size of SysEx array on given MIDI interface in bytes.
///
uint16_t MIDIlib::sysExArrayLength(interface_t type)
{
    switch (type)
    {
    case interface_t::din:
    {
        uint16_t size = unsigned(_dinMessage.data2) << 8 | _dinMessage.data1;
        return size > MIDI_SYSEX_ARRAY_SIZE ? MIDI_SYSEX_ARRAY_SIZE : size;
    }
    break;

    case interface_t::usb:
    {
        return _sysExArrayLength > MIDI_SYSEX_ARRAY_SIZE ? MIDI_SYSEX_ARRAY_SIZE : _sysExArrayLength;
    }
    break;

    default:
        return 0;
    }
}

///
/// \brief Checks MIDI channel on which incoming messages are being listened.
/// \returns MIDI channel value (1-16) if _zeroStartChannel is disabled, 0-15 otherwise.
///          Two additional values can be returned (MIDI_CHANNEL_OMNI and MIDI_CHANNEL_OFF).
///
uint8_t MIDIlib::inputChannel()
{
    if ((_mInputChannel == MIDI_CHANNEL_OMNI) || (_mInputChannel == MIDI_CHANNEL_OFF))
    {
        return _mInputChannel;
    }

    return _mInputChannel - _zeroStartChannel;
}

///
/// \brief Configures MIDI channel on which incoming messages are being listened.
/// \param inChannel [in]   The channel value. Valid values are 1 to 16 if _zeroStartChannel
///                         is set to false, otherwise valid values are 0-15. Two additional values
///                         can be passed:
///                         MIDI_CHANNEL_OMNI value is used to listen on all channels (default).
///                         MIDI_CHANNEL_OFF value is used to disable input.
/// \returns                True if passed channel is valid.
///
bool MIDIlib::setInputChannel(uint8_t inChannel)
{
    bool valid = false;

    if ((inChannel == MIDI_CHANNEL_OMNI) || (inChannel == MIDI_CHANNEL_OFF))
    {
        valid = true;
    }
    else
    {
        if (_zeroStartChannel)
        {
            if (_mInputChannel >= 16)
            {
                valid = false;
            }
        }
        else
        {
            if (!_mInputChannel || (_mInputChannel > 16))
            {
                valid = false;
            }
        }
    }

    if (valid)
    {
        _mInputChannel = inChannel;
        return true;
    }

    return false;
}

///
/// \brief Extract MIDI message type from status byte.
/// \param inStatus [in]    Status byte.
/// \returns Extracted MIDI message type.
///
MIDIlib::messageType_t MIDIlib::typeFromStatusByte(uint8_t inStatus)
{
    // extract an enumerated MIDI type from a status byte

    if ((inStatus < 0x80) ||
        (inStatus == 0xf4) ||
        (inStatus == 0xf5) ||
        (inStatus == 0xf9) ||
        (inStatus == 0xfD))
    {
        return messageType_t::invalid;
    }

    if (inStatus < 0xf0)
    {
        // channel message, remove channel nibble
        return static_cast<messageType_t>(inStatus & 0xf0);
    }

    return static_cast<messageType_t>(inStatus);
}

///
/// \brief Extract MIDI channel from status byte.
/// \param inStatus [in]            Status byte.
/// \param zeroStartChannel [in]    Specifies whether to return channel starting from 0.
/// \returns Extracted MIDI channel.
///
uint8_t MIDIlib::channelFromStatusByte(uint8_t inStatus, bool zeroStartChannel)
{
    return (inStatus & 0x0f) + 1 - zeroStartChannel;
}

///
/// \brief Checks if MIDI message is channel message.
/// \param inType [in]  Type of MIDI message which is being checked.
/// \returns True if requested MIDI message type is channel type, false otherwise.
///
bool MIDIlib::isChannelMessage(messageType_t inType)
{
    return (inType == messageType_t::noteOff ||
            inType == messageType_t::noteOn ||
            inType == messageType_t::controlChange ||
            inType == messageType_t::afterTouchPoly ||
            inType == messageType_t::afterTouchChannel ||
            inType == messageType_t::pitchBend ||
            inType == messageType_t::programChange);
}

///
/// \brief Used to enable or disable recursive parsing of incoming messages on UART interface.
/// Setting this to false will make MIDI.read parse only one byte of data for each
/// call when data is available. This can speed up your application if receiving
/// a lot of traffic, but might induce MIDI Thru and treatment latency.
/// \param state [in]   Set to true to enable recursive parsing or false to disable it.
///
void MIDIlib::useRecursiveParsing(bool state)
{
    _recursiveParseState = state;
}

///
/// \brief Responsible for MIDI Thru message forwarding.
///
/// This method is called upon reception of a message and takes
/// care of filtering and sending. All system messages (System Exclusive, Common and Real Time)
/// are passed to output unless filter is set to THRU_OFF.
/// Channel messages are passed to the output depending on the filter setting.
/// \param inChannel [in]   Channel of the incoming MIDI message.
/// \param type [in]        MIDI interface which is being checked.
/// \param filterMode [in]  MIDI thru filtering mode.
///
void MIDIlib::thruFilter(uint8_t inChannel, interface_t type, filterMode_t filterMode)
{
    // if the feature is disabled, don't do anything.
    if (filterMode == filterMode_t::off)
    {
        return;
    }

    message_t& msg = type == interface_t::din ? _dinMessage : _usbMessage;

    bool dinState = _dinEnabled;
    bool usbState = _usbEnabled;

    switch (filterMode)
    {
    case filterMode_t::fullDIN:
    case filterMode_t::channelDIN:
    {
        _usbEnabled = false;
    }
    break;

    case filterMode_t::fullUSB:
    case filterMode_t::channelUSB:
    {
        _dinEnabled = false;
    }
    break;

    case filterMode_t::fullAll:
    case filterMode_t::channelAll:
    {
        // leave as is
    }
    break;

    default:
        return;
    }

    // first, check if the received message is Channel
    if (msg.type >= messageType_t::noteOff && msg.type <= messageType_t::pitchBend)
    {
        const bool filter_condition = ((msg.channel == _mInputChannel) ||
                                       (_mInputChannel == MIDI_CHANNEL_OMNI));

        // now let's pass it to the output
        switch (filterMode)
        {
        case filterMode_t::fullDIN:
        case filterMode_t::fullUSB:
        case filterMode_t::fullAll:
        {
            send(msg.type, msg.data1, msg.data2, msg.channel);
        }
        break;

        case filterMode_t::channelDIN:
        case filterMode_t::channelUSB:
        case filterMode_t::channelAll:
        {
            if (filter_condition)
            {
                send(msg.type, msg.data1, msg.data2, msg.channel);
            }
        }
        break;

        default:
            break;
        }
    }
    else
    {
        // always forward system messages
        switch (msg.type)
        {
        // real Time and 1 byte
        case messageType_t::sysRealTimeClock:
        case messageType_t::sysRealTimeStart:
        case messageType_t::sysRealTimeStop:
        case messageType_t::sysRealTimeContinue:
        case messageType_t::sysRealTimeActiveSensing:
        case messageType_t::sysRealTimeSystemReset:
        case messageType_t::sysCommonTuneRequest:
        {
            sendRealTime(msg.type);
        }
        break;

        case messageType_t::systemExclusive:
        {
            // send SysEx (0xf0 and 0xf7 are included in the buffer)
            sendSysEx(sysExArrayLength(type), sysExArray(type), true);
        }
        break;

        case messageType_t::sysCommonSongSelect:
        {
            sendSongSelect(msg.data1);
        }
        break;

        case messageType_t::sysCommonSongPosition:
        {
            sendSongPosition(msg.data1 | ((unsigned)msg.data2 << 7));
        }
        break;

        case messageType_t::sysCommonTimeCodeQuarterFrame:
        {
            sendTimeCodeQuarterFrame(msg.data1, msg.data2);
        }
        break;

        default:
            break;
        }
    }

    // restore original state
    _dinEnabled = dinState;
    _usbEnabled = usbState;
}

///
/// \brief Configures how Note Off messages are sent.
/// \param type [in]    Type of MIDI Note Off message. See noteOffType_t.
///
void MIDIlib::setNoteOffMode(noteOffType_t type)
{
    _noteOffMode = type;
}

///
/// \brief Checks how MIDI Note Off messages are being sent.
///
MIDIlib::noteOffType_t MIDIlib::noteOffMode()
{
    return _noteOffMode;
}

///
/// \brief Calculates octave from received MIDI note.
/// @param [in] note    Raw MIDI note (0-127).
/// \returns Calculated octave (0 - MIDI_NOTES-1).
///
uint8_t MIDIlib::noteToOctave(int8_t note)
{
    // sanitize input
    note &= 0x7F;

    return note / static_cast<uint8_t>(note_t::AMOUNT);
}

///
/// \brief Calculates tonic (root note) from received raw MIDI note.
/// @param [in] note    Raw MIDI note (0-127).
/// \returns Calculated tonic/root note (enumerated type). See note_t enumeration.
///
MIDIlib::note_t MIDIlib::noteToTonic(int8_t note)
{
    // sanitize input
    note &= 0x7F;

    return static_cast<note_t>(note % static_cast<uint8_t>(note_t::AMOUNT));
}

///
/// \brief Configures the way channels are calculated internally when sending MIDI messages.
/// @param state [in]   When set to true, all channels passed to library
///                     need to start from zero (channel 1 is 0, channel 16 is 15).
///                     Otherwise, MIDI channels need to be passed as they are
///                     (channel 1 is 1, channel 16 is 16).
///                     This is used only when sending MIDI messages. When requesting MIDI
///                     channel, normal 1-16 values are used.
///
void MIDIlib::setChannelSendZeroStart(bool state)
{
    _zeroStartChannel = state ? true : false;
}

bool MIDIlib::dinWrite(uint8_t data)
{
    if (!_dinEnabled)
    {
        return false;
    }

    return hwa.dinWrite(data);
}

bool MIDIlib::dinRead(uint8_t& data)
{
    if (!_dinEnabled)
    {
        return false;
    }

    return hwa.dinRead(data);
}

bool MIDIlib::usbWrite(usbMIDIPacket_t& packet)
{
    if (!_usbEnabled)
    {
        return false;
    }

    return hwa.usbWrite(packet);
}

bool MIDIlib::usbRead(usbMIDIPacket_t& packet)
{
    if (!_usbEnabled)
    {
        return false;
    }

    return hwa.usbRead(packet);
}