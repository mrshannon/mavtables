// MAVLink router and firewall.
// Copyright (C) 2017  Michael R. Shannon <mrshannon.aerospace@gmail.com>
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


#ifndef DNSLOOKUPERROR_HPP_
#define DNSLOOKUPERROR_HPP_


#include <string>
#include <exception>


/** Exception type for a failed DNS lookup.
 */
class DNSLookupError : public std::exception
{
    public:
        DNSLookupError(std::string url);
        const char *what() const noexcept;

    private:
        std::string message_;
};


#endif // DNSLOOKUPERROR_HPP_
