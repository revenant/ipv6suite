// Copyright (C) 2001, 2004 CTIE, Monash University
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
//   file: WirelessAccessPoint
//   Purpose: implementation of wireless ethernet access point
//   author: Eric Wu
//
//  Access Point is a module that uses wlin for the wireless port (it is not
//  connected) and the numOfPorts parameter refers to number of wired ports.
//  @todo Access Point and Mobile Node should have wlin[] arrays to support
//  multiple wireless interfaces.

import
    "WirelessEtherBridge",

    "PhysicalLayer",
    "MobilityManager",
    "LinkLayer6";








module AccessPoint
    parameters:
        chann : numeric, // channel
        beaconPeriod : numeric,
        authWaitEntryTimeout : numeric,
        authEntryTimeout : numeric,
        assEntryTimeout : numeric,
                consecFailedTransLimit : numeric,
        beginCollectionTime : numeric,
        endCollectionTime : numeric;
    gates:
        out: out[], wlout;
        in: in[], wlin;
    submodules:
        ds: LinkLayer6[sizeof(in)];
            parameters:
                nwi_delay = 0,
                enqueueHookName = "L2_EnqueueHook",
                dequeueHookName = "L2_DequeueHook",
                enqueue_delay = 0,
                dequeue_delay = 0;
            display: "p=136,124;i=card2";
        wirelessAccessPoint: LinkLayer6;
            parameters:
                NWIName = "WirelessAccessPoint",
                nwi_delay = 0,
                enqueueHookName = "L2_EnqueueHook",
                dequeueHookName = "L2_DequeueHook",
                enqueue_delay = 0,
                dequeue_delay = 0;
            display: "p=144,268;i=card2";
        bridge: WirelessEtherBridge;
            gatesizes:
                dsIn[sizeof(in)],
                dsOut[sizeof(in)];
            display: "p=248,188;i=bwxconn";
        phyLayerAP: PHYLayer;
            gatesizes:
                in[sizeof(wlin)],
                out[sizeof(wlin)],
                linkIn[sizeof(wlin)],
                linkOut[sizeof(wlin)];
            display: "p=144,336;b=128,16;o=#d73948,#d73849";
        phyLayer: PHYLayer;
            gatesizes:
                in[sizeof(in)],
                out[sizeof(in)],
                linkIn[sizeof(in)],
                linkOut[sizeof(in)];
            display: "p=136,72;b=96,16;o=#5853d7,#5951d7";
        mobilityManager: MobilityManager;
            display: "p=332,284;b=104,56";
    connections nocheck:

        // dsPHY <--> network
        for i=0..sizeof(in)-1 do
            in[i] --> phyLayer.in[0];
            out[i] <-- phyLayer.out[0];
        endfor;

        // ds <--> dsPHY
        for i=0..sizeof(in)-1 do
            phyLayer.linkOut[i] --> ds[i].physIn;
            phyLayer.linkIn[i] <-- ds[i].physOut;
        endfor;

        // ds <--> bridge
        for i=0..sizeof(in)-1 do
            bridge.dsOut[i] --> ds[i].outputQueueIn;
            ds[i].inputQueueOut --> bridge.dsIn[i];
        endfor;

        // bridge <--> ap
        bridge.apOut --> wirelessAccessPoint.outputQueueIn;
        bridge.apIn <-- wirelessAccessPoint.inputQueueOut;

        wirelessAccessPoint.physIn <-- phyLayerAP.linkOut[0];
        wlin --> phyLayerAP.in[0];
    display: "p=10,10;b=408,374";
endmodule

