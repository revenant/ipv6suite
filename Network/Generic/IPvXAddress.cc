//
// Copyright (C) 2005 Andras Varga
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//

#include <iostream>
#include "IPvXAddress.h"

std::string IPv6Address_::str()
{
    std::stringstream os;
    //os << hex;
    os << (d[0]>>16) << ":" << (d[0]&0xffff) << ":"
       << (d[1]>>16) << ":" << (d[1]&0xffff) << ":"
       << (d[2]>>16) << ":" << (d[2]&0xffff) << ":"
       << (d[3]>>16) << ":" << (d[3]&0xffff);
    return os.str();
}

