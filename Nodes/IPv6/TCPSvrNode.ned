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


// Definition of an IPNode with transport layers

import
    "NetworkLayer6",
    "LinkLayer6",
    "PhysicalLayer",

    "tcp4ip",
    "ControlApp";






module TCPSvrNode
    parameters:
        // TCP information
        local_addr : numeric,
        // Network information

        //        routingFile : string,
        IPv6routingFile : string,
        IPForward : bool;
    gates:
        in: in[];
        out: out[];
    submodules:
        // transport and application layers
        tcpApp: TCPUpperLayers;
            parameters:
                tcpApplType = "TCPServer",
                local_addr = local_addr,
                server_addr = 0;
            display: "p=84,69;b=40,24,rect";
        errorHandling: ErrorHandling;
            display: "p=163,86;i=sink";
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

                // hook names
                preRoutingHookName = "NF_IP_PRE_ROUTING",
                routingHookName = "NF_IP_FORWARD",
                localDeliverHookName = "NF_IP_LOCAL_IN",
                ipSendHookName = "NF_IP_LOCAL_OUT",
                ipOutputHookName = "NF_IP_POST_ROUTING",

                // other network parameters
                //                routingFile = routingFile,
                IPForward = ancestor IPForward,
                numOfPorts = sizeof(in);

            gatesizes:
                physIn[sizeof(in)],
                physOut[sizeof(in)];

            display: "p=84,148;i=fork";
        linkLayers: LinkLayer6[sizeof(in)];
            parameters:
                //                NWIName = "IPv6PPPInterface",
                nwi_delay = 0,
                enqueueHookName = "L2_EnqueueHook",
                dequeueHookName = "L2_DequeueHook",
                enqueue_delay = 0,
                dequeue_delay = 0;
            display: "p=88,212;i=prot3";
        phyLayer: PHYLayer;
            gatesizes:
                in[sizeof(in)],
                out[sizeof(in)],
                linkIn[sizeof(in)],
                linkOut[sizeof(in)];
            display: "p=132,308;b=168,8;o=#4359d7,#4159d7";
    connections nocheck:
        // transport connections
        networkLayer.TCPOut --> tcpApp.in;
        networkLayer.TCPIn <-- tcpApp.out;
        networkLayer.errorOut --> errorHandling.in;

        // Link Layer <-> Network Layer
        for i=0..sizeof(in)-1 do
            networkLayer.physOut[i] --> linkLayers[i].outputQueueIn;
            linkLayers[i].inputQueueOut --> networkLayer.physIn[i];
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

endmodule

