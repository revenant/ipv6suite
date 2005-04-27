// -*- Ned -*-
//
// Copyright (C) 2002, 2004 Johnny Lai
//
// This file is part of IPv6Suite
//
// IPv6Suite is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// IPv6Suite is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.


import
    "Blackboard",
    "InterfaceTable",
    "NetworkLayer6",
    "PhysicalLayer",
    "LinkLayer6",
    "ErrorHandling6",
    "TCPMain",
    "TCPApp",
    "UDPProcessing",
    "UDPApp",
    "PingApp";

//
// UDP node that can contain many different UDP services
//
// @author Johnny Lai
//
module UDPNode
    parameters:
        numTcpApps: numeric const,
        tcpAppType: string,
        numUdpApps: numeric const,
        udpAppType: string,
        IPForward: bool; //Added at higher level so can specify in Network Hierarchy ned file
                         //whether this node acts as a router.
    gates:
        in: in[];
        out: out[];
    submodules:
        blackboard: Blackboard;
            display: "p=62,60;i=block/blackboard";
        interfaceTable: InterfaceTable;
            display: "p=60,150;i=block/table";
        errorHandling: ErrorHandling6;
            display: "p=309,212;i=block/sink_vs";
        networkLayer: NetworkLayer6;
            parameters:
                IPForward = ancestor IPForward,
                numOfPorts = sizeof(in);
            gatesizes:
                physIn[sizeof(in)],
                physOut[sizeof(in)];
            display: "p=213,234;i=block/network2";
        linkLayers: LinkLayer6[sizeof(in)];
            parameters:
                nwi_delay = 0;
            display: "p=217,314;i=block/ifcard";
        tcpApp: tcpAppType[numTcpApps] like TCPApp;
            display: "p=163,67;i=block/app";
        tcp: TCPMain;
            display: "p=163,154;i=block/wheelbarrow";
        udp: UDPProcessing;
            display: "i=block/transport;p=279,151";
        udpApp: udpAppType[numUdpApps] like UDPApp;
            display: "p=281,67;i=block/app";
        pingApp: PingApp;
            display: "i=block/source;p=106,218";
        phyLayer: PHYLayer;
            gatesizes:
                in[sizeof(in)],
                out[sizeof(in)],
                linkIn[sizeof(in)],
                linkOut[sizeof(in)];
            display: "p=221,370;b=160,8;o=#4359d7,#4159d7";
    connections nocheck:
        networkLayer.errorOut --> errorHandling.in;

        // Link Layer <-> Network Layer
        for i=0..sizeof(in)-1 do
            networkLayer.physOut[i] --> linkLayers[i].netwIn;
            linkLayers[i].netwOut --> networkLayer.physIn[i];
        endfor;

        // connections to network outside
        for i=0..sizeof(in)-1 do
            in[i] --> phyLayer.in[i];
            out[i] <-- phyLayer.out[i];
        endfor;

        // Link Layer <-> Physical Layer
        for i=0..sizeof(in)-1 do
            phyLayer.linkOut[i] --> linkLayers[i].physIn;
            phyLayer.linkIn[i] <-- linkLayers[i].physOut;
        endfor;

        tcp.to_ipv6 --> networkLayer.TCPIn;
        tcp.from_ipv6 <-- networkLayer.TCPOut;

        for i=0..numTcpApps-1 do
            tcpApp[i].tcpOut --> tcp.from_appl++;
            tcpApp[i].tcpIn <-- tcp.to_appl++;
        endfor;

        udp.to_ipv6 --> networkLayer.UDPIn;
        udp.from_ipv6 <-- networkLayer.UDPOut;

        for i=0..numUdpApps-1 do
            udp.to_app++ --> udpApp[i].from_udp;
            udp.from_app++ <-- udpApp[i].to_udp;
        endfor;

        pingApp.pingv6Out --> networkLayer.pingIn;
        pingApp.pingv6In <-- networkLayer.pingOut;

endmodule

