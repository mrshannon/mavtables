// MAVLink router and firewall.
// Copyright (C) 2018  Michael R. Shannon <mrshannon.aerospace@gmail.com>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include <catch.hpp>
#include <string>
#include <vector>
#include <memory>
#include <cstdint>
#include <optional>
#include <stdexcept>

#include <boost/range/irange.hpp>

#include "PacketVersion2.hpp"
#include "MAVAddress.hpp"
#include "Connection.hpp"
#include "mavlink.h"
#include "macros.hpp"
#include "util.hpp"

namespace
{

#ifdef __clang__
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wweak-vtables"
#endif

    // Subclass of Packet used for testing the abstract class Connection.
    class ConnectionTestClass : public Connection
    {
    };

#ifdef __clang__
    #pragma clang diagnostic pop
#endif


    // HEARTBEAT structure for testing packets without target system/component.
    struct PACKED Heartbeat
    {
        struct PACKED payload
        {
            uint8_t type = 1;
            uint8_t autopilot = 2;
            uint8_t base_mode = 3;
            uint32_t custom_mode = 4;
            uint8_t system_status = 5;
            uint8_t mavlink_version = 6;
        };
        uint8_t magic = 0xFD;
        uint8_t len = sizeof(payload);
        uint8_t incompat_flags = 0;
        uint8_t compat_flags = 0;
        uint8_t seq = 0;
        uint8_t sysid = 1;
        uint8_t compid = 0;
        uint32_t msgid : 24;
        payload payload;
        uint16_t checksum = 0xFACE;
        Heartbeat() : msgid(0) {}
    };


    // PING structure for testing target system/component.
    struct PACKED Ping
    {
        struct PACKED payload
        {
            uint64_t time_usec = 295128000000000;
            uint32_t seq = 0xBA5EBA11;
            uint8_t target_system = 255;
            uint8_t target_component = 23;
        };
        uint8_t magic = 0xFD;
        uint8_t len = sizeof(payload);
        uint8_t incompat_flags = 0;
        uint8_t compat_flags = 0;
        uint8_t seq = 0;
        uint8_t sysid = 60;
        uint8_t compid = 40;
        uint32_t msgid : 24;
        payload payload;
        uint16_t checksum = 0xFACE;
        Ping() : msgid(4) {}
    };


    // SET_MODE structure for testing target system only.
    struct PACKED SetMode
    {
        struct PACKED payload
        {
            uint32_t custom_mode = 2;
            uint8_t target_system = 123;
            uint8_t base_mode = 1;
        };
        uint8_t magic = 0xFD;
        uint8_t len = sizeof(payload);
        uint8_t incompat_flags = 0;
        uint8_t compat_flags = 0;
        uint8_t seq = 0;
        uint8_t sysid = 70;
        uint8_t compid = 30;
        uint32_t msgid : 24;
        payload payload;
        uint16_t checksum = 0xFACE;
        SetMode() : msgid(11) {}
    };


    // MISSION_SET_CURRENT for testing trimmed out target system and components.
    struct PACKED MissionSetCurrent
    {
        struct PACKED payload
        {
            uint16_t seq = 12;
            // uint8_t target_system = 0;
            // uint8_t target_component = 0;
        };
        uint8_t magic = 0xFD;
        uint8_t len = sizeof(payload);
        uint8_t incompat_flags = 0;
        uint8_t compat_flags = 0;
        uint8_t seq = 0;
        uint8_t sysid = 80;
        uint8_t compid = 20;
        uint32_t msgid : 24;
        payload payload;
        uint16_t checksum = 0xFACE;
        MissionSetCurrent() : msgid(41) {}
    };


    // ENCAPSULATED_DATA structure for testing maximum length packets.
    struct PACKED EncapsulatedData
    {
        struct PACKED payload
        {
            uint16_t seqnr = 0;
            uint8_t data[253];
        };
        uint8_t magic = 0xFD;
        uint8_t len = sizeof(payload);
        uint8_t incompat_flags = 0;
        uint8_t compat_flags = 0;
        uint8_t seq = 0;
        uint8_t sysid = 255;
        uint8_t compid = 1;
        uint32_t msgid : 24;
        payload payload;
        uint16_t checksum = 0xFACE;
        EncapsulatedData() : msgid(131) {}
    };


    // PARAM_EXT_REQUEST_LIST structure for testing message ID's beyond 255.
    struct PACKED ParamExtRequestList
    {
        struct PACKED payload
        {
            uint8_t target_system = 32;
            uint8_t target_component = 64;
        };
        uint8_t magic = 0xFD;
        uint8_t len = sizeof(payload);
        uint8_t incompat_flags = 0;
        uint8_t compat_flags = 0;
        uint8_t seq = 0;
        uint8_t sysid = 1;
        uint8_t compid = 255;
        uint32_t msgid : 24;
        payload payload;
        uint16_t checksum = 0xFACE;
        ParamExtRequestList() : msgid(321) {}
    };


    // Convert a MAVLink packet structure to a vector of bytes.
    template <class T>
    static std::vector<uint8_t> to_vector(T packet, bool signature = false)
    {
        std::vector<uint8_t> data;

        // Set signature flag.
        if (signature)
        {
            packet.incompat_flags |= MAVLINK_IFLAG_SIGNED;
        }

        data.assign(reinterpret_cast<uint8_t *>(&packet),
                    reinterpret_cast<uint8_t *>(&packet) + sizeof(packet));

        // Insert a dummy signature.
        if (signature)
        {
            std::vector<uint8_t> sig =
            {
                1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13
            };
            data.insert(std::end(data), std::begin(sig), std::end(sig));
        }

        return data;
    }

}


TEST_CASE("PacketVersion2's can be constructed.", "[PacketVersion2]")
{
    Heartbeat heartbeat;
    Ping ping;
    SetMode set_mode;
    MissionSetCurrent mission_set_current;
    EncapsulatedData encapsulated_data;
    ParamExtRequestList param_ext_request_list;
    auto conn = std::make_shared<ConnectionTestClass>();
    SECTION("With proper arguments and no signature.")
    {
        // HEARTBEAT
        REQUIRE_NOTHROW(PacketVersion2(to_vector(heartbeat), conn));
        REQUIRE_NOTHROW(PacketVersion2(to_vector(heartbeat), conn, -10));
        REQUIRE_NOTHROW(PacketVersion2(to_vector(heartbeat), conn, +10));
        // PING
        REQUIRE_NOTHROW(PacketVersion2(to_vector(ping), conn));
        REQUIRE_NOTHROW(PacketVersion2(to_vector(ping), conn, -10));
        REQUIRE_NOTHROW(PacketVersion2(to_vector(ping), conn, +10));
        // SET_MODE
        REQUIRE_NOTHROW(PacketVersion2(to_vector(set_mode), conn));
        REQUIRE_NOTHROW(PacketVersion2(to_vector(set_mode), conn, -10));
        REQUIRE_NOTHROW(PacketVersion2(to_vector(set_mode), conn, +10));
        // PARAM_REQUEST_READ
        REQUIRE_NOTHROW(PacketVersion2(to_vector(mission_set_current), conn));
        REQUIRE_NOTHROW(
            PacketVersion2(to_vector(mission_set_current), conn, -10));
        REQUIRE_NOTHROW(
            PacketVersion2(to_vector(mission_set_current), conn, +10));
        // ENCAPSULATED_DATA
        REQUIRE_NOTHROW(PacketVersion2(to_vector(encapsulated_data), conn));
        REQUIRE_NOTHROW(
            PacketVersion2(to_vector(encapsulated_data), conn, -10));
        REQUIRE_NOTHROW(
            PacketVersion2(to_vector(encapsulated_data), conn, +10));
        // PARAM_EXT_REQUEST_READ
        REQUIRE_NOTHROW(
            PacketVersion2(to_vector(param_ext_request_list), conn));
        REQUIRE_NOTHROW(
            PacketVersion2(to_vector(param_ext_request_list), conn, -10));
        REQUIRE_NOTHROW(
            PacketVersion2(to_vector(param_ext_request_list), conn, +10));
    }
    SECTION("With proper arguments and a signature.")
    {
        // HEARTBEAT
        REQUIRE_NOTHROW(PacketVersion2(to_vector(heartbeat, true), conn));
        REQUIRE_NOTHROW(PacketVersion2(to_vector(heartbeat, true), conn, -10));
        REQUIRE_NOTHROW(PacketVersion2(to_vector(heartbeat, true), conn, +10));
        // PING
        REQUIRE_NOTHROW(PacketVersion2(to_vector(ping, true), conn));
        REQUIRE_NOTHROW(PacketVersion2(to_vector(ping, true), conn, -10));
        REQUIRE_NOTHROW(PacketVersion2(to_vector(ping, true), conn, +10));
        // SET_MODE
        REQUIRE_NOTHROW(PacketVersion2(to_vector(set_mode, true), conn));
        REQUIRE_NOTHROW(PacketVersion2(to_vector(set_mode, true), conn, -10));
        REQUIRE_NOTHROW(PacketVersion2(to_vector(set_mode, true), conn, +10));
        // PARAM_REQUEST_READ
        REQUIRE_NOTHROW(
            PacketVersion2(to_vector(mission_set_current, true), conn));
        REQUIRE_NOTHROW(
            PacketVersion2(to_vector(mission_set_current, true), conn, -10));
        REQUIRE_NOTHROW(
            PacketVersion2(to_vector(mission_set_current, true), conn, +10));
        // ENCAPSULATED_DATA
        REQUIRE_NOTHROW(
            PacketVersion2(to_vector(encapsulated_data, true), conn));
        REQUIRE_NOTHROW(
            PacketVersion2(to_vector(encapsulated_data, true), conn, -10));
        REQUIRE_NOTHROW(
            PacketVersion2(to_vector(encapsulated_data, true), conn, +10));
        // PARAM_EXT_REQUEST_READ
        REQUIRE_NOTHROW(
            PacketVersion2(to_vector(param_ext_request_list, true), conn));
        REQUIRE_NOTHROW(
            PacketVersion2(to_vector(param_ext_request_list, true), conn, -10));
        REQUIRE_NOTHROW(
            PacketVersion2(to_vector(param_ext_request_list, true), conn, +10));
    }
    SECTION("And ensures a complete header is given (without signature).")
    {
        // 0 length packet.
        REQUIRE_THROWS_AS(PacketVersion2({}, conn), std::length_error);
        REQUIRE_THROWS_WITH(
            PacketVersion2({}, conn),
            "Packet (0 bytes) is shorter than a v2.0 header (10 bytes).");
        // Packet short 1 byte.
        REQUIRE_THROWS_AS(
            PacketVersion2({1, 2, 3, 4, 5, 6, 7, 8, 9}, conn),
            std::length_error);
        REQUIRE_THROWS_WITH(
            PacketVersion2({1, 2, 3, 4, 5, 6, 7, 8, 9}, conn),
            "Packet (9 bytes) is shorter than a v2.0 header (10 bytes).");
    }
    SECTION("And ensures a complete header is given (with signature).")
    {
        // 0 length packet.
        REQUIRE_THROWS_AS(PacketVersion2({}, conn), std::length_error);
        REQUIRE_THROWS_WITH(
            PacketVersion2({}, conn),
            "Packet (0 bytes) is shorter than a v2.0 header (10 bytes).");
        // Packet short 1 byte.
        REQUIRE_THROWS_AS(
            PacketVersion2({1, 2, 3, 4, 5, 6, 7, 8, 9}, conn, true),
            std::length_error);
        REQUIRE_THROWS_WITH(
            PacketVersion2({1, 2, 3, 4, 5, 6, 7, 8, 9}, conn, true),
            "Packet (9 bytes) is shorter than a v2.0 header (10 bytes).");
    }
    SECTION("And ensures packets begins with the magic byte (0xFE).")
    {
        heartbeat.magic = 0xBC;
        REQUIRE_THROWS_AS(
            PacketVersion2(to_vector(heartbeat), conn), std::invalid_argument);
        REQUIRE_THROWS_WITH(
            PacketVersion2(to_vector(heartbeat), conn),
            "Invalid packet starting byte (0xBC), "
            "v2.0 packets should start with 0xFD.");
        ping.magic = 0xAD;
        REQUIRE_THROWS_AS(
            PacketVersion2(to_vector(ping), conn), std::invalid_argument);
        REQUIRE_THROWS_WITH(
            PacketVersion2(to_vector(ping), conn),
            "Invalid packet starting byte (0xAD), "
            "v2.0 packets should start with 0xFD.");
    }
    SECTION("And ensures the message ID is valid.")
    {
        ping.msgid = 255; // ID 255 is not currently valid.
        REQUIRE_THROWS_AS(
            PacketVersion2(to_vector(ping), conn), std::runtime_error);
        REQUIRE_THROWS_WITH(
            PacketVersion2(to_vector(ping), conn),
            "Invalid packet ID (#255).");
        ping.msgid = 5000; // ID 5000 is not currently valid.
        REQUIRE_THROWS_AS(
            PacketVersion2(to_vector(ping), conn), std::runtime_error);
        REQUIRE_THROWS_WITH(
            PacketVersion2(to_vector(ping), conn),
            "Invalid packet ID (#5000).");
    }
    SECTION("And ensures the packet is the correct length (without signature).")
    {
        // HEARTBEAT (no target system/component).
        auto heartbeat_data = to_vector(heartbeat);
        heartbeat_data.push_back(0x00);
        REQUIRE_THROWS_AS(
            PacketVersion2(heartbeat_data, conn), std::length_error);
        REQUIRE_THROWS_WITH(
            PacketVersion2(heartbeat_data, conn),
            "Packet is 22 bytes, should be 21 bytes.");
        // PING (with target system/component).
        auto ping_data = to_vector(ping);
        ping_data.pop_back();
        REQUIRE_THROWS_AS(PacketVersion2(ping_data, conn), std::length_error);
        REQUIRE_THROWS_WITH(
            PacketVersion2(ping_data, conn),
            "Packet is 25 bytes, should be 26 bytes.");
        // SET_MODE (target system only, no target component).
        auto set_mode_data = to_vector(set_mode);
        set_mode_data.push_back(0x00);
        REQUIRE_THROWS_AS(
            PacketVersion2(set_mode_data, conn), std::length_error);
        REQUIRE_THROWS_WITH(
            PacketVersion2(set_mode_data, conn),
            "Packet is 19 bytes, should be 18 bytes.");
        // PARAM_REQUEST_READ (trimmed out, 0.0, system/component).
        auto mission_set_current_data = to_vector(mission_set_current);
        mission_set_current_data.pop_back();
        REQUIRE_THROWS_AS(
            PacketVersion2(mission_set_current_data, conn), std::length_error);
        REQUIRE_THROWS_WITH(
            PacketVersion2(mission_set_current_data, conn),
            "Packet is 13 bytes, should be 14 bytes.");
        // ENCAPSULATED_DATA (longest packet).
        auto encapsulated_data_data = to_vector(encapsulated_data);
        encapsulated_data_data.push_back(0x00);
        REQUIRE_THROWS_AS(
            PacketVersion2(encapsulated_data_data, conn), std::length_error);
        REQUIRE_THROWS_WITH(
            PacketVersion2(encapsulated_data_data, conn),
            "Packet is 268 bytes, should be 267 bytes.");
        // PARAM_EXT_REQUEST_LIST (message ID beyond 255).
        auto param_ext_request_list_data = to_vector(param_ext_request_list);
        param_ext_request_list_data.pop_back();
        REQUIRE_THROWS_AS(
            PacketVersion2(param_ext_request_list_data, conn),
            std::length_error);
        REQUIRE_THROWS_WITH(
            PacketVersion2(param_ext_request_list_data, conn),
            "Packet is 13 bytes, should be 14 bytes.");
    }
    SECTION("And ensures the packet is the correct length (wit signature).")
    {
        // HEARTBEAT (no target system/component).
        auto heartbeat_data = to_vector(heartbeat, true);
        heartbeat_data.push_back(0x00);
        REQUIRE_THROWS_AS(
            PacketVersion2(heartbeat_data, conn), std::length_error);
        REQUIRE_THROWS_WITH(
            PacketVersion2(heartbeat_data, conn),
            "Packet is 35 bytes, should be 34 bytes.");
        // PING (with target system/component).
        auto ping_data = to_vector(ping, true);
        ping_data.pop_back();
        REQUIRE_THROWS_AS(PacketVersion2(ping_data, conn), std::length_error);
        REQUIRE_THROWS_WITH(
            PacketVersion2(ping_data, conn),
            "Packet is 38 bytes, should be 39 bytes.");
        // SET_MODE (target system only, no target component).
        auto set_mode_data = to_vector(set_mode, true);
        set_mode_data.push_back(0x00);
        REQUIRE_THROWS_AS(
            PacketVersion2(set_mode_data, conn), std::length_error);
        REQUIRE_THROWS_WITH(
            PacketVersion2(set_mode_data, conn),
            "Packet is 32 bytes, should be 31 bytes.");
        // PARAM_REQUEST_READ (trimmed out, 0.0, system/component).
        auto mission_set_current_data = to_vector(mission_set_current, true);
        mission_set_current_data.pop_back();
        REQUIRE_THROWS_AS(
            PacketVersion2(mission_set_current_data, conn), std::length_error);
        REQUIRE_THROWS_WITH(
            PacketVersion2(mission_set_current_data, conn),
            "Packet is 26 bytes, should be 27 bytes.");
        // ENCAPSULATED_DATA (longest packet).
        auto encapsulated_data_data = to_vector(encapsulated_data, true);
        encapsulated_data_data.push_back(0x00);
        REQUIRE_THROWS_AS(
            PacketVersion2(encapsulated_data_data, conn), std::length_error);
        REQUIRE_THROWS_WITH(
            PacketVersion2(encapsulated_data_data, conn),
            "Packet is 281 bytes, should be 280 bytes.");
        // PARAM_EXT_REQUEST_LIST (message ID beyond 255).
        auto param_ext_request_list_data =
            to_vector(param_ext_request_list, true);
        param_ext_request_list_data.pop_back();
        REQUIRE_THROWS_AS(
            PacketVersion2(param_ext_request_list_data, conn),
            std::length_error);
        REQUIRE_THROWS_WITH(
            PacketVersion2(param_ext_request_list_data, conn),
            "Packet is 26 bytes, should be 27 bytes.");
    }
}


TEST_CASE("PacketVersion2's contain raw packet data and make it accessible.",
          "[PacketVersion2]")
{
    auto heartbeat = to_vector(Heartbeat(), true);
    auto ping = to_vector(Ping());
    auto set_mode = to_vector(SetMode(), true);
    auto mission_set_current = to_vector(MissionSetCurrent());
    auto encapsulated_data = to_vector(EncapsulatedData(), true);
    auto param_ext_request_list = to_vector(ParamExtRequestList());
    auto conn = std::make_shared<ConnectionTestClass>();
    REQUIRE(PacketVersion2(heartbeat, conn).data() == heartbeat);
    REQUIRE(PacketVersion2(ping, conn).data() == ping);
    REQUIRE(PacketVersion2(set_mode, conn).data() == set_mode);
    REQUIRE(PacketVersion2(mission_set_current, conn).data() ==
            mission_set_current);
    REQUIRE(PacketVersion2(encapsulated_data, conn).data() ==
            encapsulated_data);
    REQUIRE(PacketVersion2(param_ext_request_list, conn).data() ==
            param_ext_request_list);
}


TEST_CASE("PacketVersion2's contain a weak_ptr to a connection.",
          "[PacketVersion2]")
{
    auto ping = to_vector(Ping());
    auto conn1 = std::make_shared<ConnectionTestClass>();
    auto conn2 = std::make_shared<ConnectionTestClass>();
    std::weak_ptr<ConnectionTestClass> empty_conn; // empty weak pointer
    REQUIRE(PacketVersion2(ping, conn1).connection().lock() == conn1);
    REQUIRE(PacketVersion2(ping, conn2).connection().lock() == conn2);
    REQUIRE(PacketVersion2(ping, empty_conn).connection().lock() ==
            empty_conn.lock());
    REQUIRE(PacketVersion2(ping, empty_conn).connection().lock() == nullptr);
    REQUIRE_FALSE(PacketVersion2(ping, conn1).connection().lock() == nullptr);
    REQUIRE_FALSE(PacketVersion2(ping, conn2).connection().lock() == nullptr);
    REQUIRE_FALSE(PacketVersion2(ping, conn1).connection().lock() == conn2);
    REQUIRE_FALSE(PacketVersion2(ping, conn2).connection().lock() == conn1);
    REQUIRE_FALSE(
        PacketVersion2(ping, empty_conn).connection().lock() == conn1);
    REQUIRE_FALSE(
        PacketVersion2(ping, empty_conn).connection().lock() == conn2);
}


TEST_CASE("PacketVersion2's have a priority.", "[PacketVersion2]")
{
    auto ping = to_vector(Ping());
    auto conn = std::make_shared<ConnectionTestClass>();
    SECTION("Which has a default value of 0.")
    {
        REQUIRE(PacketVersion2(ping, conn).priority() == 0);
    }
    SECTION("That can be set during construction.")
    {
        REQUIRE(PacketVersion2(ping, conn, -32768).priority() == -32768);
        REQUIRE(PacketVersion2(ping, conn, 0).priority() == 0);
        REQUIRE(PacketVersion2(ping, conn, 32767).priority() == 32767);

        // Loop over every 1000 priority values.
        for (auto i : boost::irange(-32768, 32768, 1000))
        {
            REQUIRE(PacketVersion2(ping, conn, i).priority() == i);
        }
    }
    SECTION("That can be set after construction.")
    {
        PacketVersion2 packet(ping, conn);
        REQUIRE(packet.priority() == 0);
        // -32768
        REQUIRE(packet.priority(-32768) == -32768);
        REQUIRE(packet.priority() == -32768);
        // 0
        REQUIRE(packet.priority(0) == 0);
        REQUIRE(packet.priority() == 0);
        // 32767
        REQUIRE(packet.priority(32767) == 32767);
        REQUIRE(packet.priority() == 32767);

        // Loop over every 1000 priority values.
        for (auto i : boost::irange(-32768, 32768, 1000))
        {
            REQUIRE(packet.priority(i) == i);
            REQUIRE(packet.priority() == i);
        }
    }
}


TEST_CASE("PacketVersion2's are copyable.", "[PacketVersion2]")
{
    auto heartbeat = to_vector(Heartbeat());
    auto ping = to_vector(Ping());
    auto conn1 = std::make_shared<ConnectionTestClass>();
    auto conn2 = std::make_shared<ConnectionTestClass>();
    PacketVersion2 a(heartbeat, conn1, 1);
    PacketVersion2 b(ping, conn2, 2);
    PacketVersion2 a_copy = a;
    PacketVersion2 b_copy(b);
    REQUIRE(&a != &a_copy);
    REQUIRE(a.data() == a_copy.data());
    REQUIRE(a.connection().lock() == a_copy.connection().lock());
    REQUIRE(a.priority() == a_copy.priority());
    REQUIRE(&b != &b_copy);
    REQUIRE(b.data() == b_copy.data());
    REQUIRE(b.connection().lock() == b_copy.connection().lock());
    REQUIRE(b.priority() == b_copy.priority());
}


TEST_CASE("PacketVersion2's are assignable.", "[PacketVersion2]")
{
    auto heartbeat = to_vector(Heartbeat());
    auto ping = to_vector(Ping(), true);
    auto conn = std::make_shared<ConnectionTestClass>();
    PacketVersion2 packet(heartbeat, conn, -10);
    REQUIRE(packet.data() == heartbeat);
    REQUIRE(packet.connection().lock() == conn);
    REQUIRE(packet.priority() == -10);
    packet = PacketVersion2(ping, std::weak_ptr<ConnectionTestClass>(), 10);
    REQUIRE(packet.data() == ping);
    REQUIRE(packet.connection().lock() == nullptr);
    REQUIRE(packet.priority() == 10);
}


TEST_CASE("PacketVersion2's have a version.", "[PacketVersion2]")
{
    auto heartbeat = to_vector(Heartbeat(), true);
    auto ping = to_vector(Ping());
    auto set_mode = to_vector(SetMode(), true);
    auto mission_set_current = to_vector(MissionSetCurrent());
    auto encapsulated_data = to_vector(EncapsulatedData(), true);
    auto param_ext_request_list = to_vector(ParamExtRequestList());
    auto conn = std::make_shared<ConnectionTestClass>();
    // All should read 0x0200 for v2.0.
    REQUIRE(PacketVersion2(heartbeat, conn).version() == 0x0200);
    REQUIRE(PacketVersion2(ping, conn).version() == 0x0200);
    REQUIRE(PacketVersion2(set_mode, conn).version() == 0x0200);
    REQUIRE(PacketVersion2(mission_set_current, conn).version() == 0x0200);
    REQUIRE(PacketVersion2(encapsulated_data, conn).version() == 0x0200);
    REQUIRE(PacketVersion2(param_ext_request_list, conn).version() == 0x0200);
}


TEST_CASE("PacketVersion2's have an ID.", "[PacketVersion2]")
{
    auto heartbeat = to_vector(Heartbeat(), true);
    auto ping = to_vector(Ping());
    auto set_mode = to_vector(SetMode(), true);
    auto mission_set_current = to_vector(MissionSetCurrent());
    auto encapsulated_data = to_vector(EncapsulatedData(), true);
    auto param_ext_request_list = to_vector(ParamExtRequestList());
    auto conn = std::make_shared<ConnectionTestClass>();
    REQUIRE(PacketVersion2(heartbeat, conn).id() == 0);
    REQUIRE(PacketVersion2(ping, conn).id() == 4);
    REQUIRE(PacketVersion2(set_mode, conn).id() == 11);
    REQUIRE(PacketVersion2(mission_set_current, conn).id() == 41);
    REQUIRE(PacketVersion2(encapsulated_data, conn).id() == 131);
    REQUIRE(PacketVersion2(param_ext_request_list, conn).id() == 321);
}


TEST_CASE("PacketVersion2's have a name.", "[PacketVersion2]")
{
    auto heartbeat = to_vector(Heartbeat(), true);
    auto ping = to_vector(Ping());
    auto set_mode = to_vector(SetMode(), true);
    auto mission_set_current = to_vector(MissionSetCurrent());
    auto encapsulated_data = to_vector(EncapsulatedData(), true);
    auto param_ext_request_list = to_vector(ParamExtRequestList());
    auto conn = std::make_shared<ConnectionTestClass>();
    REQUIRE(PacketVersion2(heartbeat, conn).name() == "HEARTBEAT");
    REQUIRE(PacketVersion2(ping, conn).name() == "PING");
    REQUIRE(PacketVersion2(set_mode, conn).name() == "SET_MODE");
    REQUIRE(PacketVersion2(mission_set_current, conn).name() ==
            "MISSION_SET_CURRENT");
    REQUIRE(PacketVersion2(encapsulated_data, conn).name() ==
            "ENCAPSULATED_DATA");
    REQUIRE(PacketVersion2(param_ext_request_list, conn).name() ==
            "PARAM_EXT_REQUEST_LIST");
}


TEST_CASE("PacketVersion2's have a source address.", "[PacketVersion2]")
{
    auto heartbeat = to_vector(Heartbeat(), true);
    auto ping = to_vector(Ping());
    auto set_mode = to_vector(SetMode(), true);
    auto mission_set_current = to_vector(MissionSetCurrent());
    auto encapsulated_data = to_vector(EncapsulatedData(), true);
    auto param_ext_request_list = to_vector(ParamExtRequestList());
    auto conn = std::make_shared<ConnectionTestClass>();
    REQUIRE(PacketVersion2(heartbeat, conn).source() == MAVAddress("1.0"));
    REQUIRE(PacketVersion2(ping, conn).source() == MAVAddress("60.40"));
    REQUIRE(PacketVersion2(set_mode, conn).source() == MAVAddress("70.30"));
    REQUIRE(PacketVersion2(mission_set_current, conn).source() ==
            MAVAddress("80.20"));
    REQUIRE(PacketVersion2(encapsulated_data, conn).source() ==
            MAVAddress("255.1"));
    REQUIRE(PacketVersion2(param_ext_request_list, conn).source() ==
            MAVAddress("1.255"));
}


TEST_CASE("PacketVersion2's optionally have a destination address.",
          "[PacketVersion2]")
{
    auto heartbeat = to_vector(Heartbeat(), true);
    auto ping = to_vector(Ping());
    auto set_mode = to_vector(SetMode(), true);
    auto mission_set_current = to_vector(MissionSetCurrent());
    auto encapsulated_data = to_vector(EncapsulatedData(), true);
    auto param_ext_request_list = to_vector(ParamExtRequestList());
    auto conn = std::make_shared<ConnectionTestClass>();
    REQUIRE_THROWS_AS(
        PacketVersion2(heartbeat, conn).dest().value(),
        std::bad_optional_access);
    REQUIRE(PacketVersion2(ping, conn).dest().value() == MAVAddress("255.23"));
    REQUIRE(PacketVersion2(set_mode, conn).dest().value() ==
            MAVAddress("123.0"));
    REQUIRE(PacketVersion2(mission_set_current, conn).dest().value() ==
            MAVAddress("0.0"));
    REQUIRE_THROWS_AS(
        PacketVersion2(encapsulated_data, conn).dest().value(),
        std::bad_optional_access);
    REQUIRE(PacketVersion2(param_ext_request_list, conn).dest().value() ==
            MAVAddress("32.64"));
}


TEST_CASE("PacketVersion2's are printable.", "[PacketVersion2]")
{
    auto heartbeat = to_vector(Heartbeat(), true);
    auto ping = to_vector(Ping());
    auto set_mode = to_vector(SetMode(), true);
    auto mission_set_current = to_vector(MissionSetCurrent());
    auto encapsulated_data = to_vector(EncapsulatedData(), true);
    auto param_ext_request_list = to_vector(ParamExtRequestList());
    auto conn = std::make_shared<ConnectionTestClass>();
    REQUIRE(str(PacketVersion2(heartbeat, conn)) ==
            "HEARTBEAT (#0) from 1.0 (v2.0)");
    REQUIRE(str(PacketVersion2(ping, conn)) ==
            "PING (#4) from 60.40 to 255.23 (v2.0)");
    REQUIRE(str(PacketVersion2(set_mode, conn)) ==
            "SET_MODE (#11) from 70.30 to 123.0 (v2.0)");
    REQUIRE(str(PacketVersion2(mission_set_current, conn)) ==
            "MISSION_SET_CURRENT (#41) from 80.20 to 0.0 (v2.0)");
    REQUIRE(str(PacketVersion2(encapsulated_data, conn)) ==
            "ENCAPSULATED_DATA (#131) from 255.1 (v2.0)");
    REQUIRE(str(PacketVersion2(param_ext_request_list, conn)) ==
            "PARAM_EXT_REQUEST_LIST (#321) from 1.255 to 32.64 (v2.0)");
}
