//
// Copyright (C) 2001 Monash University, Australia
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
    "WorldProcessor",
    "UDPNode",
    "EtherHub",
    "Router6";


//
// Part of the Monash University network, with the IPv6Suite people: some of the
// department's servers; Eric and Johnny connecting from home via their ISPs;
// Ahmet also connecting from home via the staff dial-in service.
//
// It has a mixture of PPP and Ethernet.
// 
module MelbourneNetwork
    submodules:
        worldProcessor: WorldProcessor;
            display: "p=60,40;i=bwgen_s;b=17,17";
        ecse_1: UDPNode;
            parameters:
                IPForward = false;
            gatesizes:
                in[1],
                out[1];
            display: "i=comp_s;p=56,104;b=18,16";
        ecse_2: UDPNode;
            parameters:
                IPForward = false;
            gatesizes:
                in[1],
                out[1];
            display: "p=124,72;i=comp_s;b=55,45";
        ecse_3: UDPNode;
            parameters:
                IPForward = false;
            gatesizes:
                in[1],
                out[1];
            display: "p=196,96;i=comp_s;b=18,16";
        ecse_4: UDPNode;
            parameters:
                IPForward = false;
            display: "p=52,172;i=comp_s;b=53,45";
        ecse_router: Router6;
            gatesizes:
                in[3],
                out[3];
            display: "p=220,212;i=router;b=32,32";
        ecse_hub: Hub;
            gatesizes:
                in[5],
                out[5];
            display: "p=128,208;i=ipc;b=32,15";
        Ahmet_PC: UDPNode;
            parameters:
                IPForward = false;
            display: "p=528,44;i=comp;b=36,32";
        phoenix: UDPNode;
            parameters:
                IPForward = false;
            gatesizes:
                in[1],
                out[1];
            display: "p=380,48;i=comp;b=38,32";
        hydra: UDPNode;
            parameters:
                IPForward = false;
            gatesizes:
                in[1],
                out[1];
            display: "p=276,52;i=comp;b=38,32";
        staff_dialin: Router6;
            gatesizes:
                in[2],
                out[2];
            display: "p=488,224;i=cloud;b=32,32";
        ctie_router: Router6;
            gatesizes:
                in[4],
                out[4];
            display: "i=router;p=337,195;b=32,32";
        ctie_hub: Hub;
            gatesizes:
                in[3],
                out[3];
            display: "p=420,164;i=ipc;b=32,15";
        optus_1: UDPNode;
            parameters:
                IPForward = false;
            gatesizes:
                in[1],
                out[1];
            display: "p=44,284;i=comp_s;b=18,16";
        optus_2: UDPNode;
            parameters:
                IPForward = false;
            gatesizes:
                in[1],
                out[1];
            display: "p=44,344;i=comp_s;b=18,16";
        optus_3: UDPNode;
            parameters:
                IPForward = false;
            gatesizes:
                in[1],
                out[1];
            display: "p=212,356;i=comp_s;b=18,16";
        Johnny_PC: UDPNode;
            parameters:
                IPForward = false;
            gatesizes:
                in[1],
                out[1];
            display: "p=72,432;i=comp;b=55,45";
        optus_router: Router6;
            gatesizes:
                in[2],
                out[2];
            display: "p=216,296;i=router;b=32,32";
        optus_hub: Hub;
            gatesizes:
                in[5],
                out[5];
            display: "p=132,344;i=ipc;b=32,15";
        telstra_1: UDPNode;
            parameters:
                IPForward = false;
            gatesizes:
                in[1],
                out[1];
            display: "p=376,356;i=comp_s;b=18,16";
        telstra_2: UDPNode;
            parameters:
                IPv6routingFile = "melbourneNetwork.xml",
                IPForward = false;
            gatesizes:
                in[1],
                out[1];
            display: "p=508,296;i=comp_s;b=18,16";
        Eric_PC: UDPNode;
            parameters:
                IPv6routingFile = "melbourneNetwork.xml",
                IPForward = false;
            display: "p=528,432;i=comp;b=53,45";
        telstra_router: Router6;
            gatesizes:
                in[4],
                out[4];
            display: "p=368,296;i=router;b=32,32";
        telstra_hub: Hub;
            gatesizes:
                in[4],
                out[4];
            display: "p=460,336;i=ipc;b=32,15";
    connections nocheck:
        ecse_router.out[0] --> ecse_hub.in[0];
        ecse_router.in[0] <-- ecse_hub.out[0];

        ecse_1.out[0] --> ecse_hub.in[1];
        ecse_1.in[0] <-- ecse_hub.out[1];

        ecse_2.out[0] --> ecse_hub.in[2];
        ecse_2.in[0] <-- ecse_hub.out[2];

        ecse_3.out[0] --> ecse_hub.in[3];
        ecse_3.in[0] <-- ecse_hub.out[3];

        ecse_4.out[0] --> ecse_hub.in[4];
        ecse_4.in[0] <-- ecse_hub.out[4];

        ctie_router.out[0] --> ctie_hub.in[0];
        ctie_router.in[0] <-- ctie_hub.out[0];

        staff_dialin.out[1] --> ctie_router.in[3];
        staff_dialin.in[1] <-- ctie_router.out[3];

        Ahmet_PC.out[0] --> staff_dialin.in[0];
        Ahmet_PC.in[0] <-- staff_dialin.out[0];

        hydra.out[0] --> ctie_hub.in[1];
        hydra.in[0] <-- ctie_hub.out[1];

        phoenix.out[0] --> ctie_hub.in[2];
        phoenix.in[0] <-- ctie_hub.out[2];

        optus_router.out[0] --> optus_hub.in[0];
        optus_router.in[0] <-- optus_hub.out[0];

        optus_1.out[0] --> optus_hub.in[1];
        optus_1.in[0] <-- optus_hub.out[1];

        optus_2.out[0] --> optus_hub.in[2];
        optus_2.in[0] <-- optus_hub.out[2];

        optus_3.out[0] --> optus_hub.in[3];
        optus_3.in[0] <-- optus_hub.out[3];

        Johnny_PC.out[0] --> optus_hub.in[4];
        Johnny_PC.in[0] <-- optus_hub.out[4];

        telstra_router.out[0] --> telstra_hub.in[0];
        telstra_router.in[0] <-- telstra_hub.out[0];

        telstra_1.out[0] --> telstra_hub.in[1];
        telstra_1.in[0] <-- telstra_hub.out[1];

        telstra_2.out[0] --> telstra_hub.in[2];
        telstra_2.in[0] <-- telstra_hub.out[2];

        Eric_PC.out[0] --> telstra_hub.in[3];
        Eric_PC.in[0] <-- telstra_hub.out[3];

        ecse_router.out[1] --> ctie_router.in[1];
        ecse_router.in[1] <-- ctie_router.out[1];

        telstra_router.out[1] --> ecse_router.in[2];
        telstra_router.in[1] <-- ecse_router.out[2];

        telstra_router.out[2] --> ctie_router.in[2];
        telstra_router.in[2] <-- ctie_router.out[2];

        telstra_router.out[3] --> optus_router.in[1];
        telstra_router.in[3] <-- optus_router.out[1];
endmodule

network melbourneNetwork : MelbourneNetwork
endnetwork

