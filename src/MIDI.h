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

#include <inttypes.h>
#include <stddef.h>
#include <array>

/// To override the symbols below, define global symbols with the same name.

#ifndef MIDI_SYSEX_ARRAY_SIZE
#define MIDI_SYSEX_ARRAY_SIZE 45
#endif

#ifndef MIDI_MAX_THRU_INTERFACES
#define MIDI_MAX_THRU_INTERFACES 5
#endif

namespace MIDIlib
{
    class Base
    {
        public:
        /// Enumeration holding various types of MIDI messages.
        enum class messageType_t : uint8_t
        {
            NOTE_OFF                           = 0x80,    ///< Note Off
            NOTE_ON                            = 0x90,    ///< Note On
            CONTROL_CHANGE                     = 0xB0,    ///< Control Change / Channel Mode
            PROGRAM_CHANGE                     = 0xC0,    ///< Program Change
            AFTER_TOUCH_CHANNEL                = 0xD0,    ///< Channel (monophonic) AfterTouch
            AFTER_TOUCH_POLY                   = 0xA0,    ///< Polyphonic AfterTouch
            PITCH_BEND                         = 0xE0,    ///< Pitch Bend
            SYS_EX                             = 0xF0,    ///< System Exclusive
            SYS_COMMON_TIME_CODE_QUARTER_FRAME = 0xF1,    ///< System Common - MIDI Time Code Quarter Frame
            SYS_COMMON_SONG_POSITION           = 0xF2,    ///< System Common - Song Position Pointer
            SYS_COMMON_SONG_SELECT             = 0xF3,    ///< System Common - Song Select
            SYS_COMMON_TUNE_REQUEST            = 0xF6,    ///< System Common - Tune Request
            SYS_REAL_TIME_CLOCK                = 0xF8,    ///< System Real Time - Timing Clock
            SYS_REAL_TIME_START                = 0xFA,    ///< System Real Time - Start
            SYS_REAL_TIME_CONTINUE             = 0xFB,    ///< System Real Time - Continue
            SYS_REAL_TIME_STOP                 = 0xFC,    ///< System Real Time - Stop
            SYS_REAL_TIME_ACTIVE_SENSING       = 0xFE,    ///< System Real Time - Active Sensing
            SYS_REAL_TIME_SYSTEM_RESET         = 0xFF,    ///< System Real Time - System Reset
            MMC_PLAY                           = 0x02,
            MMC_STOP                           = 0x01,
            MMC_PAUSE                          = 0x09,
            MMC_RECORD_START                   = 0x06,
            MMC_RECORD_STOP                    = 0x07,
            NRPN_7BIT                          = 0x99,
            NRPN_14BIT                         = 0x38,
            CONTROL_CHANGE_14BIT               = 0x32,
            INVALID                            = 0x00    ///< For notifying errors
        };

        /// Holds various types of MIDI Note Off message
        enum class noteOffType_t : uint8_t
        {
            NOTE_ON_ZERO_VEL,
            STANDARD_NOTE_OFF
        };

        /// List off all possible MIDI notes.
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

        /// Holds decoded data of a MIDI message.
        struct message_t
        {
            uint8_t       channel                           = 1;
            messageType_t type                              = messageType_t::INVALID;
            uint8_t       data1                             = 0;
            uint8_t       data2                             = 0;
            uint8_t       sysexArray[MIDI_SYSEX_ARRAY_SIZE] = {};
            bool          valid                             = false;    // validity implies that the message respects the MIDI norm
            size_t        length                            = 0;

            message_t() = default;
        };

        /// Helper class used to convert 7-bit high and low bytes to single 14-bit value.
        /// @param [in] high    Higher 7 bits.
        /// @param [in] low     Lower 7 bits.
        class Merge14bit
        {
            public:
            Merge14bit(uint8_t high, uint8_t low)
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

                _value = joined;
            }

            uint16_t value() const
            {
                return _value;
            }

            private:
            uint16_t _value;
        };

        /// Helper class used to convert single 14-bit value to high and low bytes (7-bit each).
        /// @param [in]     value   14-bit value to split.
        class Split14bit
        {
            public:
            Split14bit(uint16_t value)
            {
                uint8_t newHigh = (value >> 8) & 0xFF;
                uint8_t newLow  = value & 0xFF;
                newHigh         = (newHigh << 1) & 0x7F;

                if ((newLow >> 7) & 0x01)
                {
                    newHigh |= 0x01;
                }
                else
                {
                    newHigh &= ~0x01;
                }

                newLow &= 0x7F;
                _high = newHigh;
                _low  = newLow;
            }

            uint8_t high() const
            {
                return _high;
            }

            uint8_t low() const
            {
                return _low;
            }

            private:
            uint8_t _high;
            uint8_t _low;
        };

        class Thru
        {
            public:
            virtual bool beginTransmission(messageType_t type) = 0;
            virtual bool write(uint8_t data)                   = 0;
            virtual bool endTransmission()                     = 0;
        };

        class Transport : public Thru
        {
            public:
            virtual bool init()              = 0;
            virtual bool deInit()            = 0;
            virtual bool read(uint8_t& data) = 0;
        };

        static constexpr uint8_t  MIDI_7BIT_VALUE_MAX  = 127;
        static constexpr uint16_t MIDI_14BIT_VALUE_MAX = 16383;

        Base(Transport& transport)
            : _transport(transport)
        {}

        bool                 init();
        bool                 deInit();
        bool                 initialized();
        void                 reset();
        Transport&           transport();
        bool                 sendNoteOn(uint8_t inNoteNumber, uint8_t inVelocity, uint8_t inChannel);
        bool                 sendNoteOff(uint8_t inNoteNumber, uint8_t inVelocity, uint8_t inChannel);
        bool                 sendProgramChange(uint8_t inProgramNumber, uint8_t inChannel);
        bool                 sendControlChange(uint8_t inControlNumber, uint8_t inControlValue, uint8_t inChannel);
        bool                 sendControlChange14bit(uint16_t inControlNumber, uint16_t inControlValue, uint8_t inChannel);
        bool                 sendPitchBend(uint16_t inPitchValue, uint8_t inChannel);
        bool                 sendAfterTouch(uint8_t inPressure, uint8_t inChannel, uint8_t inNoteNumber);
        bool                 sendAfterTouch(uint8_t inPressure, uint8_t inChannel);
        bool                 sendSysEx(uint16_t inLength, const uint8_t* inArray, bool inArrayContainsBoundaries);
        bool                 sendTimeCodeQuarterFrame(uint8_t inTypeNibble, uint8_t inValuesNibble);
        bool                 sendTimeCodeQuarterFrame(uint8_t inData);
        bool                 sendSongPosition(uint16_t inBeats);
        bool                 sendSongSelect(uint8_t inSongNumber);
        bool                 sendTuneRequest();
        bool                 sendCommon(messageType_t inType, uint8_t inData1 = 0);
        bool                 sendRealTime(messageType_t inType);
        bool                 sendMMC(uint8_t deviceID, messageType_t mmc);
        bool                 sendNRPN(uint16_t inParameterNumber, uint16_t inValue, uint8_t inChannel, bool value14bit = false);
        bool                 send(messageType_t inType, uint8_t inData1, uint8_t inData2, uint8_t inChannel);
        bool                 read();
        bool                 parse();
        static bool          isChannelMessage(messageType_t inType);
        static bool          isSystemRealTime(messageType_t inType);
        static bool          isSystemCommon(messageType_t inType);
        void                 useRecursiveParsing(bool state);
        bool                 runningStatusState();
        noteOffType_t        noteOffMode();
        static uint8_t       noteToOctave(int8_t note);
        static note_t        noteToTonic(int8_t note);
        messageType_t        type();
        uint8_t              channel();
        uint8_t              data1();
        uint8_t              data2();
        uint8_t*             sysExArray();
        uint16_t             length();
        static messageType_t typeFromStatusByte(uint8_t inStatus);
        static uint8_t       channelFromStatusByte(uint8_t inStatus);
        void                 setRunningStatusState(bool state);
        void                 setNoteOffMode(noteOffType_t type);
        void                 registerThruInterface(Thru& interface);
        void                 unregisterThruInterface(Thru& interface);
        message_t&           message();

        private:
        void    thru();
        uint8_t status(messageType_t inType, uint8_t inChannel);

        Transport&                                  _transport;
        message_t                                   _message;
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
    };
}    // namespace MIDIlib
