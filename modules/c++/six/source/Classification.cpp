/* =========================================================================
 * This file is part of six-c++ 
 * =========================================================================
 * 
 * (C) Copyright 2004 - 2009, General Dynamics - Advanced Information Systems
 *
 * six-c++ is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public 
 * License along with this program; If not, 
 * see <http://www.gnu.org/licenses/>.
 *
 */
#include "six/Classification.h"

using namespace six;

std::ostream& operator<<(std::ostream& os, const Classification& c)
{
    os << "Classification: " << std::endl;
    os << "------------------------------------" << std::endl;

    c.put(os);

    os << "File specific options: " << std::endl;
    for (Options::ParameterIter iter = c.fileOptions.begin();
         iter != c.fileOptions.end();
         ++iter)
    {
        os << "    {" << iter->first << "}  " << iter->second.str()
           << std::endl;
    }
    return os;
}

