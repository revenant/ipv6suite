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

import "NF_IP_LOCAL_OUT";

//
// Encapsulates packets received from higher layers into IPv6 datagrams.
//
// Responsibilities:
//  - receive IPInterfacePacket from Transport layer or ICMP
//    or Tunneling (IP tunneled datagram)
//  - take out control information
//#FIXME what does that mean?
//  - encapsulate data packet in IPv6 datagram
//  - set version
//  - set hoplimit according to router's advertised value
//  - set Protocol to received value
//#FIXME received how?
//  - set destination address to received value
//#FIXME received how?
//  - send datagram to Routing
//  - if IPInterfacePacket is invalid (e.g. invalid source address),
//    it is thrown away without feedback
//
// @author Johnny Lai
// based on IPSendCore by Jochen Reber
//
simple IPv6SendCore
    parameters:
        procdelay,
        multicastTimeToLive,
        numOfTransportProt : numeric const;
    gates:
        in: ICMPIn;
        in: tunnelIn;
        in: transportIn[];
        out: routingOut;
endsimple


//
// Encapsulates packets received from higher layers into IPv6 datagrams.
//
//#FIXME following description appears to come from original IPv4 suite -- does it still apply to IPv6?
//
// Responsibilities:
//  - receive packet from Transport layer or ICMP or Tunneling (IP tunneled datagram)
//  - encapsulate in IP datagram
//  - set version
//  - set TOS (currently constant 0!!!)
//  - set TTL to constant value
//  - choose and set fragmentation identifier
//  - set fragment offset = 0, more fragments = 0 (fragmentation occurs
//    in IPFragmentation)
//  - set "don't Fragment" bit to received value (default 0)
//  - set Protocol to received value
//#FIXME received how?
//  - set destination address to received value
//#FIXME received how?
//  - send datagram to Routing
//
// author: Jochen Reber
// date: 5.00, 6.00
//
module IPv6Send
    parameters:
        procdelay,
        multicastTimeToLive,
        numOfTransportProt : numeric const;
    gates:
        in: ICMPIn;
        in: tunnelIn;
        in: transportIn[];
        out: routingOut;
    submodules:
        core: IPv6SendCore;
            parameters:
                procdelay = procdelay,
                multicastTimeToLive = multicastTimeToLive,
                numOfTransportProt = numOfTransportProt;
            gatesizes:
                transportIn[numOfTransportProt];
            display: "p=125,212;b=40,24,rect";

    connections nocheck:
        ICMPIn --> core.ICMPIn;
        tunnelIn --> core.tunnelIn;
        for i=0..numOfTransportProt-1 do
            transportIn[i] --> core.transportIn[i];
        endfor;
        core.routingOut --> routingOut;

endmodule


