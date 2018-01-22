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


#include <string>
#include <vector>
#include <memory>
#include <sstream>
#include <cstdint>
#include <cstdlib>
#include <stdexcept>

extern "C"
{
#include "mavlink.h"
}
#include "Connection.hpp"
#include "PacketVersion1.hpp"


/** \copydoc Packet::Packet(std::vector<uint8_t>,std::weak_ptr<Connection>,int)
 *
 *  \throws std::invalid_argument If packet data does not start with magic byte
 *      (0xFE).
 *  \throws std::length_error If packet data is not of correct length.
 */
PacketVersion1::PacketVersion1(
    std::vector<uint8_t> data,
    std::weak_ptr<Connection> connection,
    int priority)
    : Packet(std::move(data), std::move(connection), priority)
{
    // Check that a complete header was given (including magic number).
    if (this->data().size() < (MAVLINK_CORE_HEADER_MAVLINK1_LEN + 1))
    {
        throw std::length_error(
            "Packet (" + std::to_string(this->data().size()) +
            " bytes) is shorter than a v1.0 header (" +
            std::to_string(MAVLINK_CORE_HEADER_MAVLINK1_LEN + 1) + " bytes).");
    }

    // Verify the magic number.
    if (header_()->magic != MAVLINK_STX_MAVLINK1)
    {
        std::stringstream ss;
        ss << "Invalid packet starting byte (0x"
           << std::uppercase << std::hex
           << static_cast<unsigned int>(header_()->magic)
           << std::nouppercase << "), v1.0 packets should start with 0x"
           << std::uppercase << std::hex << MAVLINK_STX_MAVLINK1
           << std::nouppercase << ".";
        throw std::invalid_argument(ss.str());
    }

    // Verify the message ID.
    if (mavlink_get_message_info_by_id(header_()->msgid) == nullptr)
    {
        throw std::runtime_error(
            "Invalid packet ID (#" + std::to_string(header_()->msgid) + ").");
    }

    // Ensure a complete packet was given.
    size_t expected_length = 1 + MAVLINK_CORE_HEADER_MAVLINK1_LEN +
                             header_()->len + MAVLINK_NUM_CHECKSUM_BYTES;

    if (this->data().size() != expected_length)
    {
        throw std::length_error(
            "Packet is " + std::to_string(this->data().size()) +
            " bytes, should be " + std::to_string(expected_length) + " bytes.");
    }
}


// Return pointer to the header structure.
const struct mavlink_packet_version1_header *PacketVersion1::header_() const
{
    return reinterpret_cast<const struct mavlink_packet_version1_header *>
           (&(data()[0]));
}


/** \copydoc Packet::version()
 *
 *  \returns 0x0100 (v1.0)
 *  \complexity \f$O(1)\f$
 */
unsigned int PacketVersion1::version() const
{
    return 0x0100;
}


/** \copydoc Packet::id()
 *
 *  \complexity \f$O(1)\f$
*/
unsigned long PacketVersion1::id() const
{
    return header_()->msgid;
}


/** \copydoc Packet::name()
 *
 *  \complexity \f$O(log(n))\f$ where \f$n\f$ is the total number of MAVLink
 *      messages.
 *  \throws std::runtime_error If the packet data has an invalid ID.
 */
std::string PacketVersion1::name() const
{
    if (const mavlink_message_info_t *msg_info =
                mavlink_get_message_info_by_id(header_()->msgid))
    {
        return std::string(msg_info->name);
    }

    // There should never be any way to reach this point since the message ID
    // was checked in the constructor.  It is here just in case the MAVLink C
    // library has an error in it.
    throw std::runtime_error(
        "Invalid packet ID (#" + std::to_string(header_()->msgid) + ").");
}


/** \copydoc Packet::source()
 *
 *  \complexity \f$O(1)\f$
 */
MAVAddress PacketVersion1::source() const
{
    return MAVAddress(header_()->sysid, header_()->compid);
}


/** \copydoc Packet::dest()
 *
 *  \complexity \f$O(1)\f$
 *  \thanks The [mavlink-router](https://github.com/intel/mavlink-router)
 *      project for an example of how to extract the destination address.
 *
 *  \throws std::runtime_error If the packet data has an invalid ID.
 */
std::optional<MAVAddress> PacketVersion1::dest() const
{
    if (const mavlink_msg_entry_t *msg_entry = mavlink_get_msg_entry(
                header_()->msgid))
    {
        int dest_system = -1;
        int dest_component = 0;

        // Extract destination system.
        if (msg_entry->flags & MAV_MSG_ENTRY_FLAG_HAVE_TARGET_SYSTEM)
        {
            // target_system_ofs is offset from start of payload
            size_t offset = msg_entry->target_system_ofs + sizeof(
                                mavlink_packet_version1_header);
            dest_system = data()[offset];
        }

        // Extract destination component.
        if (msg_entry->flags & MAV_MSG_ENTRY_FLAG_HAVE_TARGET_COMPONENT)
        {
            // target_compoent_ofs is offset from start of payload
            size_t offset = msg_entry->target_component_ofs + sizeof(
                                mavlink_packet_version1_header);
            dest_component = data()[offset];
        }

        // Construct MAVLink address.
        if (dest_system >= 0 && dest_component >= 0)
        {
            return MAVAddress(static_cast<unsigned int>(dest_system),
                              static_cast<unsigned int>(dest_component));
        }

        // No destination address.
        return {};
    }

    // There should never be any way to reach this point since the message ID
    // was checked in the constructor.  It is here just in case the MAVLink C
    // library has an error in it.
    throw std::runtime_error(
        "Invalid packet ID (#" + std::to_string(header_()->msgid) + ").");
}
