/*
    Copyright 2016 Francois Best
    Copyright 2017-2021 Igor Petrovic

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

#include "common.h"

namespace lib::midi
{
    class Base
    {
        public:
        Base(Transport& transport)
            : _transport(transport)
        {}

        bool          init();
        bool          deInit();
        bool          initialized();
        void          reset();
        Transport&    transport();
        bool          sendNoteOn(uint8_t inNoteNumber, uint8_t inVelocity, uint8_t inChannel);
        bool          sendNoteOff(uint8_t inNoteNumber, uint8_t inVelocity, uint8_t inChannel);
        bool          sendProgramChange(uint8_t inProgramNumber, uint8_t inChannel);
        bool          sendControlChange(uint8_t inControlNumber, uint8_t inControlValue, uint8_t inChannel);
        bool          sendControlChange14bit(uint16_t inControlNumber, uint16_t inControlValue, uint8_t inChannel);
        bool          sendPitchBend(uint16_t inPitchValue, uint8_t inChannel);
        bool          sendAfterTouch(uint8_t inPressure, uint8_t inChannel, uint8_t inNoteNumber);
        bool          sendAfterTouch(uint8_t inPressure, uint8_t inChannel);
        bool          sendSysEx(uint16_t inLength, const uint8_t* inArray, bool inArrayContainsBoundaries);
        bool          sendTimeCodeQuarterFrame(uint8_t inTypeNibble, uint8_t inValuesNibble);
        bool          sendTimeCodeQuarterFrame(uint8_t inData);
        bool          sendSongPosition(uint16_t inBeats);
        bool          sendSongSelect(uint8_t inSongNumber);
        bool          sendTuneRequest();
        bool          sendCommon(messageType_t inType, uint8_t inData1 = 0);
        bool          sendRealTime(messageType_t inType);
        bool          sendMMC(uint8_t deviceID, messageType_t mmc);
        bool          sendNRPN(uint16_t inParameterNumber, uint16_t inValue, uint8_t inChannel, bool value14bit = false);
        bool          send(messageType_t inType, uint8_t inData1, uint8_t inData2, uint8_t inChannel);
        bool          read();
        bool          parse();
        void          useRecursiveParsing(bool state);
        bool          runningStatusState();
        noteOffType_t noteOffMode();
        messageType_t type();
        uint8_t       channel();
        uint8_t       data1();
        uint8_t       data2();
        uint8_t*      sysExArray();
        uint16_t      length();
        void          setRunningStatusState(bool state);
        void          setNoteOffMode(noteOffType_t type);
        void          registerThruInterface(Thru& interface);
        void          unregisterThruInterface(Thru& interface);
        Message&      message();

        private:
        Transport&                                  _transport;
        Message                                     _message                      = {};
        bool                                        _initialized                  = false;
        bool                                        _useRunningStatus             = false;
        bool                                        _recursiveParseState          = false;
        uint8_t                                     _mRunningStatusRX             = 0;
        uint8_t                                     _mRunningStatusTX             = 0;
        uint8_t                                     _mPendingMessage[3]           = {};
        uint16_t                                    _pendingMessageExpectedLength = 0;
        uint16_t                                    _pendingMessageIndex          = 0;
        noteOffType_t                               _noteOffMode                  = noteOffType_t::NOTE_ON_ZERO_VEL;
        std::array<Thru*, MIDI_MAX_THRU_INTERFACES> _thruInterface                = {};

        void    thru();
        uint8_t status(messageType_t inType, uint8_t inChannel);
    };
}    // namespace lib::midi
