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
#include <stdexcept>

#include <config_grammar.hpp>

#include "ConfigFileParser.hpp"


/** Construct a configuration parser from a file.
 *
 *  \param filename The path to the configuration file to parse.
 */
ConfigFileParser::ConfigFileParser(std::string filename)
    : in_(filename)
{
    root_ = config::parse(in_);
    if (root_ == nullptr)
    {
        // It is technically impossible to reach this because all possible
        // errors should produce a parse_error.
        // LCOV_EXCL_START
        throw std::runtime_error(
            "Unexpected error while parsing configuration file.");
        // LCOV_EXCL_STOP
    }
}


ConfigFileParser::~ConfigFileParser()
{
}
