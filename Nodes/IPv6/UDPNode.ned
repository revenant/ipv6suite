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


//
// @file   UDPNode.ned
// @author Johnny Lai
// @date   06 Aug 2003
//
// @brief  Abstract UDP node that can contain many different UDP services
//
// Replaced file and module of the same name


import
    "Blackboard",
    "NetworkLayer6",
    "PhysicalLayer",
    "LinkLayer6",
    "ControlApp",
    "UDPProcessing6",
    "UDPApplication",
    "Ping6App";

///Replaces the old UDPNode (this file and module previously called AbstractUDPNode)
module UDPNode
    parameters:
        IPv6routingFile : string, //Only if it is a router do we read routingFile
        IPForward : bool, //Added at higher level so can specify in Network Hierarchy ned file
                          //whether this node acts as a router.
        numOfUDPApps: numeric;
    gates:
        in: in[];
        out: out[];
    submodules:
        blackboard: Blackboard;
            display: "p=62,60;i=block/blackboard";
        errorHandling: ErrorHandling6;
            display: "p=291,222;i=sink";
        networkLayer: NetworkLayer6;
            parameters:
                // waiting times
                icmp_delay = 0,
                fragmentation_delay = 0.2 us,
                multicast_delay = 0.5 us,
                ipoutput_delay = 0,
                ipsend_delay = 0,
                iptunneling_delay = 0,
                localdeliver_delay = 0,
                prerouting_delay = 0.2 us,
                routing_delay = 1 us,
                inputqueue_delay = 0,

                // other network parameters
                //                routingFile = routingFile,
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
        udpLayer: UDPProcessing6;
            gatesizes:
                from_app[numOfUDPApps],
                to_app[numOfUDPApps];
            display: "i=fork;p=160,152";
    udpApps: UDPAppLayer[numOfUDPApps];
            display: "p=296,44;i=cogwheel";
        ping6App: Ping6App;
            display: "p=79,168;b=40,24";
        phyLayer: PHYLayer;
            gatesizes:
                in[sizeof(in)],
                out[sizeof(in)],
                linkIn[sizeof(in)],
                linkOut[sizeof(in)];
            display: "p=172,348;b=168,8;o=#4359d7,#4159d7";
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

        udpLayer.to_ip --> networkLayer.UDPIn;
        udpLayer.from_ip <-- networkLayer.UDPOut;

        for i=0..numOfUDPApps-1 do
            udpLayer.to_app[i] --> udpApps[i].physIn;
            udpLayer.from_app[i] <-- udpApps[i].physOut;
        endfor;

        ping6App.pingOut --> networkLayer.pingIn;
        ping6App.pingIn <-- networkLayer.pingOut;

    display: "p=10,10;b=407,383";
endmodule
