// $Header: /home/cvs/IPv6Suite/IPv6SuiteWithINET/Network/IPv6/IPv6Forward.ned,v 1.2 2005/02/16 06:23:48 andras Exp $
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


import "NF_IP_FORWARD";


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
simple IPv6ForwardCore
    parameters:
        procdelay : numeric const,
        routingInfoDisplay: bool ;
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

//
// IPv6 packet forwarding.
//
// Responsibilities:
// - receive correct IP datagram
// - if source routing option is on, use next source addr. as dest. addr.
// - map IP address on output port, use static routing table
// - if destination address is not in routing table, throw datagram away
//   and notify ICMP
// - process record route and timestamp options, if applicable
// - send to localDeliver if dest. addr. = 127.0.0.1 or
//   dest. addr. = NetworkCardAddr.[]
// - send datagram with Multicast addr. to Multicast module
// - otherwise, send to Fragmentation module
//
// author: Johnny Lai
//
module IPv6Forward
    parameters:
    procdelay : numeric const;
    gates:
        in: preRoutingIn;
        in: sendIn;
    in: tunnelExit;
    in: localIn; //From routing header
    in: mobilityIn;
    in: ICMPIn;
    in: MLDIn;
        out: localOut;
        out: fragmentationOut;
        out: multicastOut;
        out: errorOut;
    out: pendingQueueOut;
    out: tunnelEntry;
    submodules:
        core: IPv6ForwardCore;
        parameters:
        procdelay = procdelay;
            display: "p=131,186;b=40,24,rect";
    connections:
        preRoutingIn --> core.preRoutingIn;
        sendIn --> core.sendIn;
    tunnelExit --> core.tunnelExit;
    localIn --> core.localIn;
    mobilityIn --> core.mobilityIn;
        core.localOut --> localOut;
        core.fragmentationOut --> fragmentationOut;
        core.multicastOut --> multicastOut;
        core.errorOut --> errorOut;
    core.pendingQueueOut --> pendingQueueOut;
    core.tunnelEntry --> tunnelEntry;
    ICMPIn --> core.ICMPIn;
    MLDIn --> core.MLDIn;
    display: "p=10,10;b=260,235,rect";
endmodule


