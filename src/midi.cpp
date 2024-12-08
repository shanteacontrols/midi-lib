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

#include "lib/midi/midi.h"
#include <cstddef>

using namespace lib::midi;

bool Base::init()
{
    if (_initialized)
    {
        return true;
    }

    reset();

    if (_transport.init())
    {
        _initialized = true;
        return true;
    }

    return false;
}

bool Base::deInit()
{
    if (!_initialized)
    {
        return true;
    }

    reset();
    _initialized = false;
    return _transport.deInit();
}

bool Base::initialized()
{
    return _initialized;
}

void Base::reset()
{
    _mRunningStatusRX             = 0;
    _pendingMessageExpectedLength = 0;
    _pendingMessageIndex          = 0;
}

Transport& Base::transport()
{
    return _transport;
}

/// Generate and send a MIDI message from the values given.
/// Use this only if you need to send raw data.
bool Base::send(messageType_t inType, uint8_t inData1, uint8_t inData2, uint8_t inChannel)
{
    bool channelValid = true;

    // test if channel is valid
    if ((inChannel > 16) || !inChannel)
    {
        channelValid = false;
    }

    if (!channelValid || (inType < messageType_t::NOTE_OFF))
    {
        if (_useRunningStatus)
        {
            _mRunningStatusTX = static_cast<uint8_t>(messageType_t::INVALID);
        }

        return false;    // don't send anything
    }

    if (inType <= messageType_t::PITCH_BEND)
    {
        // channel messages
        // protection: remove MSBs on data
        inData1 &= 0x7F;
        inData2 &= 0x7F;

        const uint8_t IN_STATUS = status(inType, inChannel);

        if (_transport.beginTransmission(inType))
        {
            if (_useRunningStatus)
            {
                if (_mRunningStatusTX != IN_STATUS)
                {
                    // new message, memorize and send header
                    _mRunningStatusTX = IN_STATUS;

                    if (!_transport.write(_mRunningStatusTX))
                    {
                        return false;
                    }
                }
            }
            else
            {
                // don't care about running status, send the status byte
                if (!_transport.write(IN_STATUS))
                {
                    return false;
                }
            }

            // send data
            if (!_transport.write(inData1))
            {
                return false;
            }

            if ((inType != messageType_t::PROGRAM_CHANGE) && (inType != messageType_t::AFTER_TOUCH_CHANNEL))
            {
                if (!_transport.write(inData2))
                {
                    return false;
                }
            }

            return _transport.endTransmission();
        }
    }
    else if ((inType >= messageType_t::SYS_COMMON_TUNE_REQUEST) && (inType <= messageType_t::SYS_REAL_TIME_SYSTEM_RESET))
    {
        // system real-time and 1 byte
        return sendRealTime(inType);
    }

    return false;
}

/// Send a Note On message.
/// param inNoteNumber [in]    Pitch value in the MIDI format (0 to 127).
/// param inVelocity [in]      Note attack velocity (0 to 127).
/// param inChannel [in]       The channel on which the message will be sent (1 to 16).
bool Base::sendNoteOn(uint8_t inNoteNumber, uint8_t inVelocity, uint8_t inChannel)
{
    return send(messageType_t::NOTE_ON, inNoteNumber, inVelocity, inChannel);
}

/// Send a Note Off message.
/// If note off mode is set to noteOffType_t::standardNoteOff, Note Off message will be sent.
/// If mode is set to noteOffType_noteOnZeroVel, Note On will be sent will velocity 0.
/// param inNoteNumber [in]    Pitch value in the MIDI format (0 to 127).
/// param inVelocity [in]      Release velocity (0 to 127).
/// param inChannel [in]       The channel on which the message will be sent (1 to 16).
bool Base::sendNoteOff(uint8_t inNoteNumber, uint8_t inVelocity, uint8_t inChannel)
{
    if (_noteOffMode == noteOffType_t::STANDARD_NOTE_OFF)
    {
        return send(messageType_t::NOTE_OFF, inNoteNumber, inVelocity, inChannel);
    }

    return send(messageType_t::NOTE_ON, inNoteNumber, inVelocity, inChannel);
}

/// Send a Program Change message.
/// param inProgramNumber [in]     The program to select (0 to 127).
/// param inChannel [in]           The channel on which the message will be sent (1 to 16).
bool Base::sendProgramChange(uint8_t inProgramNumber, uint8_t inChannel)
{
    return send(messageType_t::PROGRAM_CHANGE, inProgramNumber, 0, inChannel);
}

/// Send a Control Change message.
/// param inControlNumber [in]     The controller number (0 to 127).
/// param inControlValue [in]      The value for the specified controller (0 to 127).
/// param inChannel [in]           The channel on which the message will be sent (1 to 16).
bool Base::sendControlChange(uint8_t inControlNumber, uint8_t inControlValue, uint8_t inChannel)
{
    return send(messageType_t::CONTROL_CHANGE, inControlNumber, inControlValue, inChannel);
}

/// Send a Polyphonic AfterTouch message (applies to a specified note).
/// param inPressure [in]    The amount of AfterTouch to apply (0 to 127).
/// param inChannel [in]     The channel on which the message will be sent (1 to 16).
/// param inNoteNumber [in]  The note to apply AfterTouch to (0 to 127).
bool Base::sendAfterTouch(uint8_t inPressure, uint8_t inChannel, uint8_t inNoteNumber)
{
    return send(messageType_t::AFTER_TOUCH_POLY, inNoteNumber, inPressure, inChannel);
}

/// Send a Monophonic AfterTouch message (applies to all notes).
/// param inPressure [in]    The amount of AfterTouch to apply (0 to 127).
/// param inChannel [in]     The channel on which the message will be sent (1 to 16).
bool Base::sendAfterTouch(uint8_t inPressure, uint8_t inChannel)
{
    return send(messageType_t::AFTER_TOUCH_CHANNEL, inPressure, 0, inChannel);
}

/// Send a Pitch Bend message using a signed integer value.
/// param inPitchValue [in]  The amount of bend to send (0-16383).
/// param inChannel [in]     The channel on which the message will be sent (1 to 16).
bool Base::sendPitchBend(uint16_t inPitchValue, uint8_t inChannel)
{
    auto split = Split14Bit(inPitchValue & 0x3FFF);

    return send(messageType_t::PITCH_BEND, split.low(), split.high(), inChannel);
}

/// Send a System Exclusive message.
/// param inLength [in]                    The size of the array to send.
/// param inArray [in]                     The byte array containing the data to send
/// param inArrayContainsBoundaries [in]   When set to 'true', 0xF0 & 0xF7 bytes (start & stop SysEx)
///                                        will not be sent and therefore must be included in the array.
bool Base::sendSysEx(uint16_t inLength, const uint8_t* inArray, bool inArrayContainsBoundaries)
{
    if (_transport.beginTransmission(messageType_t::SYS_EX))
    {
        if (!inArrayContainsBoundaries)
        {
            if (!_transport.write(0xF0))
            {
                return false;
            }
        }

        for (uint16_t i = 0; i < inLength; ++i)
        {
            if (!_transport.write(inArray[i]))
            {
                return false;
            }
        }

        if (!inArrayContainsBoundaries)
        {
            if (!_transport.write(0xF7))
            {
                return false;
            }
        }

        if (_transport.endTransmission())
        {
            if (_useRunningStatus)
            {
                _mRunningStatusTX = static_cast<uint8_t>(messageType_t::INVALID);
            }

            return true;
        }
    }

    return false;
}

/// Send a Tune Request message.
/// When a MIDI unit receives this message, it should tune its oscillators (if equipped with any).
bool Base::sendTuneRequest()
{
    return sendCommon(messageType_t::SYS_COMMON_TUNE_REQUEST);
}

/// Send a MIDI Time Code Quarter Frame.
/// param inTypeNibble [in]    MTC type.
/// param inValuesNibble [in]  MTC data.
/// See MIDI Specification for more information.
bool Base::sendTimeCodeQuarterFrame(uint8_t inTypeNibble, uint8_t inValuesNibble)
{
    const uint8_t DATA = (((inTypeNibble & 0x07) << 4) | (inValuesNibble & 0x0F));
    return sendTimeCodeQuarterFrame(DATA);
}

/// Send a MIDI Time Code Quarter Frame.
/// param inData [in]  If you want to encode directly the nibbles in your program,
///                     you can send the byte here.
bool Base::sendTimeCodeQuarterFrame(uint8_t inData)
{
    return sendCommon(messageType_t::SYS_COMMON_TIME_CODE_QUARTER_FRAME, inData);
}

/// Send a Song Position Pointer message.
/// param inBeats [in]     The number of beats since the start of the song.
bool Base::sendSongPosition(uint16_t inBeats)
{
    return sendCommon(messageType_t::SYS_COMMON_SONG_POSITION, inBeats);
}

/// Send a Song Select message.
/// param inSongNumber [in]    Song to select (0-127).
bool Base::sendSongSelect(uint8_t inSongNumber)
{
    return sendCommon(messageType_t::SYS_COMMON_SONG_SELECT, inSongNumber);
}

/// Send a Common message. Common messages reset the running status.
///  param inType [in]  The available Common types are:
///                     sysCommonTimeCodeQuarterFrame
///                     sysCommonSongPosition
///                     sysCommonSongSelect
///                     sysCommonTuneRequest
/// inData1             The byte that goes with the common message, if any.
bool Base::sendCommon(messageType_t inType, uint8_t inData1)
{
    switch (inType)
    {
    case messageType_t::SYS_COMMON_TIME_CODE_QUARTER_FRAME:
    case messageType_t::SYS_COMMON_SONG_POSITION:
    case messageType_t::SYS_COMMON_SONG_SELECT:
    case messageType_t::SYS_COMMON_TUNE_REQUEST:
        break;    // valid

    default:
        return false;
    }

    if (_transport.beginTransmission(inType))
    {
        if (!_transport.write(static_cast<uint8_t>(inType)))
        {
            return false;
        }

        switch (inType)
        {
        case messageType_t::SYS_COMMON_TIME_CODE_QUARTER_FRAME:
        {
            if (!_transport.write(inData1))
            {
                return false;
            }
        }
        break;

        case messageType_t::SYS_COMMON_SONG_POSITION:
        {
            if (!_transport.write(inData1 & 0x7F))
            {
                return false;
            }

            if (!_transport.write((inData1 >> 7) & 0x7F))
            {
                return false;
            }
        }
        break;

        case messageType_t::SYS_COMMON_SONG_SELECT:
        {
            if (!_transport.write(inData1 & 0x7F))
            {
                return false;
            }
        }
        break;

        case messageType_t::SYS_COMMON_TUNE_REQUEST:
            break;

        default:
            break;
        }

        if (_transport.endTransmission())
        {
            if (_useRunningStatus)
            {
                _mRunningStatusTX = static_cast<uint8_t>(messageType_t::INVALID);
            }
        }
    }

    return false;
}

/// Send a Real Time (one byte) message.
/// param inType [in]   The available Real Time types are:
///                     sysRealTimeClock
///                     sysRealTimeStart
///                     sysRealTimeStop
///                     sysRealTimeContinue
///                     sysRealTimeActiveSensing
///                     sysRealTimeSystemReset
bool Base::sendRealTime(messageType_t inType)
{
    switch (inType)
    {
    case messageType_t::SYS_REAL_TIME_CLOCK:
    case messageType_t::SYS_REAL_TIME_START:
    case messageType_t::SYS_REAL_TIME_STOP:
    case messageType_t::SYS_REAL_TIME_CONTINUE:
    case messageType_t::SYS_REAL_TIME_ACTIVE_SENSING:
    case messageType_t::SYS_REAL_TIME_SYSTEM_RESET:
    {
        if (_transport.beginTransmission(inType))
        {
            if (!_transport.write(static_cast<uint8_t>(inType)))
            {
                return false;
            }

            return _transport.endTransmission();
        }
    }
    break;

    default:
        return false;
    }

    return false;
}

/// Sends transport control messages.
/// Based on MIDI specification for transport control.
/// param deviceID  [in]    See transport control spec.
/// param mmc       [in]    The available MMC types are:
///                         mmcPlay
///                         mmcStop
///                         mmcPause
///                         mmcRecordStart
///                         mmcRecordStop
bool Base::sendMMC(uint8_t deviceID, messageType_t mmc)
{
    switch (mmc)
    {
    case messageType_t::MMC_PLAY:
    case messageType_t::MMC_STOP:
    case messageType_t::MMC_PAUSE:
    case messageType_t::MMC_RECORD_START:
    case messageType_t::MMC_RECORD_STOP:
        break;

    default:
        return false;
    }

    uint8_t mmcArray[6] = { 0xF0, 0x7F, 0x7F, 0x06, 0x00, 0xF7 };
    mmcArray[2]         = deviceID;
    mmcArray[4]         = static_cast<uint8_t>(mmc);

    return sendSysEx(6, mmcArray, true);
}

bool Base::sendNRPN(uint16_t inParameterNumber, uint16_t inValue, uint8_t inChannel, bool value14bit)
{
    auto inParameterNumberSplit = Split14Bit(inParameterNumber);

    if (!sendControlChange(99, inParameterNumberSplit.high(), inChannel))
    {
        return false;
    }

    if (!sendControlChange(98, inParameterNumberSplit.low(), inChannel))
    {
        return false;
    }

    if (!value14bit)
    {
        return sendControlChange(6, inValue, inChannel);
    }

    auto inValueSplit = Split14Bit(inValue);

    if (!sendControlChange(6, inValueSplit.high(), inChannel))
    {
        return false;
    }

    return sendControlChange(38, inValueSplit.low(), inChannel);
}

bool Base::sendControlChange14bit(uint16_t inControlNumber, uint16_t inControlValue, uint8_t inChannel)
{
    auto inControlValueSplit = Split14Bit(inControlValue);

    if (!sendControlChange(inControlNumber, inControlValueSplit.high(), inChannel))
    {
        return false;
    }

    return sendControlChange(inControlNumber + 32, inControlValueSplit.low(), inChannel);
}

/// Enable or disable running status.
/// param [in] state   True when enabling running status, false otherwise.
void Base::setRunningStatusState(bool state)
{
    _useRunningStatus = state;
}

/// Returns current running status state for outgoing DIN MIDI messages.
/// returns: True if running status is enabled, false otherwise.
bool Base::runningStatusState()
{
    return _useRunningStatus;
}

/// Calculates MIDI status byte for a given message type and channel.
/// param inType [in]      MIDI message type.
/// param inChannel [in]   MIDI channel.
/// returns: Calculated status byte.
uint8_t Base::status(messageType_t inType, uint8_t inChannel)
{
    return (static_cast<uint8_t>(inType) | ((inChannel - 1) & 0x0f));
}

/// Reads from transport interface and tries to parse MIDI message.
/// A valid message is a message that matches the input channel.
/// If any thru interface is registered, parsed message is sent to it.
/// returns: True on successful read.
bool Base::read()
{
    if (!parse())
    {
        return false;
    }

    thru();

    return true;
}

/// Handles parsing of MIDI messages.
bool Base::parse()
{
    uint8_t data = 0;

    if (!_transport.read(data))
    {
        return false;    // no data available
    }

    const uint8_t EXTRACTED = data;

    if (_pendingMessageIndex == 0)
    {
        // start a new pending message
        _mPendingMessage[0] = EXTRACTED;

        // check for running status first (din only)
        if (IS_CHANNEL_MESSAGE(TYPE_FROM_STATUS_BYTE(_mRunningStatusRX)))
        {
            // Only channel messages allow Running Status.
            // If the status byte is not received, prepend it to the pending message.
            if (EXTRACTED < 0x80)
            {
                _mPendingMessage[0]  = _mRunningStatusRX;
                _mPendingMessage[1]  = EXTRACTED;
                _pendingMessageIndex = 1;
            }

            // Else: well, we received another status byte,
            // so the running status does not apply here.
            // It will be updated upon completion of this message.
        }

        const auto PENDING_TYPE = TYPE_FROM_STATUS_BYTE(_mPendingMessage[0]);

        switch (PENDING_TYPE)
        {
        // 1 byte messages
        case messageType_t::SYS_REAL_TIME_START:
        case messageType_t::SYS_REAL_TIME_CONTINUE:
        case messageType_t::SYS_REAL_TIME_STOP:
        case messageType_t::SYS_REAL_TIME_CLOCK:
        case messageType_t::SYS_REAL_TIME_ACTIVE_SENSING:
        case messageType_t::SYS_REAL_TIME_SYSTEM_RESET:
        case messageType_t::SYS_COMMON_TUNE_REQUEST:
        {
            // handle the message type directly here
            _message.type    = PENDING_TYPE;
            _message.channel = 0;
            _message.data1   = 0;
            _message.data2   = 0;
            _message.valid   = true;

            // do not reset all input attributes: running status must remain unchanged
            // we still need to reset these
            _pendingMessageIndex          = 0;
            _pendingMessageExpectedLength = 0;

            return true;
        }
        break;

        // 2 bytes messages
        case messageType_t::PROGRAM_CHANGE:
        case messageType_t::AFTER_TOUCH_CHANNEL:
        case messageType_t::SYS_COMMON_TIME_CODE_QUARTER_FRAME:
        case messageType_t::SYS_COMMON_SONG_SELECT:
        {
            _pendingMessageExpectedLength = 2;
        }
        break;

        // 3 bytes messages
        case messageType_t::NOTE_ON:
        case messageType_t::NOTE_OFF:
        case messageType_t::CONTROL_CHANGE:
        case messageType_t::PITCH_BEND:
        case messageType_t::AFTER_TOUCH_POLY:
        case messageType_t::SYS_COMMON_SONG_POSITION:
        {
            _pendingMessageExpectedLength = 3;
        }
        break;

        case messageType_t::SYS_EX:
        {
            // the message can be any length between 3 and MIDI_SYSEX_ARRAY_SIZE
            _pendingMessageExpectedLength = MIDI_SYSEX_ARRAY_SIZE;
            _mRunningStatusRX             = static_cast<uint8_t>(messageType_t::INVALID);
            _message.sysexArray[0]        = static_cast<uint8_t>(messageType_t::SYS_EX);
        }
        break;

        case messageType_t::INVALID:
        default:
        {
            reset();

            return false;
        }
        break;
        }

        if (_pendingMessageIndex >= (_pendingMessageExpectedLength - 1))
        {
            // reception complete
            _message.type                 = PENDING_TYPE;
            _message.channel              = CHANNEL_FROM_STATUS_BYTE(_mPendingMessage[0]);
            _message.data1                = _mPendingMessage[1];
            _message.data2                = 0;
            _message.length               = 1;
            _pendingMessageIndex          = 0;
            _pendingMessageExpectedLength = 0;
            _message.valid                = true;

            return true;
        }

        // waiting for more data
        _pendingMessageIndex++;

        if (!_recursiveParseState)
        {
            // message is not complete
            return false;
        }

        return parse();
    }

    // first, test if this is a status byte
    if (EXTRACTED >= 0x80)
    {
        // Reception of status bytes in the middle of an uncompleted message
        // are allowed only for interleaved Real Time message or EOX.
        switch (EXTRACTED)
        {
        case static_cast<uint8_t>(messageType_t::SYS_REAL_TIME_CLOCK):
        case static_cast<uint8_t>(messageType_t::SYS_REAL_TIME_START):
        case static_cast<uint8_t>(messageType_t::SYS_REAL_TIME_CONTINUE):
        case static_cast<uint8_t>(messageType_t::SYS_REAL_TIME_STOP):
        case static_cast<uint8_t>(messageType_t::SYS_REAL_TIME_ACTIVE_SENSING):
        case static_cast<uint8_t>(messageType_t::SYS_REAL_TIME_SYSTEM_RESET):
        {
            // Here we will have to extract the one-byte message,
            // pass it to the structure for being read outside
            // the MIDI class, and recompose the message it was
            // interleaved into without killing the running status..
            // This is done by leaving the pending message as is,
            // it will be completed on next calls.
            _message.type    = static_cast<messageType_t>(EXTRACTED);
            _message.data1   = 0;
            _message.data2   = 0;
            _message.channel = 0;
            _message.length  = 1;
            _message.valid   = true;

            return true;
        }
        break;

        // end of sysex
        case 0xF7:
        {
            if (_message.sysexArray[0] == static_cast<uint8_t>(messageType_t::SYS_EX))
            {
                // store the last byte (EOX)
                _message.sysexArray[_pendingMessageIndex++] = 0xF7;
                _message.type                               = messageType_t::SYS_EX;

                // get length
                _message.data1   = 0;
                _message.data2   = 0;
                _message.channel = 0;
                _message.length  = _pendingMessageIndex;
                _message.valid   = true;

                reset();
                return true;
            }

            // error
            reset();
            return false;
        }
        break;

        // start of sysex
        case 0xF0:
        {
            // reset the parsing of sysex
            _message.sysexArray[0] = static_cast<uint8_t>(messageType_t::SYS_EX);
            _pendingMessageIndex   = 1;
        }
        break;

        default:
            break;
        }
    }

    // add extracted data byte to pending message
    if (_mPendingMessage[0] == static_cast<uint8_t>(messageType_t::SYS_EX))
    {
        _message.sysexArray[_pendingMessageIndex] = EXTRACTED;
    }
    else
    {
        _mPendingMessage[_pendingMessageIndex] = EXTRACTED;
    }

    // now we are going to check if we have reached the end of the message
    if (_pendingMessageIndex >= (_pendingMessageExpectedLength - 1))
    {
        //"FML" case: fall down here with an overflown SysEx..
        // This means we received the last possible data byte that can fit the buffer.
        // If this happens, try increasing MIDI_SYSEX_ARRAY_SIZE.
        if (_mPendingMessage[0] == static_cast<uint8_t>(messageType_t::SYS_EX))
        {
            reset();
            return false;
        }

        _message.type = TYPE_FROM_STATUS_BYTE(_mPendingMessage[0]);

        if (IS_CHANNEL_MESSAGE(_message.type))
        {
            _message.channel = CHANNEL_FROM_STATUS_BYTE(_mPendingMessage[0]);
        }
        else
        {
            _message.channel = 0;
        }

        _message.data1 = _mPendingMessage[1];

        // save data2 only if applicable
        if (_pendingMessageExpectedLength == 3)
        {
            _message.data2 = _mPendingMessage[2];
        }
        else
        {
            _message.data2 = 0;
        }

        _message.length = _pendingMessageExpectedLength;

        // reset local variables
        _pendingMessageIndex          = 0;
        _pendingMessageExpectedLength = 0;
        _message.valid                = true;

        // activate running status (if enabled for the received type)
        switch (_message.type)
        {
        case messageType_t::NOTE_OFF:
        case messageType_t::NOTE_ON:
        case messageType_t::AFTER_TOUCH_POLY:
        case messageType_t::CONTROL_CHANGE:
        case messageType_t::PROGRAM_CHANGE:
        case messageType_t::AFTER_TOUCH_CHANNEL:
        case messageType_t::PITCH_BEND:
        {
            // running status enabled: store it from received message
            _mRunningStatusRX = _mPendingMessage[0];
        }
        break;

        default:
        {
            // no running status
            _mRunningStatusRX = static_cast<uint8_t>(messageType_t::INVALID);
        }
        break;
        }

        return true;
    }

    // update the index of the pending message
    _pendingMessageIndex++;

    if (!_recursiveParseState)
    {
        return false;    // message is not complete.
    }

    return parse();
}

/// Retrieves the MIDI message type of the last received message.
messageType_t Base::type()
{
    return _message.type;
}

/// Retrieves the MIDI channel of the last received message.
uint8_t Base::channel()
{
    return _message.channel;
}

/// Retrieves the first data byte of the last received message.
uint8_t Base::data1()
{
    return _message.data1;
}

/// Retrieves the second data byte of the last received message.
uint8_t Base::data2()
{
    return _message.data2;
}

/// Retrieves memory location in which SysEx array is being stored.
uint8_t* Base::sysExArray()
{
    return _message.sysexArray;
}

/// Checks the size of last received MIDI message.
uint16_t Base::length()
{
    return _message.length;
}

/// Used to enable or disable recursive parsing of incoming messages on UART interface.
/// Setting this to false will make MIDI.read parse only one byte of data for each
/// call when data is available. This can speed up your application if receiving
/// a lot of traffic, but might induce MIDI Thru and treatment latency.
/// param state [in]   Set to true to enable recursive parsing or false to disable it.
void Base::useRecursiveParsing(bool state)
{
    _recursiveParseState = state;
}

void Base::thru()
{
    for (size_t i = 0; i < _thruInterface.size(); i++)
    {
        auto interface = _thruInterface.at(i);

        if (interface == nullptr)
        {
            continue;
        }

        if (interface->beginTransmission(_message.type))
        {
            if (IS_SYSTEM_REAL_TIME(_message.type))
            {
                interface->write(static_cast<uint8_t>(_message.type));
            }
            else if (IS_CHANNEL_MESSAGE(_message.type))
            {
                const auto IN_STATUS = status(_message.type, _message.channel);
                interface->write(static_cast<uint8_t>(IN_STATUS));

                if (_message.length > 1)
                {
                    interface->write(_message.data1);
                }

                if (_message.length > 2)
                {
                    interface->write(_message.data2);
                }
            }
            else if (_message.type == messageType_t::SYS_EX)
            {
                const auto SIZE = _message.length;

                for (size_t i = 0; i < SIZE; i++)
                {
                    interface->write(_message.sysexArray[i]);
                }
            }
            else    // at this point, it it assumed to be a system common message
            {
                interface->write(static_cast<uint8_t>(_message.type));

                if (_message.length > 1)
                {
                    interface->write(_message.data1);
                }

                if (_message.length > 2)
                {
                    interface->write(_message.data2);
                }
            }
        }

        interface->endTransmission();
    }
}

/// Configures how Note Off messages are sent.
/// param type [in]    Type of MIDI Note Off message. See noteOffType_t.
void Base::setNoteOffMode(noteOffType_t type)
{
    _noteOffMode = type;
}

/// Checks how MIDI Note Off messages are being sent.
noteOffType_t Base::noteOffMode()
{
    return _noteOffMode;
}

void Base::registerThruInterface(Thru& interface)
{
    for (size_t i = 0; i < _thruInterface.size(); i++)
    {
        if (_thruInterface.at(i) == nullptr)
        {
            _thruInterface[i] = &interface;
            break;
        }
    }
}

void Base::unregisterThruInterface(Thru& interface)
{
    for (size_t i = 0; i < _thruInterface.size(); i++)
    {
        if (_thruInterface.at(i) == &interface)
        {
            _thruInterface[i] = nullptr;
        }
    }
}

// return the last decoded midi message
Message& Base::message()
{
    return _message;
}