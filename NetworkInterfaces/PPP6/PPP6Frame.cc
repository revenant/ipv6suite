// $Header: /home/cvs/IPv6Suite/IPv6SuiteWithINET/NetworkInterfaces/PPP6/PPP6Frame.cc,v 1.1 2005/02/12 07:17:55 andras Exp $
//
// Copyright (C) 2000 Institut fuer Telematik, Universitaet Karlsruhe
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

/*
    file: PPP6Frame .cc
    Purpose: PPP frame format definition Implementation
    author: Jochen Reber
*/


#include <omnetpp.h>
#include <string.h>

#include "PPP6Frame.h"

// constructors
PPP6Frame::PPP6Frame(): cPacket()
{
    setLength(8 * PPP_HEADER_LENGTH);
    _protocol = PPP_PROT_UNDEF;
}

PPP6Frame::PPP6Frame(const PPP6Frame& p)
{
    setName( p.name() );
    operator=(p);
}

// assignment operator
PPP6Frame& PPP6Frame::operator=(const PPP6Frame& p)
{
    cPacket::operator=(p);
    _protocol = p._protocol;
    destAddr = p.destAddr;
    return *this;
}

/* encapsulate a packet of type cPacket of the Network Layer;
    protocol set by default to IP;
    assumes that networkPacket->length() is
    length of transport packet in bits
    adds to it the PPP header length in bits */
void PPP6Frame::encapsulate(cPacket *networkPacket)
{
    cPacket::encapsulate(networkPacket);

    _protocol = PPP_PROT_IP;
}


