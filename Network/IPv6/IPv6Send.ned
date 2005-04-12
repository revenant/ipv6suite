//
// Copyright (C) 2000 Institut fuer Telematik, Universitaet Karlsruhe
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
simple IPv6Send
    parameters:
        procDelay,
        multicastTimeToLive,
        numOfTransportProt : numeric const;
    gates:
        in: ICMPIn;
        in: tunnelIn;
        in: transportIn[];
        out: routingOut;
endsimple

