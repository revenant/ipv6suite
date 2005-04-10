//
// Copyright (C) 2000 Institut fuer Telematik, Universitaet Karlsruhe
// Copyright (C) 2001 CTIE, Monash University
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


import
    "IPv6PreRouting",
    "IPv6Forward",
    "IPv6LocalDeliver",
    "IPv6Multicast",
    "ICMPv6",
    "IPv6Send",
    "IPv6Fragmentation",
    "IPv6Encapsulation",
    "IPv6Output",
    "IPv6Mobility",
    "RoutingTable6",
    "AddressResolution";


//
// Network layer (L3) protocols. Contains ICMP and IP with multicast,
// tunnelling, fragmentation and mobility support.
//
// Can be configured via several hooks.
//
// @author: Johnny Lai
// @date: 4/9/01
// based on IPProcessing.ned by Jochen Reber
//
module IPv6Processing
    parameters:
        // other parameters
        multicastTimeToLive : numeric,
        numOfPorts : numeric,
        numOfTransportProt : numeric const,
        fragmentTimeout : string;
    gates:
        in: ICMPIn;
        in: transportIn[];
        in: queueIn;
        out: ICMPOut;
        out: transportOut[];
        out: queueOut[];
        out: errorOut;
    submodules:
        preRouting: IPv6PreRouting;
            display: "p=59,125;i=prot3;q=queue";
        forwarding: IPv6Forward;
            display: "p=146,217;i=bwgen;q=queue";
        localDeliver: IPv6LocalDeliver;
            parameters:
                numOfTransportProt = numOfTransportProt,
                fragmentTimeout = fragmentTimeout;
            gatesizes:
                transportOut[numOfTransportProt];
            display: "p=130,51;i=fork";
        multicast: IPv6Multicast;
            display: "p=226,106;i=bwgen_s";
        ICMP: ICMPv6;
            parameters:
                numOfPorts = numOfPorts;
            gatesizes:
                ipOutputIn[numOfPorts],
                outputOut[numOfPorts];
            display: "p=293,69;i=cogwheel";
        send: IPv6Send;
            parameters:
                multicastTimeToLive = multicastTimeToLive,
                numOfTransportProt = numOfTransportProt;
            gatesizes:
                transportIn[numOfTransportProt];
            display: "p=439,59;i=prot1;q=queue";
        fragmentation: IPv6Fragmentation;
            parameters:
                numOfPorts = numOfPorts;
            gatesizes:
                outputOut[numOfPorts];
            display: "p=428,195;i=bwxconn;q=queue";
        output: IPv6Output[numOfPorts];
            display: "p=302,282,r;i=prot2;q=queue";
        routingTable6: RoutingTable6;
            display: "p=490,120;i=box3";
        addrResln: AddressResolution;
            parameters:
                numOfPorts = ancestor numOfPorts;
            gatesizes:
                outputOut[numOfPorts];
            display: "p=193,280;i=queue";
        tunneling: IPv6Encapsulation;
            display: "p=274,182;i=bwxcon_s";
        mobility: IPv6Mobility;
            display: "p=60,268;i=cogwheel";
    connections nocheck:
        // PreRouting Input
        queueIn --> preRouting.inputQueueIn;

        // PreRouting Output
        preRouting.routingOut --> forwarding.preRoutingIn;
        preRouting.errorOut --> ICMP.preRoutingIn;

        // Routing output
        forwarding.localOut --> localDeliver.routingIn;
        forwarding.fragmentationOut --> fragmentation.routingIn;
        forwarding.multicastOut --> multicast.routingIn;
        forwarding.errorOut --> ICMP.routingIn;
        forwarding.pendingQueueOut --> addrResln.pendingQueueIn;

        // LocalDeliver output
        localDeliver.multicastOut --> multicast.localIn;
        localDeliver.ICMPOut --> ICMP.localIn;
        localDeliver.preRoutingOut --> preRouting.localDeliverIn;
        localDeliver.errorOut --> ICMP.localErrorIn;
        localDeliver.routingOut --> forwarding.localIn;
        localDeliver.mobilityOut --> mobility.localDeliverIn;
        for i=0..numOfTransportProt-1 do
            localDeliver.transportOut[i] --> transportOut[i];
        endfor;

        // Mobility output
        mobility.routingOut --> forwarding.mobilityIn;

        // Multicast output
        multicast.localOut --> localDeliver.multicastIn;
        multicast.fragmentationOut --> fragmentation.multicastIn;
        // multicast.tunnelOut --> tunneling.multicastIn;

        // ICMP output, ping input
        ICMP.sendOut --> send.ICMPIn;
        ICMP.pingOut --> ICMPOut;
        ICMPIn --> ICMP.pingIn;
        ICMP.errorOut --> errorOut;
    ICMP.routingOut --> forwarding.ICMPIn;
    ICMP.mldOut --> forwarding.MLDIn;

        // IP send output, input
        send.routingOut --> forwarding.sendIn;
        for i=0..numOfTransportProt-1 do
            transportIn[i] --> send.transportIn[i];
        endfor;

        // IP Fragmentation output
        fragmentation.errorOut --> ICMP.fragmentationIn;

        // IP Tunnel output
        tunneling.encapsulatedSendOut --> send.tunnelIn;
        tunneling.decapsulatedPreRoutingOut --> preRouting.tunnelExit;
        tunneling.encapsulateRoutingIn <-- forwarding.tunnelEntry;
        tunneling.decapsulateLocalDeliverIn <-- localDeliver.tunnelOut;


        addrResln.ICMPOut --> ICMP.addrReslnIn;
        addrResln.fragmentationOut --> fragmentation.addrReslnIn;

        // Output ports
        for i=0..numOfPorts-1 do
            fragmentation.outputOut[i] --> output[i].fragmentationIn;
            ICMP.outputOut[i] --> output[i].neighbourDiscoveryIn;
            addrResln.outputOut[i] --> output[i].addrReslnIn;
            output[i].queueOut --> queueOut[i];
            output[i].errorOut --> ICMP.ipOutputIn[i];
        endfor;

    display: "p=10,10;b=500,326,rect";
endmodule

