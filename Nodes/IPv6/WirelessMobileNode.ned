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
    "InterfaceTable",
    "MobilityManager",
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
// Definition of an IPNode with transport layers. Contains IPv6 Ping application.
// Current code in WirelessEtherModule assumes that there is only one wlin port
// and the others are ignored if they are used here i.e. sizeof(wlin) > 1
// @warning if ever we add wired ports "in[]" then numOfPorts needs to be
// added as top level parameter and assigned value of sizeof(in) + sizeof(wlin) in
// omnetpp.ini otherwise the network layer will not be aware of some ports.
//
module MobileNode
    parameters:
        numTcpApps: numeric const,
        tcpAppType: string,
        numUdpApps: numeric const,
        udpAppType: string,
        beginCollectionTime: numeric,
        endCollectionTime: numeric,
        IPForward: bool;
    gates:
        in: wlin[];
        out: wlout[];
    submodules:
        notificationBoard: NotificationBoard;
            display: "p=60,70;i=block/control";
        interfaceTable: InterfaceTable;
            display: "p=60,150;i=block/table";
        errorHandling: ErrorHandling6;
            display: "p=301,217;i=block/sink_vs";
        networkLayer: NetworkLayer6;
            parameters:
                IPForward = ancestor IPForward,
                numOfPorts = sizeof(wlin);
            gatesizes:
                physIn[sizeof(wlin)],
                physOut[sizeof(wlin)];
            display: "p=208,241;i=block/network2";
        linkLayers: LinkLayer6[sizeof(wlin)];
            parameters:
                nwi_delay = 0;
            gatesizes:
                extSignalIn[sizeof(wlin)],
                extSignalOut[sizeof(wlin)];
            display: "p=212,321;i=block/ifcard";
        tcpApp: tcpAppType[numTcpApps] like TCPApp;
            display: "p=163,67;i=block/app";
        tcp: TCPMain;
            display: "p=163,154;i=block/wheelbarrow";
        udp: UDPProcessing;
            display: "i=block/transport;p=280,152";
        udpApp: udpAppType[numUdpApps] like UDPApp;
            display: "p=278,67;i=block/app";
        pingApp: PingApp;
            display: "i=block/source;p=99,217";
        phyLayer: PHYLayer;
            gatesizes:
                in[sizeof(wlin)],
                out[sizeof(wlin)],
                linkIn[sizeof(wlin)],
                linkOut[sizeof(wlin)];
            display: "p=216,377;b=168,8;o=#4359d7,#4159d7";
        mobilityManager: MobilityManager;
            display: "i=block/cogwheel;p=376,345";
    connections nocheck:
        networkLayer.errorOut --> errorHandling.in;

        // Link Layer <-> Network Layer
        for i=0..sizeof(wlin)-1 do
            networkLayer.physOut[i] --> linkLayers[i].netwIn;
            linkLayers[i].netwOut --> networkLayer.physIn[i];
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

        for i=0..sizeof(wlin)-1 do
            phyLayer.linkOut[i] --> linkLayers[i].physIn;
            wlin[i] --> phyLayer.in[i];
        endfor;
endmodule

