//
// Copyright (C) 2001, 2004 Monash University, Australia
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


import
    "Blackboard",
    "InterfaceTable",
    "MobilityManager",
    "NetworkLayer6",
    "PhysicalLayer",
    "LinkLayer6",
    "DualInterfaceLayer",
    "ControlApp",
    "UDPProcessing6",
    "UDPApplication",
    "Ping6App";






//
// Definition of an IPNode with transport layers. Contains IPv6 Ping application.
// Current code in WirelessEtherModule assumes that there is only one wlin port
// and the others are ignored if they are used here i.e. sizeof(wlin) > 1
// @warning if ever we add wired ports "in[]" then numOfPorts needs to be
// added as top level parameter and assigned value of sizeof(in) + sizeof(wlin) in
// omnetpp.ini otherwise the network layer will not be aware of some ports.
//
module DualIntMobileNode
    parameters:
        udpAppType: string,
        beginCollectionTime : numeric,
        endCollectionTime : numeric,
        IPForward : bool, //Added at higher level so can specify in Network Hierarchy ned file
                          //whether this node acts as a router.
        numUdpApps: numeric;
    gates:
        in: wlin[];
    submodules:
        blackboard: Blackboard;
            display: "p=62,60;i=block/blackboard";
        interfaceTable: InterfaceTable;
            display: "p=60,150;i=block/table";
        errorHandling: ErrorHandling6;
            display: "p=291,222;i=sink";
        networkLayer: NetworkLayer6;
            parameters:
                IPForward = ancestor IPForward,
                numOfPorts = 1;
            gatesizes:
                physIn[sizeof(wlin)],
                physOut[sizeof(wlin)];
            display: "p=164,236;i=prot2";
        dualInterfaceLayer: DualInterfaceLayer;
            gatesizes:
                fromLL[sizeof(wlin)],
                toLL[sizeof(wlin)],
                extSignalIn[sizeof(wlin)],
                extSignalOut[sizeof(wlin)];
            display: "p=160,298;i=prot3";
        linkLayers: LinkLayer6[sizeof(wlin)];
            parameters:
                nwi_delay = 0;
            gatesizes:
                extSignalIn[1],
                extSignalOut[1];
            display: "p=160,364;i=prot3";
        udp: UDPProcessing6;
            gatesizes:
        from_app[numUdpApps],
        to_app[numUdpApps];
            display: "i=fork;p=160,152";
        udpApps: udpAppType[numUdpApps] like UDPApp;
            display: "p=296,44;i=cogwheel";
        ping6App: Ping6App;
            display: "p=79,168;b=40,24";
        phyLayer: PHYLayer[sizeof(wlin)];
            gatesizes:
                in[1],
                out[1],
                linkIn[1],
                linkOut[1];
            display: "p=180,420;b=168,8;o=#4359d7,#4159d7";
        mobilityManager: MobilityManager;
            display: "p=332,316;b=72,40";
    connections nocheck:
        networkLayer.errorOut --> errorHandling.in;

                                                    // Outer Shell <-> Phy Layer

        for i=0..sizeof(wlin)-1 do
            wlin[i] --> phyLayer[i].in[0];
        endfor;

// Phy Layer <-> Link Layer

        for i=0..sizeof(wlin)-1 do
            phyLayer[i].linkOut[0] --> linkLayers[i].physIn;
        endfor;

// Link Layer <-> Dual Interface Layer

        for i=0..sizeof(wlin)-1 do
            dualInterfaceLayer.toLL[i] --> linkLayers[i].netwIn;
            dualInterfaceLayer.fromLL[i] <-- linkLayers[i].netwOut;
            dualInterfaceLayer.extSignalOut[i] --> linkLayers[i].extSignalIn[0];
            dualInterfaceLayer.extSignalIn[i] <-- linkLayers[i].extSignalOut[0];
        endfor;

// Dual Interface Layer <-> Network Layer

        networkLayer.physOut[0] --> dualInterfaceLayer.fromNL;
        networkLayer.physIn[0] <-- dualInterfaceLayer.toNL;

        udp.to_ip --> networkLayer.UDPIn;
        udp.from_ip <-- networkLayer.UDPOut;

        for i=0..numUdpApps-1 do
            udp.to_app[i] --> udpApps[i].from_udp;
            udp.from_app[i] <-- udpApps[i].to_udp;
        endfor;

        ping6App.pingOut --> networkLayer.pingIn;
        ping6App.pingIn <-- networkLayer.pingOut;

endmodule

