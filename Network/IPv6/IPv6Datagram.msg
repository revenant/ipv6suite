//
// Copyright (C) 2001, 2004 CTIE, Monash University
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


cplusplus {{
#include "ipv6_addr.h"
}}

class noncobject ipv6_addr;

enum IPProtocolId;

//
// IPv6 datagram. Very few fields are here -- rest is in the customized C++
// implementation :-/
//
message IPv6Datagram
{
    properties:
        customize=true;
        omitGetVerb = true;

    fields:
        int inputPort = -1;
        int outputPort = -1;
        short hopLimit = 0;
        int transportProtocol enum (IPProtocolId);
        ipv6_addr srcAddress;
        ipv6_addr destAddress;
        unsigned int flowLabel;
        unsigned int trafficClass;
        //supposed to be dest opt
        short encapLimit = 16;
};

