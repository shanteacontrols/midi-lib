#include "tests/Common.h"
#include "MIDI/transport/BLE.h"

using namespace MIDIlib;

namespace
{
    class BLEMIDITest : public ::testing::Test
    {
        protected:
        void SetUp()
        {
            ASSERT_TRUE(_bleMIDI.init());
        }

        void TearDown()
        {}

        class BLEHWA : public BLEMIDI::HWA
        {
            public:
            BLEHWA() = default;

            bool init() override
            {
                return true;
            }

            bool deInit() override
            {
                return true;
            }

            bool write(BLEMIDI::bleMIDIPacket_t& packet) override
            {
                _writePackets.push_back(packet);
                return true;
            }

            bool read(BLEMIDI::bleMIDIPacket_t& packet) override
            {
                if (_readPackets.size())
                {
                    packet = _readPackets.at(0);
                    _readPackets.erase(_readPackets.begin());

                    return true;
                }

                return false;
            }

            uint32_t time() override
            {
                return 0x80;
            }

            std::vector<BLEMIDI::bleMIDIPacket_t> _writePackets;
            std::vector<BLEMIDI::bleMIDIPacket_t> _readPackets;
        };

        BLEHWA  _hwa;
        BLEMIDI _bleMIDI = BLEMIDI(_hwa);
    };
}    // namespace

TEST_F(BLEMIDITest, ReadSingleMessage)
{
    BLEMIDI::bleMIDIPacket_t packet;

    packet.data.at(packet.size++) = 0x80;    // header
    packet.data.at(packet.size++) = 0x80;    // timestamp
    packet.data.at(packet.size++) = 0x90;    // note on
    packet.data.at(packet.size++) = 0x00;    // note index
    packet.data.at(packet.size++) = 0x7F;    // velocity

    _hwa._readPackets.push_back(packet);

    ASSERT_TRUE(_bleMIDI.read());
    EXPECT_EQ(1, _bleMIDI.message().channel);
    EXPECT_EQ(MIDIlib::Base::messageType_t::NOTE_ON, _bleMIDI.message().type);
    EXPECT_EQ(0, _bleMIDI.message().data1);
    EXPECT_EQ(0x7F, _bleMIDI.message().data2);

    ASSERT_FALSE(_bleMIDI.read());
}

TEST_F(BLEMIDITest, ReadTwoMessages)
{
    BLEMIDI::bleMIDIPacket_t packet;

    packet.data.at(packet.size++) = 0x80;    // header
    packet.data.at(packet.size++) = 0x80;    // timestamp
    packet.data.at(packet.size++) = 0x90;    // note on
    packet.data.at(packet.size++) = 0x00;    // note index
    packet.data.at(packet.size++) = 0x7F;    // velocity
    packet.data.at(packet.size++) = 0x80;    // timestamp
    packet.data.at(packet.size++) = 0x91;    // note on,
    packet.data.at(packet.size++) = 0x00;    // note index
    packet.data.at(packet.size++) = 0x7F;    // velocity

    _hwa._readPackets.push_back(packet);

    ASSERT_TRUE(_bleMIDI.read());
    EXPECT_EQ(1, _bleMIDI.message().channel);
    EXPECT_EQ(MIDIlib::Base::messageType_t::NOTE_ON, _bleMIDI.message().type);
    EXPECT_EQ(0, _bleMIDI.message().data1);
    EXPECT_EQ(0x7F, _bleMIDI.message().data2);

    ASSERT_TRUE(_bleMIDI.read());
    EXPECT_EQ(2, _bleMIDI.message().channel);
    EXPECT_EQ(MIDIlib::Base::messageType_t::NOTE_ON, _bleMIDI.message().type);
    EXPECT_EQ(0, _bleMIDI.message().data1);
    EXPECT_EQ(0x7F, _bleMIDI.message().data2);

    ASSERT_FALSE(_bleMIDI.read());
}

TEST_F(BLEMIDITest, ReadTwoMessagesWithRunningStatusNoTimestampBetween)
{
    BLEMIDI::bleMIDIPacket_t packet;

    packet.data.at(packet.size++) = 0x80;    // header
    packet.data.at(packet.size++) = 0x80;    // timestamp
    packet.data.at(packet.size++) = 0x90;    // note on
    packet.data.at(packet.size++) = 0x00;    // note index
    packet.data.at(packet.size++) = 0x7F;    // velocity
    packet.data.at(packet.size++) = 0x00;    // note index
    packet.data.at(packet.size++) = 0x7E;    // velocity

    _hwa._readPackets.push_back(packet);

    ASSERT_TRUE(_bleMIDI.read());
    EXPECT_EQ(1, _bleMIDI.message().channel);
    EXPECT_EQ(MIDIlib::Base::messageType_t::NOTE_ON, _bleMIDI.message().type);
    EXPECT_EQ(0, _bleMIDI.message().data1);
    EXPECT_EQ(0x7F, _bleMIDI.message().data2);

    ASSERT_TRUE(_bleMIDI.read());
    EXPECT_EQ(1, _bleMIDI.message().channel);
    EXPECT_EQ(MIDIlib::Base::messageType_t::NOTE_ON, _bleMIDI.message().type);
    EXPECT_EQ(0, _bleMIDI.message().data1);
    EXPECT_EQ(0x7E, _bleMIDI.message().data2);

    ASSERT_FALSE(_bleMIDI.read());
}

TEST_F(BLEMIDITest, ReadTwoMessagesWithRunningStatusTimestampBetween)
{
    BLEMIDI::bleMIDIPacket_t packet;

    packet.data.at(packet.size++) = 0x80;    // header
    packet.data.at(packet.size++) = 0x80;    // timestamp
    packet.data.at(packet.size++) = 0x90;    // note on
    packet.data.at(packet.size++) = 0x00;    // note index
    packet.data.at(packet.size++) = 0x7F;    // velocity
    packet.data.at(packet.size++) = 0x80;    // timestamp
    packet.data.at(packet.size++) = 0x00;    // note index
    packet.data.at(packet.size++) = 0x7E;    // velocity

    _hwa._readPackets.push_back(packet);

    ASSERT_TRUE(_bleMIDI.read());
    EXPECT_EQ(1, _bleMIDI.message().channel);
    EXPECT_EQ(MIDIlib::Base::messageType_t::NOTE_ON, _bleMIDI.message().type);
    EXPECT_EQ(0, _bleMIDI.message().data1);
    EXPECT_EQ(0x7F, _bleMIDI.message().data2);

    ASSERT_TRUE(_bleMIDI.read());
    EXPECT_EQ(1, _bleMIDI.message().channel);
    EXPECT_EQ(MIDIlib::Base::messageType_t::NOTE_ON, _bleMIDI.message().type);
    EXPECT_EQ(0, _bleMIDI.message().data1);
    EXPECT_EQ(0x7E, _bleMIDI.message().data2);

    ASSERT_FALSE(_bleMIDI.read());
}

TEST_F(BLEMIDITest, ReadSysEx1Packet)
{
    BLEMIDI::bleMIDIPacket_t packet;

    packet.data.at(packet.size++) = 0x80;
    packet.data.at(packet.size++) = 0x80;
    packet.data.at(packet.size++) = 0xF0;
    packet.data.at(packet.size++) = 0x00;
    packet.data.at(packet.size++) = 0x53;
    packet.data.at(packet.size++) = 0x43;
    packet.data.at(packet.size++) = 0x00;
    packet.data.at(packet.size++) = 0x00;
    packet.data.at(packet.size++) = 0x01;
    packet.data.at(packet.size++) = 0x80;    // timestamp before end
    packet.data.at(packet.size++) = 0xF7;

    _hwa._readPackets.push_back(packet);

    ASSERT_TRUE(_bleMIDI.read());
}

TEST_F(BLEMIDITest, ReadSysEx2Packet)
{
    // packet length is limited to 16 bytes for these tests

    BLEMIDI::bleMIDIPacket_t packet1;
    BLEMIDI::bleMIDIPacket_t packet2;

    packet1.data.at(packet1.size++) = 0x80;
    packet1.data.at(packet1.size++) = 0x80;
    packet1.data.at(packet1.size++) = 0xF0;
    packet1.data.at(packet1.size++) = 0x00;
    packet1.data.at(packet1.size++) = 0x53;
    packet1.data.at(packet1.size++) = 0x43;
    packet1.data.at(packet1.size++) = 0x00;
    packet1.data.at(packet1.size++) = 0x00;
    packet1.data.at(packet1.size++) = 0x01;
    packet1.data.at(packet1.size++) = 0x01;
    packet1.data.at(packet1.size++) = 0x01;
    packet1.data.at(packet1.size++) = 0x01;
    packet1.data.at(packet1.size++) = 0x01;
    packet1.data.at(packet1.size++) = 0x01;
    packet1.data.at(packet1.size++) = 0x01;
    packet1.data.at(packet1.size++) = 0x01;

    _hwa._readPackets.push_back(packet1);

    packet2.data.at(packet2.size++) = 0x80;    // header - no timestamp in continuation
    packet2.data.at(packet2.size++) = 0x53;
    packet2.data.at(packet2.size++) = 0x43;
    packet2.data.at(packet2.size++) = 0x80;    // timestamp
    packet2.data.at(packet2.size++) = 0xF7;

    _hwa._readPackets.push_back(packet2);

    ASSERT_TRUE(_bleMIDI.read());
    EXPECT_EQ(MIDIlib::Base::messageType_t::SYS_EX, _bleMIDI.message().type);
    EXPECT_EQ(17, _bleMIDI.message().length);
}

TEST_F(BLEMIDITest, ReadSysEx2PacketEdgeCase)
{
    // packet length is limited to 16 bytes for these tests

    BLEMIDI::bleMIDIPacket_t packet1;
    BLEMIDI::bleMIDIPacket_t packet2;

    packet1.data.at(packet1.size++) = 0x80;
    packet1.data.at(packet1.size++) = 0x80;
    packet1.data.at(packet1.size++) = 0xF0;
    packet1.data.at(packet1.size++) = 0x00;
    packet1.data.at(packet1.size++) = 0x53;
    packet1.data.at(packet1.size++) = 0x43;
    packet1.data.at(packet1.size++) = 0x00;
    packet1.data.at(packet1.size++) = 0x00;
    packet1.data.at(packet1.size++) = 0x01;
    packet1.data.at(packet1.size++) = 0x01;
    packet1.data.at(packet1.size++) = 0x01;
    packet1.data.at(packet1.size++) = 0x01;
    packet1.data.at(packet1.size++) = 0x01;
    packet1.data.at(packet1.size++) = 0x01;
    packet1.data.at(packet1.size++) = 0x01;
    packet1.data.at(packet1.size++) = 0x01;

    _hwa._readPackets.push_back(packet1);

    // packet two in this test contains single remaining byte before sysex end
    packet2.data.at(packet2.size++) = 0x80;    // header - no timestamp in continuation
    packet2.data.at(packet2.size++) = 0x80;    // timestamp
    packet2.data.at(packet2.size++) = 0xF7;

    _hwa._readPackets.push_back(packet2);

    ASSERT_TRUE(_bleMIDI.read());
    EXPECT_EQ(MIDIlib::Base::messageType_t::SYS_EX, _bleMIDI.message().type);
    EXPECT_EQ(15, _bleMIDI.message().length);
}

TEST_F(BLEMIDITest, ReadSysEx3Packet)
{
    // packet length is limited to 16 bytes for these tests

    BLEMIDI::bleMIDIPacket_t packet1;
    BLEMIDI::bleMIDIPacket_t packet2;
    BLEMIDI::bleMIDIPacket_t packet3;

    packet1.data.at(packet1.size++) = 0x80;
    packet1.data.at(packet1.size++) = 0x80;
    packet1.data.at(packet1.size++) = 0xF0;
    packet1.data.at(packet1.size++) = 0x00;
    packet1.data.at(packet1.size++) = 0x53;
    packet1.data.at(packet1.size++) = 0x43;
    packet1.data.at(packet1.size++) = 0x00;
    packet1.data.at(packet1.size++) = 0x00;
    packet1.data.at(packet1.size++) = 0x01;
    packet1.data.at(packet1.size++) = 0x01;
    packet1.data.at(packet1.size++) = 0x01;
    packet1.data.at(packet1.size++) = 0x01;
    packet1.data.at(packet1.size++) = 0x01;
    packet1.data.at(packet1.size++) = 0x01;
    packet1.data.at(packet1.size++) = 0x01;
    packet1.data.at(packet1.size++) = 0x01;

    _hwa._readPackets.push_back(packet1);

    packet2.data.at(packet2.size++) = 0x80;    // header - no timestamp in continuation
    packet2.data.at(packet2.size++) = 0x01;
    packet2.data.at(packet2.size++) = 0x01;
    packet2.data.at(packet2.size++) = 0x01;
    packet2.data.at(packet2.size++) = 0x01;
    packet2.data.at(packet2.size++) = 0x01;
    packet2.data.at(packet2.size++) = 0x01;
    packet2.data.at(packet2.size++) = 0x01;
    packet2.data.at(packet2.size++) = 0x01;
    packet2.data.at(packet2.size++) = 0x01;
    packet2.data.at(packet2.size++) = 0x01;
    packet2.data.at(packet2.size++) = 0x01;
    packet2.data.at(packet2.size++) = 0x01;
    packet2.data.at(packet2.size++) = 0x01;
    packet2.data.at(packet2.size++) = 0x01;
    packet2.data.at(packet2.size++) = 0x01;

    _hwa._readPackets.push_back(packet2);

    packet3.data.at(packet3.size++) = 0x80;    // header - no timestamp in continuation
    packet3.data.at(packet3.size++) = 0x80;    // timestamp
    packet3.data.at(packet3.size++) = 0xF7;

    _hwa._readPackets.push_back(packet3);

    ASSERT_TRUE(_bleMIDI.read());
    EXPECT_EQ(MIDIlib::Base::messageType_t::SYS_EX, _bleMIDI.message().type);
}