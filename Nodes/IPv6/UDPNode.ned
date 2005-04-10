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
    "ControlApp",
    "UDPProcessing6",
    "UDPApplication",
    "Ping6App";

//
// UDP node that can contain many different UDP services
//
// @author Johnny Lai
//
module UDPNode
    parameters:
        udpAppType: string,
        IPForward : bool, //Added at higher level so can specify in Network Hierarchy ned file
                          //whether this node acts as a router.
        numUdpApps: numeric;
    gates:
        in: in[];
        out: out[];
    submodules:
        blackboard: Blackboard;
            display: "p=62,60;i=block/blackboard";
        interfaceTable: InterfaceTable;
            display: "p=60,150;i=block/table";
        errorHandling: ErrorHandling6;
            display: "p=260,190;i=block/sink_vs";
        networkLayer: NetworkLayer6;
            parameters:
                IPForward = ancestor IPForward,
                numOfPorts = sizeof(in);
            gatesizes:
                physIn[sizeof(in)],
                physOut[sizeof(in)];
            display: "p=164,212;i=prot2";
        linkLayers: LinkLayer6[sizeof(in)];
            parameters:
                nwi_delay = 0;
            display: "p=168,292;i=prot3";
        udp: UDPProcessing6;
            gatesizes:
                from_app[numUdpApps],
                to_app[numUdpApps];
            display: "i=fork;p=160,152";
        udpApps: udpAppType[numUdpApps] like UDPApp;
            display: "p=296,44;i=cogwheel";
        ping6App: Ping6App;
            display: "p=79,210;b=40,24";
        phyLayer: PHYLayer;
            gatesizes:
                in[sizeof(in)],
                out[sizeof(in)],
                linkIn[sizeof(in)],
                linkOut[sizeof(in)];
            display: "p=172,348;b=160,8;o=#4359d7,#4159d7";
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

        udp.to_ip --> networkLayer.UDPIn;
        udp.from_ip <-- networkLayer.UDPOut;

        for i=0..numUdpApps-1 do
            udp.to_app[i] --> udpApps[i].from_udp;
            udp.from_app[i] <-- udpApps[i].to_udp;
        endfor;

        ping6App.pingOut --> networkLayer.pingIn;
        ping6App.pingIn <-- networkLayer.pingOut;

    display: "p=10,10;b=407,383";
endmodule
