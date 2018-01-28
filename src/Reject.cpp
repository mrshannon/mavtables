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


#include <ostream>

#include "Packet.hpp"
#include "MAVAddress.hpp"
#include "Reject.hpp"


/** \copydoc Action::print_(std::ostream &os) const
 *
 *  Prints `"reject"`.
 */
std::ostream &Reject::print_(std::ostream &os) const
{
    os << "reject";
    return os;
}


/** \copydoc Action::action(const Packet &, const MAVAddress &)
 *
 *  The Reject class always returns false.
 */
bool Reject::action(const Packet &packet, const MAVAddress &address)
{
    (void)packet;
    (void)address;
    return false;
}