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
// Implementation of forwarding via table lookups
//
// Responsibilities:
//  - receive valid IPv6 datagram
//  - send datagram with Multicast addr. to Multicast module
//  - forward to IPv6Encapsulation module if next hop is a tunnel entry point
//  - drop datagram and notify ICMP if dest. addr not in forwarding table
//  - send to local Deliver if dest. addr. = 127.0.0.1
//    or dest. addr. = NetworkCardAddr.[]; otherwise, send to Fragmentation module
//  - check hopLimit and drop or decrement at end of routing header proc.
//
// ICMP error messages
//  - ICMP Time exceeded when hopLimit of packet is at 0
//
// @author Johnny Lai
// @date 28/08/01
//
simple IPv6Forward
    parameters:
        procdelay : numeric const,
        routingInfoDisplay: bool;
    gates:
        in: mobilityIn;
        in: preRoutingIn;
        in: sendIn;
        in: tunnelExit;
        in: localIn; //From routing header
        in: ICMPIn;
        in: MLDIn;
        out: localOut;
        out: fragmentationOut;
        out: multicastOut;
        out: errorOut;
        out: pendingQueueOut;
        out: tunnelEntry;
endsimple


