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
    "MobilityManager",
    "NetworkLayer6",
    "PhysicalLayer",
    "LinkLayer6",
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

module MobileNode
    parameters:
        beginCollectionTime : numeric,
        endCollectionTime : numeric,
        IPv6routingFile : string,
        IPForward : bool,
        numOfUDPApps: numeric;
    gates:
        in: wlin[];
        out: wlout[];
    submodules:
        errorHandling: ErrorHandling;
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
                numOfPorts = sizeof(wlin);

            gatesizes:
                physIn[sizeof(wlin)],
                physOut[sizeof(wlin)];

            display: "p=164,212;i=prot2";
        linkLayers: LinkLayer6[sizeof(wlin)];
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
                in[sizeof(wlin)],
                out[sizeof(wlin)],
                linkIn[sizeof(wlin)],
                linkOut[sizeof(wlin)];
            display: "p=172,348;b=168,8;o=#4359d7,#4159d7";
        mobilityManager: MobilityManager;
            display: "p=332,316;b=72,40";
    connections nocheck:
        networkLayer.errorOut --> errorHandling.in;

        // Link Layer <-> Network Layer
        for i=0..sizeof(wlin)-1 do
            networkLayer.physOut[i] --> linkLayers[i].outputQueueIn;
            linkLayers[i].inputQueueOut --> networkLayer.physIn[i];
        endfor;

        udpLayer.to_ip --> networkLayer.UDPIn;
        udpLayer.from_ip <-- networkLayer.UDPOut;

        for i=0..numOfUDPApps-1 do
            udpLayer.to_app[i] --> udpApps[i].physIn;
            udpLayer.from_app[i] <-- udpApps[i].physOut;
        endfor;

        ping6App.pingOut --> networkLayer.pingIn;
        ping6App.pingIn <-- networkLayer.pingOut;

        for i=0..sizeof(wlin)-1 do
          phyLayer.linkOut[i] --> linkLayers[i].physIn;
          wlin[i] --> phyLayer.in[i];
        endfor;
    display: "p=10,10;b=395,371";
endmodule

