// $Header: /home/cvs/IPv6Suite/IPv6SuiteWithINET/Network/IPv6/Attic/ICMPv6.ned,v 1.2 2005/02/16 06:23:48 andras Exp $
//
// Copyright (C) 2001 Monash University, Australia
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



//
// Multicast Listener Discovery (TBD)
//
simple MLD
//   parameters:
//        Group,
//        Source;
   gates:
        in: MLDIn;
    in: ReportMessage;
    in: DoneMessage;
        out:MLDOut;
endsimple


//
// Module for processing of ICMPv6 messages.
//
// Processing of received ICMP messages from network nodes and local modules.
//
//#FIXME gate descriptions needed...
//
simple ICMPv6Core
    parameters:
        IPForward : bool,
        procdelay : numeric const,
        numOfPorts : numeric,
    icmpRecordRequests: bool,
    icmpRecordStart: numeric,
    replyToICMPRequests: bool;
    gates:
        in: preRoutingIn;
        in: routingIn;
        in: localIn;
        in: pingIn;
        in: fragmentationIn;
        in: ipOutputIn[];
        in: addrReslnIn;
        in: localErrorIn;
        out: pingOut;
        out: sendOut;
        out: errorOut;
        out: NDiscOut;
    out: MLDOut;
endsimple


//
// Module housing the Neighbour Discovery state machine.
//
// Provide a client interface to Neighbour Discovery state machine for
// ICMP Layer to use.  Each interface has its own configurable state
// i.e.  some interfaces act as routers by advertising while others
// are by default hosts.
//
//#FIXME gate descriptions needed...
//
simple NeighbourDiscovery
    parameters:
        IPForward : bool;
    gates:
        in: NDiscIn;
        in: mobilityDIn;
        out: sendOut;
        out: errorOut;
        out: routingOut;
        out: outputOut[];
        out: mobilityDOut;
endsimple



//
// Combine ICMPv6Core and NeighbourDiscovery
//
// - ICMP output messages into one gate
// - errors and notifications into one gate
//
//#FIXME gate descriptions needed...
//
simple ICMPv6Combine
    gates:
        in: xcoreSendOut;
        in: xcoreErrorOut;
        in: xndSendOut;
        in: xndErrorOut;
        out: sendOut;
        out: errorOut;
endsimple


//
// ICMPv6 implementation.
//
module ICMPv6
    parameters:
        numOfPorts : numeric,
        procdelay : numeric const;
    gates:
        in: preRoutingIn;
        in: routingIn;
        in: localIn;
        in: pingIn;
        in: fragmentationIn;
        in: ipOutputIn[];
        in: addrReslnIn;
        in: localErrorIn;
        out: pingOut;
        out: sendOut;
        out: errorOut;
        out: outputOut[];
        out: routingOut;
        out: mldOut;
    submodules:
        icmpv6Core: ICMPv6Core;
            parameters:
                procdelay = procdelay,
                numOfPorts = numOfPorts,
                IPForward = ancestor IPForward;
            gatesizes:
                ipOutputIn[numOfPorts];
            display: "p=207,129;b=48,24";
        nd: NeighbourDiscovery;
            parameters:
                IPForward = ancestor IPForward;
            gatesizes:
                outputOut[numOfPorts];
            display: "p=64,49;b=40,24";
        combine: ICMPv6Combine;
            display: "p=149,247;b=40,24";
    mld: MLD;
    connections nocheck:
        preRoutingIn --> icmpv6Core.preRoutingIn;
        routingIn --> icmpv6Core.routingIn;
        localIn --> icmpv6Core.localIn;
        pingIn --> icmpv6Core.pingIn;
        fragmentationIn --> icmpv6Core.fragmentationIn;
        addrReslnIn --> icmpv6Core.addrReslnIn;
        localErrorIn --> icmpv6Core.localErrorIn;
        icmpv6Core.errorOut --> combine.xcoreErrorOut;
        icmpv6Core.sendOut --> combine.xcoreSendOut;
        icmpv6Core.NDiscOut --> nd.NDiscIn;
        nd.errorOut --> combine.xndErrorOut;
        nd.sendOut --> combine.xndSendOut;
        nd.routingOut --> routingOut;

        pingOut <-- icmpv6Core.pingOut;
        errorOut <-- combine.errorOut;
        sendOut <-- combine.sendOut;

    mld.MLDIn<-- icmpv6Core.MLDOut;
    mld.MLDOut-->mldOut;

        for i=0..numOfPorts-1 do
           nd.outputOut[i] --> outputOut[i];
        endfor;
endmodule

