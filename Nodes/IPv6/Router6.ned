// $Header: /home/cvs/IPv6Suite/IPv6SuiteWithINET/Nodes/IPv6/Router6.ned,v 1.1 2005/02/09 06:15:58 andras Exp $
//
// Copyright (C) 2002, 2004 Eric Wu
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
// Definition of an IPNode with transport layers

import
    "NetworkLayer6",
    "LinkLayer6",

    "PhysicalLayer",
    "ControlApp";


//
// IPv6 router with the number of ports as parameter.
//
// Also takes routing table as input.
//
module Router6
    parameters:
        IPv6routingFile : string;    
    gates:
        in: in[];
        out: out[];

    submodules:
        errorHandling: ErrorHandling;
            parameters:
                nodename = "none";
            display: "b=32,30;p=163,86;i=sink";
        networkLayer: NetworkLayer6;
            parameters:
                // waiting times
		//icmp_delay = 0,
		//fragmentation_delay = 0.2 us,
		//multicast_delay = 0.5 us,
		//ipoutput_delay = 50ms,
		//ipsend_delay = 0,
		//iptunneling_delay = 0,
		//localdeliver_delay = 0,
		//prerouting_delay = 0.2 us,
		//routing_delay = 1 us,
		//inputqueue_delay = 0,

                // hook names
                preRoutingHookName = "NF_IP_PRE_ROUTING",
                routingHookName = "NF_IP_FORWARD",
                localDeliverHookName = "NF_IP_LOCAL_IN",
                ipSendHookName = "NF_IP_LOCAL_OUT",
                ipOutputHookName = "NF_IP_POST_ROUTING",

                // other network parameters
                //                routingFile = routingFile,
                IPForward = true,
                numOfPorts = sizeof(in);

            gatesizes:
                physIn[sizeof(in)],
                physOut[sizeof(in)];

            display: "p=116,148;i=fork;b=32,30";
        linkLayers: LinkLayer6[sizeof(in)];
            parameters:
                nwi_delay = 0,
                enqueueHookName = "L2_EnqueueHook",
                dequeueHookName = "L2_DequeueHook",
                enqueue_delay = 0,
                dequeue_delay = 0;
            display: "p=120,212;i=prot3;b=32,30";
        phyLayer: PHYLayer;
            gatesizes:
                in[sizeof(in)],
                out[sizeof(in)],
                linkIn[sizeof(in)],
                linkOut[sizeof(in)];
            display: "p=148,276;b=168,8;o=#4359d7,#4159d7";
    connections nocheck:
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
    display: "p=10,10;b=266,310";
endmodule

