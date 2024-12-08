#pragma once

#include <inttypes.h>

namespace lib::midi::serial
{
    struct Packet
    {
        uint8_t data = 0;
    };

    class Hwa
    {
        public:
        virtual bool init()              = 0;
        virtual bool deInit()            = 0;
        virtual bool write(Packet& data) = 0;
        virtual bool read(Packet& data)  = 0;
    };
}    // namespace lib::midi::serial
