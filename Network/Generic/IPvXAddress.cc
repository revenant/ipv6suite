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


bool IPv6Address_::tryParse(const char *addr)
{
    // this impl is based on c_ipv6_addr(const char *)
    // FIXME TBD doesn't understand "::" notation!
    if (!addr)
        return false;

    std::stringstream is(addr);

    unsigned int octals[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    char sep;

    try
    {
        int i;
        for (i=0; i<8; i++)
        {
            is >> std::hex >> octals[i];
            if (is.eof())
                break;
            is >> sep;
            if (sep != ':')
                return false;
        }
        if (i!=7)
            return false; // address incomplete or trailing ":"
    }
    catch (...)
    {
        return false;
    }

    for (unsigned int i=0; i<4; i++)
        d[i] = (octals[i*2]<<16) + octals[2*i + 1];

    return true;
}

void IPv6Address_::set(const char *addr)
{
    if (!tryParse(addr))
        throw new cRuntimeError("IPv6Address_: cannot interpret address string `%s'", addr);
}

std::string IPv6Address_::str() const
{
    std::stringstream os;
    os << std::hex;
    os << (d[0]>>16) << ":" << (d[0]&0xffff) << ":"
       << (d[1]>>16) << ":" << (d[1]&0xffff) << ":"
       << (d[2]>>16) << ":" << (d[2]&0xffff) << ":"
       << (d[3]>>16) << ":" << (d[3]&0xffff);
    return os.str();
}

//----

bool IPvXAddress::tryParse(const char *addr)
{
    // try as IPv4
    if (IPAddress::isWellFormed(addr))
    {
        set(IPAddress(addr));
        return true;
    }

    // try as IPv6
    IPv6Address_ ipv6;
    if (ipv6.tryParse(addr))
    {
        set(ipv6);
        return true;
    }

    // no luck
    return false;
}


