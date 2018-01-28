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


#ifndef ACTION_HPP_
#define ACTION_HPP_


#include <ostream>

#include "Packet.hpp"
#include "MAVAddress.hpp"


class Action
{
    protected:
        /** Print the action to the given output stream.
         *
         *  \param os The output stream to print to.
         *  \return The output stream.
         */
        virtual std::ostream &print_(std::ostream &os) const = 0;

    public:
        virtual ~Action();
        /** Determin whether packet is allowed to be sent to given address.
         *
         *  \param packet The packet to determine whether to allow or not.
         *  \param address The address the \p packet will be sent out on if the
         *      action allows it.
         *  \retval true The packet is allowed to be sent to \p address.
         *  \retval false The packet is not allowed to be sent to \p address.
         */
        virtual bool action(
            const Packet &packet, const MAVAddress &address) = 0;

        friend std::ostream &operator<<(std::ostream &os, const Action &action);
};


std::ostream &operator<<(std::ostream &os, const Action &action);


#endif // ACTION_HPP_