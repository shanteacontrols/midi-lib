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
#define MIDI_SYSEX_ARRAY_SIZE 128
#endif

#ifndef MIDI_MAX_THRU_INTERFACES
#define MIDI_MAX_THRU_INTERFACES 5
#endif

namespace lib::midi
{
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

    enum class noteOffType_t : uint8_t
    {
        NOTE_ON_ZERO_VEL,
        STANDARD_NOTE_OFF
    };

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
    struct Message
    {
        uint8_t       channel                           = 1;
        messageType_t type                              = messageType_t::INVALID;
        uint8_t       data1                             = 0;
        uint8_t       data2                             = 0;
        uint8_t       sysexArray[MIDI_SYSEX_ARRAY_SIZE] = {};
        bool          valid                             = false;    // validity implies that the message respects the MIDI norm
        size_t        length                            = 0;

        Message() = default;
    };

    /// Helper class used to convert 7-bit high and low bytes to single 14-bit value.
    /// @param [in] high    Higher 7 bits.
    /// @param [in] low     Lower 7 bits.
    class Merge14Bit
    {
        public:
        Merge14Bit(uint8_t high, uint8_t low)
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

            uint16_t joined = 0;
            joined          = high;
            joined <<= 8;
            joined |= low;

            _value = joined;
        }

        uint16_t value() const
        {
            return _value;
        }

        private:
        uint16_t _value = 0;
    };

    /// Helper class used to convert single 14-bit value to high and low bytes (7-bit each).
    /// @param [in]     value   14-bit value to split.
    class Split14Bit
    {
        public:
        Split14Bit(uint16_t value)
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
        uint8_t _high = 0;
        uint8_t _low  = 0;
    };

    /// Calculates octave from received MIDI note.
    /// @param [in] note    Raw MIDI note (0-127).
    /// returns: Calculated octave (0 - MIDI_NOTES-1).
    constexpr uint8_t NOTE_TO_OCTAVE(int8_t note)
    {
        return (note &= 0x7F) / static_cast<uint8_t>(note_t::AMOUNT);
    }

    /// Calculates tonic (root note) from received raw MIDI note.
    /// @param [in] note    Raw MIDI note (0-127).
    /// returns: Calculated tonic/root note (enumerated type). See note_t enumeration.
    constexpr note_t NOTE_TO_TONIC(int8_t note)
    {
        return static_cast<note_t>((note &= 0x7F) % static_cast<uint8_t>(note_t::AMOUNT));
    }

    /// Extract MIDI channel from status byte.
    /// param inStatus [in]            Status byte.
    /// returns: Extracted MIDI channel.
    constexpr uint8_t CHANNEL_FROM_STATUS_BYTE(uint8_t inStatus)
    {
        return (inStatus & 0x0f) + 1;
    }

    constexpr bool IS_CHANNEL_MESSAGE(messageType_t inType)
    {
        return (inType == messageType_t::NOTE_OFF ||
                inType == messageType_t::NOTE_ON ||
                inType == messageType_t::CONTROL_CHANGE ||
                inType == messageType_t::AFTER_TOUCH_POLY ||
                inType == messageType_t::AFTER_TOUCH_CHANNEL ||
                inType == messageType_t::PITCH_BEND ||
                inType == messageType_t::PROGRAM_CHANGE);
    }

    constexpr bool IS_SYSTEM_REAL_TIME(messageType_t inType)
    {
        return (inType == messageType_t::SYS_REAL_TIME_CLOCK ||
                inType == messageType_t::SYS_REAL_TIME_START ||
                inType == messageType_t::SYS_REAL_TIME_CONTINUE ||
                inType == messageType_t::SYS_REAL_TIME_STOP ||
                inType == messageType_t::SYS_REAL_TIME_ACTIVE_SENSING ||
                inType == messageType_t::SYS_REAL_TIME_SYSTEM_RESET);
    }

    constexpr bool IS_SYSTEM_COMMON(messageType_t inType)
    {
        return (inType == messageType_t::SYS_COMMON_TIME_CODE_QUARTER_FRAME ||
                inType == messageType_t::SYS_COMMON_SONG_POSITION ||
                inType == messageType_t::SYS_COMMON_SONG_SELECT ||
                inType == messageType_t::SYS_COMMON_TUNE_REQUEST);
    }

    /// Extract MIDI message type from status byte.
    /// param inStatus [in]    Status byte.
    /// returns: Extracted MIDI message type.
    constexpr messageType_t TYPE_FROM_STATUS_BYTE(uint8_t inStatus)
    {
        // extract an enumerated MIDI type from a status byte

        if ((inStatus < 0x80) ||
            (inStatus == 0xF4) ||
            (inStatus == 0xF5) ||
            (inStatus == 0xF9) ||
            (inStatus == 0xFD))
        {
            return messageType_t::INVALID;
        }

        if (inStatus < 0xF0)
        {
            // channel message, remove channel nibble
            return static_cast<messageType_t>(inStatus & 0xF0);
        }

        return static_cast<messageType_t>(inStatus);    // NOLINT
    }

    constexpr uint8_t  MAX_VALUE_7BIT  = 127;
    constexpr uint16_t MAX_VALUE_14BIT = 16383;

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
}    // namespace lib::midi
