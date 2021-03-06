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


#ifndef APP_HPP_
#define APP_HPP_


#include <memory>
#include <vector>

#include "Interface.hpp"
#include "InterfaceThreader.hpp"


/** The mavtables application class.
 */
class App
{
    public:
        App(std::vector<std::unique_ptr<Interface>> interfaces);
        void run();

    private:
        std::vector<std::unique_ptr<InterfaceThreader>> threaders_;
};


#endif // APP_HPP_
