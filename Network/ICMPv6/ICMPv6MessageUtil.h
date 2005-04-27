//
// Copyright (C) 2005 Andras Varga
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//

#ifndef ICMPV6MESSAGEUTIL_H
#define ICMPV6MESSAGEUTIL_H

#include "ICMPv6Message_m.h"

inline
ICMPv6Message *createICMPv6Message(const char *name, ICMPv6Type type, int code,
                                   cMessage *contents, int optInfo=0)
{
    ICMPv6Message *icmpMsg = new ICMPv6Message(name);
    icmpMsg->setType(type);
    icmpMsg->setCode(code);
    icmpMsg->setOptInfo(optInfo);
    icmpMsg->setLength(ICMPv6_HEADER_OCTETLENGTH*BITS);
    icmpMsg->encapsulate(contents);
    return icmpMsg;
}

inline
bool isICMPv6Error(ICMPv6Type type) {return (int)type < 128;}


#endif
