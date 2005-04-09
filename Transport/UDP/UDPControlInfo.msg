//
// Copyright (C) 2000 Institut fuer Telematik, Universitaet Karlsruhe
// Copyright (C) 2004 Andras Varga
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


cplusplus {{
#include "IPvXAddress.h"
}}

class noncobject IPvXAddress;


//
// Control info for sending/receiving data via UDP. Travels between
// application and UDPProcessing.
//
class UDPControlInfo
{
    fields:
        IPvXAddress srcAddr;  // source IP or IPv6 address
        IPvXAddress destAddr; // destination IP or IPv6 address
        int srcPort;   // UDP source port in packet
        int destPort;  // UDP destination port in packet
        int inputPort; // gate index of interface on which the packet was received
};

