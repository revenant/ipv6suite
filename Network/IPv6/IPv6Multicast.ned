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


//
// IPv6 Multicast
//
// Responsibilities:
//  - receive datagram with multicast address from routing;
//  - duplicate datagram if it is sent to more than one output port
//  - map multicast address on output port, use multicast routing table
//  - send copy to  local deliver, if NetworkCardAddr.[] is part of multicast address
//  - if entry in multicast routing table requires tunneling, send to Tunneling module
//  - otherwise send to Fragmentation module
//  - receive IGMP message from LocalDeliver
//  - update multicast routing table
//
// @author Johnny Lai
// Based on IPMulticast by Jochen Reber
//
simple IPv6Multicast
    parameters:
        procdelay: numeric const;
    gates:
        in: localIn, routingIn, ProcessorManagerIn;
        out: localOut, fragmentationOut, tunnelOut;
endsimple

