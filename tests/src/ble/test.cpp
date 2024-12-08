#include "tests/common.h"
#include "lib/midi/transport/ble/ble.h"

using namespace lib::midi;
using namespace ble;

namespace
{
    class BleMidiTest : public ::testing::Test
    {
        protected:
        void SetUp()
        {
            ASSERT_TRUE(_ble.init());
        }

        void TearDown()
        {}

        class BleHwa : public Hwa
        {
            public:
            BleHwa() = default;

            bool init() override
            {
                return true;
            }

            bool deInit() override
            {
                return true;
            }

            bool write(Packet& packet) override
            {
                _writePackets.push_back(packet);
                return true;
            }

            bool read(Packet& packet) override
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

            std::vector<Packet> _writePackets = {};
            std::vector<Packet> _readPackets  = {};
        };

        BleHwa _hwa;
        Ble    _ble = Ble(_hwa);
    };
}    // namespace

TEST_F(BleMidiTest, ReadSingleMessage)
{
    Packet packet = {};

    packet.data.at(packet.size++) = 0x80;    // header
    packet.data.at(packet.size++) = 0x80;    // timestamp
    packet.data.at(packet.size++) = 0x90;    // note on
    packet.data.at(packet.size++) = 0x00;    // note index
    packet.data.at(packet.size++) = 0x7F;    // velocity

    _hwa._readPackets.push_back(packet);

    ASSERT_TRUE(_ble.read());
    EXPECT_EQ(1, _ble.message().channel);
    EXPECT_EQ(messageType_t::NOTE_ON, _ble.message().type);
    EXPECT_EQ(0, _ble.message().data1);
    EXPECT_EQ(0x7F, _ble.message().data2);

    ASSERT_FALSE(_ble.read());
}

TEST_F(BleMidiTest, ReadTwoMessages)
{
    Packet packet = {};

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

    ASSERT_TRUE(_ble.read());
    EXPECT_EQ(1, _ble.message().channel);
    EXPECT_EQ(messageType_t::NOTE_ON, _ble.message().type);
    EXPECT_EQ(0, _ble.message().data1);
    EXPECT_EQ(0x7F, _ble.message().data2);

    ASSERT_TRUE(_ble.read());
    EXPECT_EQ(2, _ble.message().channel);
    EXPECT_EQ(messageType_t::NOTE_ON, _ble.message().type);
    EXPECT_EQ(0, _ble.message().data1);
    EXPECT_EQ(0x7F, _ble.message().data2);

    ASSERT_FALSE(_ble.read());
}

TEST_F(BleMidiTest, ReadTwoMessagesWithRunningStatusNoTimestampBetween)
{
    Packet packet = {};

    packet.data.at(packet.size++) = 0x80;    // header
    packet.data.at(packet.size++) = 0x80;    // timestamp
    packet.data.at(packet.size++) = 0x90;    // note on
    packet.data.at(packet.size++) = 0x00;    // note index
    packet.data.at(packet.size++) = 0x7F;    // velocity
    packet.data.at(packet.size++) = 0x00;    // note index
    packet.data.at(packet.size++) = 0x7E;    // velocity

    _hwa._readPackets.push_back(packet);

    ASSERT_TRUE(_ble.read());
    EXPECT_EQ(1, _ble.message().channel);
    EXPECT_EQ(messageType_t::NOTE_ON, _ble.message().type);
    EXPECT_EQ(0, _ble.message().data1);
    EXPECT_EQ(0x7F, _ble.message().data2);

    ASSERT_TRUE(_ble.read());
    EXPECT_EQ(1, _ble.message().channel);
    EXPECT_EQ(messageType_t::NOTE_ON, _ble.message().type);
    EXPECT_EQ(0, _ble.message().data1);
    EXPECT_EQ(0x7E, _ble.message().data2);

    ASSERT_FALSE(_ble.read());
}

TEST_F(BleMidiTest, ReadTwoMessagesWithRunningStatusTimestampBetween)
{
    Packet packet = {};

    packet.data.at(packet.size++) = 0x80;    // header
    packet.data.at(packet.size++) = 0x80;    // timestamp
    packet.data.at(packet.size++) = 0x90;    // note on
    packet.data.at(packet.size++) = 0x00;    // note index
    packet.data.at(packet.size++) = 0x7F;    // velocity
    packet.data.at(packet.size++) = 0x80;    // timestamp
    packet.data.at(packet.size++) = 0x00;    // note index
    packet.data.at(packet.size++) = 0x7E;    // velocity

    _hwa._readPackets.push_back(packet);

    ASSERT_TRUE(_ble.read());
    EXPECT_EQ(1, _ble.message().channel);
    EXPECT_EQ(messageType_t::NOTE_ON, _ble.message().type);
    EXPECT_EQ(0, _ble.message().data1);
    EXPECT_EQ(0x7F, _ble.message().data2);

    ASSERT_TRUE(_ble.read());
    EXPECT_EQ(1, _ble.message().channel);
    EXPECT_EQ(messageType_t::NOTE_ON, _ble.message().type);
    EXPECT_EQ(0, _ble.message().data1);
    EXPECT_EQ(0x7E, _ble.message().data2);

    ASSERT_FALSE(_ble.read());
}

TEST_F(BleMidiTest, ReadSysEx1Packet)
{
    Packet packet = {};

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

    ASSERT_TRUE(_ble.read());
}

TEST_F(BleMidiTest, ReadSysEx2Packet)
{
    // packet length is limited to 16 bytes for these tests

    Packet packet1 = {};
    Packet packet2 = {};

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

    ASSERT_TRUE(_ble.read());
    EXPECT_EQ(messageType_t::SYS_EX, _ble.message().type);
    EXPECT_EQ(17, _ble.message().length);
}

TEST_F(BleMidiTest, ReadSysEx2PacketEdgeCase)
{
    // packet length is limited to 16 bytes for these tests

    Packet packet1 = {};
    Packet packet2 = {};

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

    ASSERT_TRUE(_ble.read());
    EXPECT_EQ(messageType_t::SYS_EX, _ble.message().type);
    EXPECT_EQ(15, _ble.message().length);
}

TEST_F(BleMidiTest, ReadSysEx3Packet)
{
    // packet length is limited to 16 bytes for these tests

    Packet packet1 = {};
    Packet packet2 = {};
    Packet packet3 = {};

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

    ASSERT_TRUE(_ble.read());
    EXPECT_EQ(messageType_t::SYS_EX, _ble.message().type);
}