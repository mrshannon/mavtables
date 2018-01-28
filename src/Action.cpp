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

#include "Action.hpp"


Action::~Action()
{
}


/** Print the given action to the given output stream.
 *
 *  Some examples are:
 *      - `reject`
 *      - `accept`
 *      - `call gcs_in`
 *      - `goto autopilot_out`
 *
 *  \relates Action
 *  \param os The output stream to print to.
 *  \param action The MAVLink packet to print.
 *  \return The output stream.
 */
std::ostream &operator<<(std::ostream &os, const Action &action)
{
    return action.print_(os);
}