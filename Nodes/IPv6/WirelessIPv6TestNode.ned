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
    "ErrorHandling6",
    "PingApp",
    "BRSrcModel",
    "BRSink";

//
// Definition of an IPNode with transport layers. Contains IPv6 Ping and BitRate applications
// Current code in WirelessEtherModule assumes that there is only one wlin port
// and the others are ignored if they are used here i.e. sizeof(wlin) > 1
// @warning if ever we add wired ports "in[]" then numOfPorts needs to be
// added as top level parameter and assigned value of sizeof(in) + sizeof(wlin) in
// omnetpp.ini otherwise the network layer will not be aware of some ports.
//
module WirelessIPv6TestNode
    parameters:
        brModelType: string,
        beginCollectionTime: numeric,
        endCollectionTime: numeric,
        IPForward: bool;
    gates:
        in: wlin[];
        out: wlout[];
    submodules:
        blackboard: Blackboard;
            display: "p=62,60;i=block/blackboard";
        interfaceTable: InterfaceTable;
            display: "p=60,150;i=block/table";
        brSrcModel: brModelType like BRSrcModel;
            display: "p=187,105;b=40,24";
        brSink: BRSink;
            display: "p=271,105;b=48,24";
        errorHandling: ErrorHandling6;
            display: "p=301,217;i=block/sink_vs";
        networkLayer: NetworkLayer6;
            parameters:
                IPForward = ancestor IPForward,
                numOfPorts = 1;
            gatesizes:
                physIn[1],
                physOut[1];
            display: "p=208,241;i=block/network2";
        linkLayers: LinkLayer6;
            parameters:
		NWIName="WirelessEtherModule",
                nwi_delay = 0;
            gatesizes:
                extSignalIn[1],
                extSignalOut[1];
            display: "p=212,321;i=block/ifcard";
        pingApp: PingApp;
            display: "i=block/source;p=99,217";
        phyLayer: PHYLayer;
            gatesizes:
                in[1],
                out[1],
                linkIn[1],
                linkOut[1];
            display: "p=216,377;b=168,8;o=#4359d7,#4159d7";
        mobilityManager: MobilityManager;
            display: "i=block/cogwheel;p=376,345";
    connections nocheck:
        networkLayer.errorOut --> errorHandling.in;

        brSrcModel.brOut --> networkLayer.UDPIn;
        brSink.brIn <-- networkLayer.UDPOut;

        // Link Layer <-> Network Layer
        networkLayer.physOut[0] --> linkLayers.netwIn;
        linkLayers.netwOut --> networkLayer.physIn[0];

        pingApp.pingv6Out --> networkLayer.pingIn;
        pingApp.pingv6In <-- networkLayer.pingOut;

        phyLayer.linkOut[0] --> linkLayers.physIn;
        wlin[0] --> phyLayer.in[0];
endmodule

