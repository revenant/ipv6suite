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


enum IPProtocolId;
class noncobject IPv6Address_;

cplusplus {{
#include "IPAddress.h"
#include "IPvXAddress.h" // FIXME this contains IPv6Address_...
}}


//
// Control information for sending/receiving packets over \IPv6.
//
// To send a packet over \IPv6, fill in an IPv6ControlInfo object,
// attach it to the packet with the C++ method setControlInfo(),
// the send it to the IPv6 module.
//
// When IPv6 delivers packets to higher layers, it also attaches
// similar IP6ControlInfo objects to them.
//
// When sending, the following fields are required:
// - protocol: a value from IPProtocolId
// - destAddr
//
// Optional fields:
// - srcAddr: it will be set to the address of the outgoing interface
// - timeToLive
//
class IPv6ControlInfo
{
    properties:
        omitGetVerb=true;
    fields:
        int protocol enum(IPProtocolId);
        IPv6Address_ destAddr;
        IPv6Address_ srcAddr;
        short timeToLive;
};


