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


#ifndef UDPINTERFACE_HPP_
#define UDPINTERFACE_HPP_


#include <chrono>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <vector>

#include "Connection.hpp"
#include "ConnectionPool.hpp"
#include "Interface.hpp"
#include "IPAddress.hpp"
#include "PacketParser.hpp"
#include "UDPSocket.hpp"




class UDPInterface : public Interface
{
    private:
        // Variables.
        std::unique_ptr<UDPSocket> socket_;
        std::shared_ptr<ConnectionPool> connection_pool_;
        std::function<std::unique_ptr<Connection>(void)> connection_factory_;
        IPAddress last_ip_address_;
        std::map<IPAddress, std::shared_ptr<Connection>> connections_;
        PacketParser parser_;
        // Methods
        void update_connections_(
            const MAVAddress &mav_address, const IPAddress &ip_address);

    public:
        UDPInterface(
            std::unique_ptr<UDPSocket> socket,
            std::shared_ptr<ConnectionPool> connection_pool,
            std::function<std::unique_ptr<Connection>(void)>
                connection_factory);
        void send_packet(
            const std::chrono::microseconds &timeout =
                std::chrono::microseconds(100000)) final;
        void receive_packet(
            const std::chrono::microseconds &timeout =
                std::chrono::microseconds(100000)) final;
};


#endif // UDPINTERFACE_HPP_