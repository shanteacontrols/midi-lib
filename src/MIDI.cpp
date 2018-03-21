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

#include "MIDI.h"

bool                usbEnabled,
                    dinEnabled;

bool                useRunningStatus;
bool                use1byteParsing;

uint8_t             mRunningStatus_RX,
                    mRunningStatus_TX;

uint8_t             mInputChannel;
uint8_t             mPendingMessage[3];
uint16_t            dinPendingMessageExpectedLenght;
uint16_t            dinPendingMessageIndex;
uint16_t            sysExArrayLength;

MIDImessage_t       dinMessage,
                    usbMessage;

noteOffType_t       noteOffMode;

int16_t             (*sendUARTreadCallback)();
int8_t              (*sendUARTwriteCallback)(uint8_t data);

bool                (*sendUSBreadCallback)(USBMIDIpacket_t& USBMIDIpacket);
bool                (*sendUSBwriteCallback)(USBMIDIpacket_t& USBMIDIpacket);

uint8_t             zeroStartChannel;
bool                dinValidityCheckStateThru;

///
/// \brief Default constructor.
///
MIDI::MIDI()
{
    sendUARTwriteCallback   = NULL;
    sendUARTreadCallback    = NULL;
    sendUSBwriteCallback    = NULL;
    sendUSBreadCallback     = NULL;
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
void MIDI::send(midiMessageType_t inType, uint8_t inData1, uint8_t inData2, uint8_t inChannel)
{
    bool validCheck = true;

    //test if channel is valid
    if (zeroStartChannel)
    {
        if (inChannel >= 16)
            validCheck = false;
        else
            inChannel++; //normalize channel
    }
    else
    {
        if ((inChannel > 16) || !inChannel)
            validCheck = false;
    }

    if (!validCheck || (inType < 0x80))
    {
        if (useRunningStatus && dinEnabled)
            mRunningStatus_TX = midiMessageInvalidType;

        return; //don't send anything
    }

    if (inType <= midiMessagePitchBend)
    {
        //channel messages
        //protection: remove MSBs on data
        inData1 &= 0x7f;
        inData2 &= 0x7f;

        const uint8_t status = getStatus(inType, inChannel);

        if (dinEnabled)
        {
            if (useRunningStatus)
            {
                if (mRunningStatus_TX != status)
                {
                    //new message, memorize and send header
                    mRunningStatus_TX = status;
                    sendUARTwriteCallback(mRunningStatus_TX);
                }
            }
            else
            {
                //don't care about running status, send the status byte
                sendUARTwriteCallback(status);
            }

            //send data
            sendUARTwriteCallback(inData1);

            if ((inType != midiMessageProgramChange) && (inType != midiMessageAfterTouchChannel))
            {
                sendUARTwriteCallback(inData2);
            }
        }

        if (usbEnabled)
        {
            uint8_t midiEvent = (uint8_t)inType >> 4;
            uint8_t data1 = getStatus(inType, inChannel);

            USBMIDIpacket_t MIDIEvent = (USBMIDIpacket_t)
            {
                .Event  = midiEvent,

                .Data1  = data1,
                .Data2  = inData1,
                .Data3  = inData2,
            };

            sendUSBwriteCallback(MIDIEvent);
        }
    }
    else if (inType >= midiMessageTuneRequest && inType <= midiMessageSystemReset)
    {
        sendRealTime(inType); //system real-time and 1 byte
    }
}

///
/// \brief Send a Note On message.
///
/// \param inNoteNumber [in]    Pitch value in the MIDI format (0 to 127).
/// \param inVelocity [in]      Note attack velocity (0 to 127).
/// \param inChannel [in]       The channel on which the message will be sent (1 to 16).
///
void MIDI::sendNoteOn(uint8_t inNoteNumber, uint8_t inVelocity, uint8_t inChannel)
{
    send(midiMessageNoteOn, inNoteNumber, inVelocity, inChannel);
}

///
/// \brief Send a Note Off message.
///
/// If note off mode is set to noteOffType_standardNoteOff, Note Off message will be sent.
/// If mode is set to noteOffType_noteOnZeroVel, Note On will be sent will velocity 0.
/// \param inNoteNumber [in]    Pitch value in the MIDI format (0 to 127).
/// \param inVelocity [in]      Release velocity (0 to 127).
/// \param inChannel [in]       The channel on which the message will be sent (1 to 16).
///
void MIDI::sendNoteOff(uint8_t inNoteNumber, uint8_t inVelocity, uint8_t inChannel)
{
    if (noteOffMode == noteOffType_standardNoteOff)
        send(midiMessageNoteOff, inNoteNumber, inVelocity, inChannel);
    else
        send(midiMessageNoteOn, inNoteNumber, inVelocity, inChannel);
}

///
/// \brief Send a Program Change message.
/// \param inProgramNumber [in]     The program to select (0 to 127).
/// \param inChannel [in]           The channel on which the message will be sent (1 to 16).
///
void MIDI::sendProgramChange(uint8_t inProgramNumber, uint8_t inChannel)
{
    send(midiMessageProgramChange, inProgramNumber, 0, inChannel);
}

///
/// \brief Send a Control Change message.
/// \param inControlNumber [in]     The controller number (0 to 127).
/// \param inControlValue [in]      The value for the specified controller (0 to 127).
/// \param inChannel [in]           The channel on which the message will be sent (1 to 16).
///
void MIDI::sendControlChange(uint8_t inControlNumber, uint8_t inControlValue, uint8_t inChannel)
{
    send(midiMessageControlChange, inControlNumber, inControlValue, inChannel);
}

///
/// \brief Send a Polyphonic AfterTouch message (applies to a specified note).
/// \param inPressure [in]    The amount of AfterTouch to apply (0 to 127).
/// \param inChannel [in]     The channel on which the message will be sent (1 to 16).
/// \param inNoteNumber [in]  The note to apply AfterTouch to (0 to 127).
///
void MIDI::sendAfterTouch(uint8_t inPressure, uint8_t inChannel, uint8_t inNoteNumber)
{
    send(midiMessageAfterTouchPoly, inNoteNumber, inPressure, inChannel);
}

///
/// \brief Send a Monophonic AfterTouch message (applies to all notes).
/// \param inPressure [in]    The amount of AfterTouch to apply (0 to 127).
/// \param inChannel [in]     The channel on which the message will be sent (1 to 16).
///
void MIDI::sendAfterTouch(uint8_t inPressure, uint8_t inChannel)
{
    send(midiMessageAfterTouchChannel, inPressure, 0, inChannel);
}

///
/// \brief Send a Pitch Bend message using a signed integer value.
/// \param inPitchValue [in]  The amount of bend to send between MIDI_PITCHBEND_MIN and MIDI_PITCHBEND_MAX.
/// \param inChannel [in]     The channel on which the message will be sent (1 to 16).
///
void MIDI::sendPitchBend(int16_t inPitchValue, uint8_t inChannel)
{
    const unsigned bend = inPitchValue - MIDI_PITCHBEND_MIN;
    send(midiMessagePitchBend, lowByte_7bit(bend), highByte_7bit(bend), inChannel);
}

///
/// \brief Send a System Exclusive message.
/// \param inLength [in]                    The size of the array to send.
/// \param inArray [in]                     The byte array containing the data to send
/// \param inArrayContainsBoundaries [in]   When set to 'true', 0xF0 & 0xF7 bytes (start & stop SysEx)
///                                         will not be sent and therefore must be included in the array.
///
void MIDI::sendSysEx(uint16_t inLength, const uint8_t* inArray, bool inArrayContainsBoundaries)
{
    if (dinEnabled && (sendUARTwriteCallback != NULL))
    {
        if (!inArrayContainsBoundaries)
            sendUARTwriteCallback(0xf0);

        for (uint16_t i=0; i<inLength; ++i)
            sendUARTwriteCallback(inArray[i]);

        if (!inArrayContainsBoundaries)
            sendUARTwriteCallback(0xf7);

        if (useRunningStatus)
            mRunningStatus_TX = midiMessageInvalidType;
    }

    if (usbEnabled)
    {
        USBMIDIpacket_t MIDIEvent;

        if (!inArrayContainsBoundaries)
        {
            //append sysex start (0xF0) and stop (0xF7) bytes to array
            bool firstByte = true;
            bool startSent = false;

            while (inLength > 3)
            {
                if (firstByte)
                {
                    MIDIEvent = (USBMIDIpacket_t)
                    {
                        .Event  = GET_USB_MIDI_EVENT(0, sysExStartCin),

                        .Data1  = midiMessageSystemExclusive,
                        .Data2  = inArray[0],
                        .Data3  = inArray[1],
                    };

                    sendUSBwriteCallback(MIDIEvent);

                    firstByte = false;
                    startSent = true;
                    inArray += 2;
                    inLength -= 2;
                }
                else
                {
                    MIDIEvent = (USBMIDIpacket_t)
                    {
                        .Event  = GET_USB_MIDI_EVENT(0, sysExStartCin),

                        .Data1  = inArray[0],
                        .Data2  = inArray[1],
                        .Data3  = inArray[2],
                    };

                    sendUSBwriteCallback(MIDIEvent);

                    inArray += 3;
                    inLength -= 3;
                }
            }

            if (inLength == 3)
            {
                if (startSent)
                {
                    MIDIEvent = (USBMIDIpacket_t)
                    {
                        .Event  = GET_USB_MIDI_EVENT(0, sysExStartCin),

                        .Data1  = inArray[0],
                        .Data2  = inArray[1],
                        .Data3  = inArray[2],
                    };

                    sendUSBwriteCallback(MIDIEvent);

                    MIDIEvent = (USBMIDIpacket_t)
                    {
                        .Event  = GET_USB_MIDI_EVENT(0, sysExStop1byteCin),

                        .Data1  = 0xF7,
                        .Data2  = 0,
                        .Data3  = 0,
                    };

                    sendUSBwriteCallback(MIDIEvent);
                }
                else
                {
                    MIDIEvent = (USBMIDIpacket_t)
                    {
                        .Event  = GET_USB_MIDI_EVENT(0, sysExStartCin),

                        .Data1  = midiMessageSystemExclusive,
                        .Data2  = inArray[0],
                        .Data3  = inArray[1],
                    };

                    sendUSBwriteCallback(MIDIEvent);

                    MIDIEvent = (USBMIDIpacket_t)
                    {
                        .Event  = GET_USB_MIDI_EVENT(0, sysExStop2byteCin),

                        .Data1  = inArray[2],
                        .Data2  = 0xF7,
                        .Data3  = 0,
                    };

                    sendUSBwriteCallback(MIDIEvent);
                }
            }
            else if (inLength == 2)
            {
                if (startSent)
                {
                    MIDIEvent = (USBMIDIpacket_t)
                    {
                        .Event  = GET_USB_MIDI_EVENT(0, sysExStop3byteCin),

                        .Data1  = inArray[0],
                        .Data2  = inArray[1],
                        .Data3  = 0xF7,
                    };

                    sendUSBwriteCallback(MIDIEvent);
                }
                else
                {
                    MIDIEvent = (USBMIDIpacket_t)
                    {
                        .Event  = GET_USB_MIDI_EVENT(0, sysExStartCin),

                        .Data1  = midiMessageSystemExclusive,
                        .Data2  = inArray[0],
                        .Data3  = inArray[1],
                    };

                    sendUSBwriteCallback(MIDIEvent);

                    MIDIEvent = (USBMIDIpacket_t)
                    {
                        .Event  = GET_USB_MIDI_EVENT(0, sysExStop1byteCin),

                        .Data1  = 0xF7,
                        .Data2  = 0,
                        .Data3  = 0,
                    };

                    sendUSBwriteCallback(MIDIEvent);
                }
            }
            else if (inLength == 1)
            {
                if (startSent)
                {
                    MIDIEvent = (USBMIDIpacket_t)
                    {
                        .Event  = GET_USB_MIDI_EVENT(0, sysExStop2byteCin),

                        .Data1  = inArray[0],
                        .Data2  = 0xF7,
                        .Data3  = 0,
                    };

                    sendUSBwriteCallback(MIDIEvent);
                }
                else
                {
                    MIDIEvent = (USBMIDIpacket_t)
                    {
                        .Event  = GET_USB_MIDI_EVENT(0, sysExStop3byteCin),

                        .Data1  = 0xF0,
                        .Data2  = inArray[0],
                        .Data3  = 0xF7,
                    };

                    sendUSBwriteCallback(MIDIEvent);
                }
            }
        }
        else
        {
            while (inLength > 3)
            {
                MIDIEvent = (USBMIDIpacket_t)
                {
                    .Event  = GET_USB_MIDI_EVENT(0, sysExStartCin),

                    .Data1  = inArray[0],
                    .Data2  = inArray[1],
                    .Data3  = inArray[2],
                };

                sendUSBwriteCallback(MIDIEvent);

                inArray += 3;
                inLength -= 3;
            }

            if (inLength == 3)
            {
                MIDIEvent = (USBMIDIpacket_t)
                {
                    .Event  = GET_USB_MIDI_EVENT(0, sysExStop3byteCin),

                    .Data1  = inArray[0],
                    .Data2  = inArray[1],
                    .Data3  = inArray[2],
                };

                sendUSBwriteCallback(MIDIEvent);
            }
            else if (inLength == 2)
            {
                MIDIEvent = (USBMIDIpacket_t)
                {
                    .Event  = GET_USB_MIDI_EVENT(0, sysExStop2byteCin),

                    .Data1  = inArray[0],
                    .Data2  = inArray[1],
                    .Data3  = 0,
                };

                sendUSBwriteCallback(MIDIEvent);
            }
            else if (inLength == 1)
            {
                MIDIEvent = (USBMIDIpacket_t)
                {
                    .Event  = GET_USB_MIDI_EVENT(0, sysExStop1byteCin),

                    .Data1  = inArray[0],
                    .Data2  = 0,
                    .Data3  = 0,
                };

                sendUSBwriteCallback(MIDIEvent);
            }
        }
    }
}

///
/// \brief Send a Tune Request message.
///
/// When a MIDI unit receives this message, it should tune its oscillators (if equipped with any).
///
void MIDI::sendTuneRequest()
{
    sendUARTwriteCallback((uint8_t)midiMessageTuneRequest);
    if (useRunningStatus && dinEnabled)
        mRunningStatus_TX = midiMessageInvalidType;
}

///
/// \brief Send a MIDI Time Code Quarter Frame.
/// \param inTypeNibble [in]    MTC type.
/// \param inValuesNibble [in]  MTC data.
/// See MIDI Specification for more information.
///
void MIDI::sendTimeCodeQuarterFrame(uint8_t inTypeNibble, uint8_t inValuesNibble)
{
    uint8_t data = (((inTypeNibble & 0x07) << 4) | (inValuesNibble & 0x0f));
    sendTimeCodeQuarterFrame(data);
}

///
/// \brief Send a MIDI Time Code Quarter Frame.
/// \param inData [in]  If you want to encode directly the nibbles in your program,
///                     you can send the byte here.
///
void MIDI::sendTimeCodeQuarterFrame(uint8_t inData)
{
    sendUARTwriteCallback((uint8_t)midiMessageTimeCodeQuarterFrame);
    sendUARTwriteCallback(inData);

    if (useRunningStatus)
        mRunningStatus_TX = midiMessageInvalidType;
}

///
/// \brief Send a Song Position Pointer message.
/// \param inBeats [in]     The number of beats since the start of the song.
///
void MIDI::sendSongPosition(uint16_t inBeats)
{
    sendUARTwriteCallback((uint8_t)midiMessageSongPosition);
    sendUARTwriteCallback(inBeats & 0x7f);
    sendUARTwriteCallback((inBeats >> 7) & 0x7f);

    if (useRunningStatus)
        mRunningStatus_TX = midiMessageInvalidType;
}

///
/// \brief Send a Song Select message.
/// \param inSongNumber [in]    Song to select (0-127).
///
void MIDI::sendSongSelect(uint8_t inSongNumber)
{
    sendUARTwriteCallback((uint8_t)midiMessageSongSelect);
    sendUARTwriteCallback(inSongNumber & 0x7f);

    if (useRunningStatus)
        mRunningStatus_TX = midiMessageInvalidType;
}

///
/// \brief Send a Real Time (one byte) message.
/// \param inType [in]  The available Real Time types are:
///                     Start, Stop, Continue, Clock, ActiveSensing and SystemReset.
///
void MIDI::sendRealTime(midiMessageType_t inType)
{
    switch (inType)
    {
        case midiMessageClock:
        case midiMessageStart:
        case midiMessageStop:
        case midiMessageContinue:
        case midiMessageActiveSensing:
        case midiMessageSystemReset:
        if (dinEnabled)
            sendUARTwriteCallback((uint8_t)inType);

        if (usbEnabled)
        {
            USBMIDIpacket_t MIDIEvent = (USBMIDIpacket_t)
            {
                .Event  = (uint8_t)midiMessageSystemExclusive >> 4,

                .Data1  = inType,
                .Data2  = 0x00,
                .Data3  = 0x00,
            };

            sendUSBwriteCallback(MIDIEvent);
        }
        break;

        default:
        //invalid Real Time marker
        break;
    }
}

///
/// \brief Enable or disable running status.
///
/// Applies to DIN MIDI only (outgoing messages).
/// \param [in] state   True when enabling running status, false otherwise.
///
void MIDI::setRunningStatusState(bool state)
{
    useRunningStatus = state;
}

///
/// \brief Returns current running status state for outgoing DIN MIDI messages.
/// \returns True if running status is enabled, false otherwise.
///
bool MIDI::getRunningStatusState()
{
    return useRunningStatus;
}

///
/// \brief Calculates MIDI status byte for a given message type and channel.
/// \param inType [in]      MIDI message type.
/// \param inChannel [in]   MIDI channel.
/// \returns Calculated status byte.
///
uint8_t MIDI::getStatus(midiMessageType_t inType, uint8_t inChannel)
{
    return ((uint8_t)inType | ((inChannel - 1) & 0x0f));
}

///
/// \brief Reads UART or USB and tries to parse MIDI message.
///
/// A valid message is a message that matches the input channel.
/// If the Thru is enabled and the message matches the filter,
/// it is sent back on the MIDI output.
/// \param type [in]        MIDI interface which is being read (USB or UART). See midiInterfaceType_t.
/// \param filterMode [in]  Thru filter mode. See midiFilterMode_t.
///
void MIDI::read(midiInterfaceType_t type, midiFilterMode_t filterMode)
{
    if (mInputChannel == MIDI_CHANNEL_OFF)
        return; //MIDI Input disabled

    switch(type)
    {
        case usbInterface:
        if (!usbEnabled)
            return;
        break;

        case dinInterface:
        if (!dinEnabled)
            return;
        break;
    }

    if (!dinValidityCheckStateThru && (filterMode == THRU_FULL_DIN) || (filterMode == THRU_CHANNEL_DIN)
    {
        //just pass data directly without checking
        int16_t data = sendUARTreadCallback();

        if (data != -1)
            sendUARTwriteCallback(data);
    }
    else
    {
        if (!parse(type))
            return;

        const bool channelMatch = inputFilter(mInputChannel, type);
        thruFilter(mInputChannel, type, filterMode);
    }
}

///
/// \brief Handles parsing of MIDI messages.
/// \param type [in] MIDI interface from which messages are being parsed.
///
bool MIDI::parse(midiInterfaceType_t type)
{
    if (type == dinInterface)
    {
        if (sendUARTreadCallback == NULL)
            return false;

        int16_t data = sendUARTreadCallback();

        if (data == -1)
            return false;   //no data available

        const uint8_t extracted = data;

        if (dinPendingMessageIndex == 0)
        {
            //start a new pending message
            mPendingMessage[0] = extracted;

            //check for running status first (din only)
            if (isChannelMessage(getTypeFromStatusByte(mRunningStatus_RX)))
            {
                //only channel messages allow Running Status
                //if the status byte is not received, prepend it to the pending message
                if (extracted < 0x80)
                {
                    mPendingMessage[0]   = mRunningStatus_RX;
                    mPendingMessage[1]   = extracted;
                    dinPendingMessageIndex = 1;
                }

                //else: well, we received another status byte,
                //so the running status does not apply here
                //it will be updated upon completion of this message
            }

            switch (getTypeFromStatusByte(mPendingMessage[0]))
            {
                //1 byte messages
                case midiMessageStart:
                case midiMessageContinue:
                case midiMessageStop:
                case midiMessageClock:
                case midiMessageActiveSensing:
                case midiMessageSystemReset:
                case midiMessageTuneRequest:
                //handle the message type directly here.
                dinMessage.type    = getTypeFromStatusByte(mPendingMessage[0]);
                dinMessage.channel = 0;
                dinMessage.data1   = 0;
                dinMessage.data2   = 0;
                dinMessage.valid   = true;

                // \fix Running Status broken when receiving Clock messages.
                // Do not reset all input attributes, Running Status must remain unchanged.
                //resetInput();

                //we still need to reset these
                dinPendingMessageIndex = 0;
                dinPendingMessageExpectedLenght = 0;

                return true;
                break;

                //2 bytes messages
                case midiMessageProgramChange:
                case midiMessageAfterTouchChannel:
                case midiMessageTimeCodeQuarterFrame:
                case midiMessageSongSelect:
                dinPendingMessageExpectedLenght = 2;
                break;

                //3 bytes messages
                case midiMessageNoteOn:
                case midiMessageNoteOff:
                case midiMessageControlChange:
                case midiMessagePitchBend:
                case midiMessageAfterTouchPoly:
                case midiMessageSongPosition:
                dinPendingMessageExpectedLenght = 3;
                break;

                case midiMessageSystemExclusive:
                //the message can be any length between 3 and MIDI_SYSEX_ARRAY_SIZE
                dinPendingMessageExpectedLenght = MIDI_SYSEX_ARRAY_SIZE;
                mRunningStatus_RX = midiMessageInvalidType;
                dinMessage.sysexArray[0] = midiMessageSystemExclusive;
                break;

                case midiMessageInvalidType:
                default:
                //this is obviously wrong
                //let's get the hell out'a here
                resetInput();
                return false;
                break;
            }

            if (dinPendingMessageIndex >= (dinPendingMessageExpectedLenght - 1))
            {
                //reception complete
                dinMessage.type    = getTypeFromStatusByte(mPendingMessage[0]);
                dinMessage.channel = getChannelFromStatusByte(mPendingMessage[0]);
                dinMessage.data1   = mPendingMessage[1];

                //save data2 only if applicable
                if (dinPendingMessageExpectedLenght == 3)
                dinMessage.data2 = mPendingMessage[2];
                else dinMessage.data2 = 0;

                dinPendingMessageIndex = 0;
                dinPendingMessageExpectedLenght = 0;
                dinMessage.valid = true;
                return true;
            }
            else
            {
                //waiting for more data
                dinPendingMessageIndex++;
            }

            if (use1byteParsing)
            {
                //message is not complete.
                return false;
            }
            else
            {
                //call the parser recursively
                //to parse the rest of the message.
                return parse(dinInterface);
            }
        }
        else
        {
            //first, test if this is a status byte
            if (extracted >= 0x80)
            {
                //reception of status bytes in the middle of an uncompleted message
                //are allowed only for interleaved Real Time message or EOX
                switch (extracted)
                {
                    case midiMessageClock:
                    case midiMessageStart:
                    case midiMessageContinue:
                    case midiMessageStop:
                    case midiMessageActiveSensing:
                    case midiMessageSystemReset:
                    //here we will have to extract the one-byte message,
                    //pass it to the structure for being read outside
                    //the MIDI class, and recompose the message it was
                    //interleaved into without killing the running status..
                    //this is done by leaving the pending message as is,
                    //it will be completed on next calls
                    dinMessage.type    = (midiMessageType_t)extracted;
                    dinMessage.data1   = 0;
                    dinMessage.data2   = 0;
                    dinMessage.channel = 0;
                    dinMessage.valid   = true;
                    return true;
                    break;

                    //end of sysex
                    case 0xf7:
                    if (dinMessage.sysexArray[0] == midiMessageSystemExclusive)
                    {
                        //store the last byte (EOX)
                        dinMessage.sysexArray[dinPendingMessageIndex++] = 0xf7;
                        dinMessage.type = midiMessageSystemExclusive;

                        //get length
                        dinMessage.data1   = dinPendingMessageIndex & 0xff; //LSB
                        dinMessage.data2   = dinPendingMessageIndex >> 8;   //MSB
                        dinMessage.channel = 0;
                        dinMessage.valid   = true;

                        resetInput();
                        return true;
                    }
                    else
                    {
                        //error
                        resetInput();
                        return false;
                    }
                    break;

                    default:
                    break;
                }
            }

            //add extracted data byte to pending message
            if (mPendingMessage[0] == midiMessageSystemExclusive)
                dinMessage.sysexArray[dinPendingMessageIndex] = extracted;
            else
                mPendingMessage[dinPendingMessageIndex] = extracted;

            //now we are going to check if we have reached the end of the message
            if (dinPendingMessageIndex >= (dinPendingMessageExpectedLenght - 1))
            {
                //"FML" case: fall down here with an overflown SysEx..
                //this means we received the last possible data byte that can fit the buffer
                //if this happens, try increasing MIDI_SYSEX_ARRAY_SIZE
                if (mPendingMessage[0] == midiMessageSystemExclusive)
                {
                    resetInput();
                    return false;
                }

                dinMessage.type = getTypeFromStatusByte(mPendingMessage[0]);

                if (isChannelMessage(dinMessage.type))
                    dinMessage.channel = getChannelFromStatusByte(mPendingMessage[0]);
                else
                    dinMessage.channel = 0;

                dinMessage.data1 = mPendingMessage[1];

                //save data2 only if applicable
                if (dinPendingMessageExpectedLenght == 3)
                    dinMessage.data2 = mPendingMessage[2];
                else dinMessage.data2 = 0;

                //reset local variables
                dinPendingMessageIndex = 0;
                dinPendingMessageExpectedLenght = 0;

                dinMessage.valid = true;

                //activate running status (if enabled for the received type)
                switch (dinMessage.type)
                {
                    case midiMessageNoteOff:
                    case midiMessageNoteOn:
                    case midiMessageAfterTouchPoly:
                    case midiMessageControlChange:
                    case midiMessageProgramChange:
                    case midiMessageAfterTouchChannel:
                    case midiMessagePitchBend:
                    //running status enabled: store it from received message
                    mRunningStatus_RX = mPendingMessage[0];
                    break;

                    default:
                    //no running status
                    mRunningStatus_RX = midiMessageInvalidType;
                    break;
                }

                return true;
            }
            else
            {
                //update the index of the pending message
                dinPendingMessageIndex++;

                if (use1byteParsing)
                    return false;   //message is not complete.
                else
                    return parse(dinInterface); //call the parser recursively to parse the rest of the message.
            }
        }
    }
    else if (type == usbInterface)
    {
        USBMIDIpacket_t USBMIDIpacket;

        if (!sendUSBreadCallback(USBMIDIpacket))
            return false; //nothing received

        //we already have entire message here
        //MIDIEvent.Event is CIN, see midi10.pdf
        //shift cin four bytes left to get midiMessageType_t
        uint8_t midiMessage = USBMIDIpacket.Event << 4;

        switch(midiMessage)
        {
            //1 byte messages
            case sysCommon1byteCin:
            if (USBMIDIpacket.Data1 != 0xF7)
            {
                //this isn't end of sysex, it's 1byte system common message

                //case midiMessageClock:
                //case midiMessageStart:
                //case midiMessageContinue:
                //case midiMessageStop:
                //case midiMessageActiveSensing:
                //case midiMessageSystemReset:
                usbMessage.type    = (midiMessageType_t)USBMIDIpacket.Data1;
                usbMessage.channel = 0;
                usbMessage.data1   = 0;
                usbMessage.data2   = 0;
                usbMessage.valid   = true;
                return true;
            }
            else
            {
                //end of sysex
                usbMessage.sysexArray[sysExArrayLength] = USBMIDIpacket.Data1;
                sysExArrayLength++;
                usbMessage.type    = (midiMessageType_t)midiMessageSystemExclusive;
                usbMessage.channel = 0;
                usbMessage.valid   = true;
                return true;
            }
            break;

            //2 byte messages
            case sysCommon2byteCin:
            //case midiMessageProgramChange:
            //case midiMessageAfterTouchChannel:
            //case midiMessageTimeCodeQuarterFrame:
            //case midiMessageSongSelect:
            usbMessage.type    = (midiMessageType_t)USBMIDIpacket.Data1;
            usbMessage.channel = (USBMIDIpacket.Data1 & 0x0F) + 1;
            usbMessage.data1   = USBMIDIpacket.Data2;
            usbMessage.data2   = 0;
            usbMessage.valid   = true;
            return true;
            break;

            //3 byte messages
            case midiMessageNoteOn:
            case midiMessageNoteOff:
            case midiMessageControlChange:
            case midiMessagePitchBend:
            case midiMessageAfterTouchPoly:
            case midiMessageSongPosition:
            usbMessage.type    = (midiMessageType_t)midiMessage;
            usbMessage.channel = (USBMIDIpacket.Data1 & 0x0F) + 1;
            usbMessage.data1   = USBMIDIpacket.Data2;
            usbMessage.data2   = USBMIDIpacket.Data3;
            usbMessage.valid   = true;
            return true;
            break;

            //sysex
            case sysExStartCin:
            //the message can be any length between 3 and MIDI_SYSEX_ARRAY_SIZE
            if (USBMIDIpacket.Data1 == 0xF0)
                sysExArrayLength = 0;   //this is a new sysex message, reset length

            usbMessage.sysexArray[sysExArrayLength] = USBMIDIpacket.Data1;
            sysExArrayLength++;
            usbMessage.sysexArray[sysExArrayLength] = USBMIDIpacket.Data2;
            sysExArrayLength++;
            usbMessage.sysexArray[sysExArrayLength] = USBMIDIpacket.Data3;
            sysExArrayLength++;
            return false;
            break;

            case sysExStop2byteCin:
            usbMessage.sysexArray[sysExArrayLength] = USBMIDIpacket.Data1;
            sysExArrayLength++;
            usbMessage.sysexArray[sysExArrayLength] = USBMIDIpacket.Data2;
            sysExArrayLength++;
            usbMessage.type    = midiMessageSystemExclusive;
            usbMessage.channel = 0;
            usbMessage.valid   = true;
            return true;
            break;

            case sysExStop3byteCin:
            if (USBMIDIpacket.Data1 == 0xF0)
                sysExArrayLength = 0; //sysex message with 1 byte of payload
            usbMessage.sysexArray[sysExArrayLength] = USBMIDIpacket.Data1;
            sysExArrayLength++;
            usbMessage.sysexArray[sysExArrayLength] = USBMIDIpacket.Data2;
            sysExArrayLength++;
            usbMessage.sysexArray[sysExArrayLength] = USBMIDIpacket.Data3;
            sysExArrayLength++;
            usbMessage.type    = midiMessageSystemExclusive;
            usbMessage.channel = 0;
            usbMessage.valid   = true;
            return true;
            break;

            default:
            return false;
            break;
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
bool MIDI::inputFilter(uint8_t inChannel, midiInterfaceType_t type)
{
    switch(type)
    {
        case dinInterface:
        if (dinMessage.type == midiMessageInvalidType)
            return false;

        //first, check if the received message is Channel
        if (dinMessage.type >= midiMessageNoteOff && dinMessage.type <= midiMessagePitchBend)
        {
            //then we need to know if we listen to it
            if ((dinMessage.channel == inChannel) || (inChannel == MIDI_CHANNEL_OMNI))
                return true;
            else
                return false;   //we don't listen to this channel

        }
        else
        {
            //system messages are always received
            return true;
        }
        break;

        case usbInterface:
        if (usbMessage.type == midiMessageInvalidType)
            return false;

        //first, check if the received message is Channel
        if (usbMessage.type >= midiMessageNoteOff && usbMessage.type <= midiMessagePitchBend)
        {
            //then we need to know if we listen to it
            if ((usbMessage.channel == inChannel) || (inChannel == MIDI_CHANNEL_OMNI))
                return true;
            else
                return false;   //we don't listen to this channel

        }
        else
        {
            //system messages are always received
            return true;
        }
        break;

        default:
        return false;
        break;
    }
}

///
/// \brief Reset attributes for incoming message (DIN MIDI only).
///
void MIDI::resetInput()
{
    dinPendingMessageIndex = 0;
    dinPendingMessageExpectedLenght = 0;
    mRunningStatus_RX = midiMessageInvalidType;
}

///
/// \brief Retrieves the MIDI message type of the last received message.
/// \param type [in]    MIDI interface which is being checked.
/// \returns MIDI message type of the last received message.
///
midiMessageType_t MIDI::getType(midiInterfaceType_t type)
{
    //get the last received message's type
    switch(type)
    {
        case dinInterface:
        return dinMessage.type;
        break;

        case usbInterface:
        return usbMessage.type;
        break;
    }

    return midiMessageInvalidType;
}

///
/// \brief Retrieves the MIDI channel of the last received message.
/// \param type [in]    MIDI interface which is being checked.
/// \returns MIDI channel of the last received message. For non-channel messages, this will return 0.
///
uint8_t MIDI::getChannel(midiInterfaceType_t type)
{
    switch(type)
    {
        case dinInterface:
        if (dinMessage.channel)
            return dinMessage.channel - zeroStartChannel;
        break;

        case usbInterface:
        if (usbMessage.channel)
            return usbMessage.channel - zeroStartChannel;
        break;
    }

    return MIDI_CHANNEL_INVALID;
}

///
/// \brief Retrieves the first data byte of the last received message.
/// \param type [in]    MIDI interface which is being checked.
/// \returns First data byte of the last received message.
///
uint8_t MIDI::getData1(midiInterfaceType_t type)
{
    switch(type)
    {
        case dinInterface:
        return dinMessage.data1;
        break;

        case usbInterface:
        return usbMessage.data1;
        break;

        default:
        return 0;
    }
}

///
/// \brief Retrieves the second data byte of the last received message.
/// \param type [in]    MIDI interface which is being checked.
/// \returns Second data byte of the last received message.
///
uint8_t MIDI::getData2(midiInterfaceType_t type)
{
    switch(type)
    {
        case dinInterface:
        return dinMessage.data2;
        break;

        case usbInterface:
        return usbMessage.data2;
        break;
    }

    return 0;
}

///
/// \brief Retrieves memory location in which SysEx array is being stored.
/// \param [in] type    MIDI interface from which SysEx array is located.
/// \returns Pointer to SysEx array.
///
uint8_t* MIDI::getSysExArray(midiInterfaceType_t type)
{
    //get the System Exclusive byte array
    switch(type)
    {
        case dinInterface:
        return dinMessage.sysexArray;
        break;

        case usbInterface:
        return usbMessage.sysexArray;
        break;
    }

    return 0;
}

///
/// \brief Checks size of SysEx array.
/// \param [in] type    MIDI interface on which SysEx size is being checked.
/// \returns Size of SysEx array on given MIDI interface in bytes.
///
uint16_t MIDI::getSysExArrayLength(midiInterfaceType_t type)
{
    uint16_t size = 0;

    switch(type)
    {
        case dinInterface:
        size = unsigned(dinMessage.data2) << 8 | dinMessage.data1;
        break;

        case usbInterface:
        return sysExArrayLength;
        break;
    }

    return size > MIDI_SYSEX_ARRAY_SIZE ? MIDI_SYSEX_ARRAY_SIZE : size;
}

///
/// \brief Checks MIDI channel on which incoming messages are being listened.
/// \returns MIDI channel value (1-16) if zeroStartChannel is disabled, 0-15 otherwise.
///          Two additional values can be returned (MIDI_CHANNEL_OMNI and MIDI_CHANNEL_OFF).
///
uint8_t MIDI::getInputChannel()
{
    if ((mInputChannel == MIDI_CHANNEL_OMNI) || (mInputChannel == MIDI_CHANNEL_OFF))
    {
        return mInputChannel;
    }
    else
    {
        return mInputChannel - zeroStartChannel;
    }
}

///
/// \brief Configures MIDI channel on which incoming messages are being listened.
/// \param inChannel [in]   The channel value. Valid values are 1 to 16 if zeroStartChannel
///                         is set to false, otherwise valid values are 0-15. Two additional values
///                         can be passed:
///                         MIDI_CHANNEL_OMNI value is used to listen on all channels (default).
///                         MIDI_CHANNEL_OFF value is used to disable input.
/// \returns                True if passed channel is valid.
///
bool MIDI::setInputChannel(uint8_t inChannel)
{
    bool valid = false;

    if ((inChannel == MIDI_CHANNEL_OMNI) || (inChannel == MIDI_CHANNEL_OFF))
    {
        valid = true;
    }
    else
    {
        if (zeroStartChannel)
        {
            if (mInputChannel >= 16)
                valid = false;
        }
        else
        {
            if (!mInputChannel || (mInputChannel > 16))
                valid = false;
        }
    }

    if (valid)
    {
        mInputChannel = inChannel;
        return true;
    }

    return false;
}

///
/// \brief Extract MIDI message type from status byte.
/// \param inStatus [in]    Status byte.
/// \returns Extracted MIDI message type.
///
midiMessageType_t MIDI::getTypeFromStatusByte(uint8_t inStatus)
{
    //extract an enumerated MIDI type from a status byte

    if ((inStatus  < 0x80) ||
        (inStatus == 0xf4) ||
        (inStatus == 0xf5) ||
        (inStatus == 0xf9) ||
        (inStatus == 0xfD))
    {
        //data bytes and undefined
        return midiMessageInvalidType;
    }

    if (inStatus < 0xf0)
    {
        //channel message, remove channel nibble
        return midiMessageType_t(inStatus & 0xf0);
    }

    return midiMessageType_t(inStatus);
}

///
/// \brief Extract MIDI channel from status byte.
/// \param inStatus [in]    Status byte.
/// \returns Extracted MIDI channel.
///
uint8_t MIDI::getChannelFromStatusByte(uint8_t inStatus)
{
    return (inStatus & 0x0f) + 1 - zeroStartChannel;
}

///
/// \brief Checks if MIDI message is channel message.
/// \param inType [in]  Type of MIDI message which is being checked.
/// \returns True if requested MIDI message type is channel type, false otherwise.
///
bool MIDI::isChannelMessage(midiMessageType_t inType)
{
    return (inType == midiMessageNoteOff           ||
            inType == midiMessageNoteOn            ||
            inType == midiMessageControlChange     ||
            inType == midiMessageAfterTouchPoly    ||
            inType == midiMessageAfterTouchChannel ||
            inType == midiMessagePitchBend         ||
            inType == midiMessageProgramChange);
}

///
/// \brief Used to enable or disable one byte parsing of incoming messages on UART.
///
/// Setting this to true will make MIDI.read parse only one byte of data for each
/// call when data is available. This can speed up your application if receiving
/// a lot of traffic, but might induce MIDI Thru and treatment latency.
/// \param state [in]   Set to true to enable one byte parsing or false to disable it.
///
void MIDI::setOneByteParseDINstate(bool state)
{
    use1byteParsing = state;
}

///
/// \brief Checks current status of one-byte parsing for incoming UART messages.
/// \returns True if one byte parsing is enabled, false otherwise.
///
bool MIDI::getOneByteParseDINstate()
{
    return use1byteParsing;
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
void MIDI::thruFilter(uint8_t inChannel, midiInterfaceType_t type, midiFilterMode_t filterMode)
{
    //if the feature is disabled, don't do anything.
    if (filterMode == THRU_OFF)
        return;

    MIDImessage_t *msg;
    msg = type == dinInterface ? &dinMessage : &usbMessage;

    bool savedDINstate = dinEnabled;
    bool savedUSBstate = usbEnabled;

    switch(filterMode)
    {
        case THRU_FULL_DIN:
        case THRU_CHANNEL_DIN:
        setUSBMIDIstate(false);
        break;

        case THRU_FULL_USB:
        case THRU_CHANNEL_USB:
        setDINMIDIstate(false);
        break;

        case THRU_FULL_ALL:
        case THRU_CHANNEL_ALL:
        //leave as is
        break;

        default:
        return;
    }

    //first, check if the received message is Channel
    if ((*msg).type >= midiMessageNoteOff && (*msg).type <= midiMessagePitchBend)
    {
        const bool filter_condition = (((*msg).channel == mInputChannel) ||
                                       (mInputChannel == MIDI_CHANNEL_OMNI));

        //now let's pass it to the output
        switch (filterMode)
        {
            case THRU_FULL_DIN:
            case THRU_FULL_USB:
            case THRU_FULL_ALL:
            send((*msg).type, (*msg).data1, (*msg).data2, (*msg).channel);
            break;

            case THRU_CHANNEL_DIN:
            case THRU_CHANNEL_USB:
            case THRU_CHANNEL_ALL:
            if (filter_condition)
            {
                send((*msg).type, (*msg).data1, (*msg).data2, (*msg).channel);
            }
            break;

            default:
            break;
        }
    }
    else
    {
        //always forward system messages
        switch ((*msg).type)
        {
            //real Time and 1 byte
            case midiMessageClock:
            case midiMessageStart:
            case midiMessageStop:
            case midiMessageContinue:
            case midiMessageActiveSensing:
            case midiMessageSystemReset:
            case midiMessageTuneRequest:
            sendRealTime((*msg).type);
            break;

            case midiMessageSystemExclusive:
            //send SysEx (0xf0 and 0xf7 are included in the buffer)
            sendSysEx(getSysExArrayLength(type), getSysExArray(type), true);
            break;

            case midiMessageSongSelect:
            sendSongSelect((*msg).data1);
            break;

            case midiMessageSongPosition:
            sendSongPosition((*msg).data1 | ((unsigned)(*msg).data2 << 7));
            break;

            case midiMessageTimeCodeQuarterFrame:
            sendTimeCodeQuarterFrame((*msg).data1, (*msg).data2);
            break;

            default:
            break;
        }
    }

    setUSBMIDIstate(savedUSBstate);
    setDINMIDIstate(savedDINstate);
}

///
/// \brief Configures how Note Off messages are sent.
/// \param type [in]    Type of MIDI Note Off message. See noteOffType_t.
///
void MIDI::setNoteOffMode(noteOffType_t type)
{
    noteOffMode = type;
}

///
/// \brief Checks how MIDI Note Off messages are being sent.
///
noteOffType_t MIDI::getNoteOffMode()
{
    return noteOffMode;
}

///
/// \brief Enables or disables USB MIDI interface.
/// \param state [in] New state of USB MIDI interface (true/enabled, false/disabled).
///
void MIDI::setUSBMIDIstate(bool state)
{
    usbEnabled = state;
}

///
/// \brief Checks current state of USB MIDI interface.
/// \returns True if USB MIDI interface is enabled, false otherwise.
///
bool MIDI::getUSBMIDIstate()
{
    return usbEnabled;
}

///
/// \brief Enables or disables DIN MIDI interface.
/// \param state [in] New state of DIN MIDI interface (true/enabled, false/disabled).
///
void MIDI::setDINMIDIstate(bool state)
{
    dinEnabled = state;
}

///
/// \brief Checks current state of DIN MIDI interface.
/// \returns True if DIN MIDI interface is enabled, false otherwise.
///
bool MIDI::getDINMIDIstate()
{
    return dinEnabled;
}

///
/// \brief Calculates octave from received MIDI note.
/// @param [in] note    Raw MIDI note (0-127).
/// \returns Calculated octave (0 - MIDI_NOTES-1).
///
uint8_t MIDI::getOctaveFromNote(int8_t note)
{
    //sanitize input
    note &= 0x7F;

    return note / MIDI_NOTES;
}

///
/// \brief Calculates tonic (root note) from received raw MIDI note.
/// @param [in] note    Raw MIDI note (0-127).
/// \returns Calculated tonic/root note (enumerated type). See note_t enumeration.
///
note_t MIDI::getTonicFromNote(int8_t note)
{
    //sanitize input
    note &= 0x7F;

    return (note_t)(note % MIDI_NOTES);
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
void MIDI::setChannelSendZeroStart(bool state)
{
    zeroStartChannel = state ? 1 : 0;
}

///
/// \brief Enables or disables validity checks for incoming DIN MIDI traffic during thruing to DIN MIDI out.
/// When disabled, all received DIN MIDI traffic will be forwarded to DIN MIDI out (if thruing to that
/// interface is enabled) directly without parsing MIDI message first.
///
void MIDI::setDINvalidityCheckState(bool state)
{
    dinValidityCheckStateThru = state;
}

///
/// \brief Used to configure function which reads data from UART.
///
/// Value -1 must be returned if there is nothing to be read from UART.
/// \param fptr [in]    Pointer to function.
///
void MIDI::handleUARTread(int16_t(*fptr)())
{
    sendUARTreadCallback = fptr;
}

///
/// \brief Used to configure function which writes data to UART.
///
/// Value -1 must be returned if writing to UART has failed.
/// \param fptr [in]    Pointer to function.
///
void MIDI::handleUARTwrite(int8_t(*fptr)(uint8_t data))
{
    sendUARTwriteCallback = fptr;
}

///
/// \brief Used to configure function which reads data from USB.
///
/// Handler function receives pointer to USBMIDIpacket_t structure.
/// Elements of that structure must be filled with correct data.
/// If there is nothing to read, false should be returned, or true otherwise.
/// \param fptr [in]    Pointer to function.
///
void MIDI::handleUSBread(bool(*fptr)(USBMIDIpacket_t& USBMIDIpacket))
{
    sendUSBreadCallback = fptr;
}

///
/// \brief Used to configure function which writes data to USB.
///
/// Handler function receives pointer to USBMIDIpacket_t structure.
/// Elements of that structure must be sent to USB interface.
/// If writing has failed, false must be returned, or true otherwise.
/// \param fptr [in]    Pointer to function.
///
void MIDI::handleUSBwrite(bool(*fptr)(USBMIDIpacket_t& USBMIDIpacket))
{
    sendUSBwriteCallback = fptr;
}
