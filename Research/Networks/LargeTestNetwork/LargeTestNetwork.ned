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
    "TCPNode",
    "TCPSvrNode",
    "Ethernet";


//
// Test network, with several hosts and Ethernet LANs.
//
//# FIXME Ethernet is not defined!
//
module LargeTestNetwork
    submodules:
        worldProcessor: WorldProcessor;
            display: "p=60,288;i=bwgen_s;b=17,17";
        ctie: Ethernet;
            parameters:
                numOfHosts = 4,
                numOfRouters = 1,
                numOfAdditionalNodes = 1,
                numOfEachRtrOutports = 1;
            gatesizes:
                routerIn[1],
                routerOut[1],
                additionalIn[1],
                additionalOut[1];
            display: "p=260,172;i=cloud;b=32,32";
        ecse: Ethernet;
            parameters:
                numOfHosts = 5,
                numOfRouters = 1,
                numOfAdditionalNodes = 0,
                numOfEachRtrOutports = 2;
            gatesizes:
                routerIn[2],
                routerOut[2],
                additionalIn[0],
                additionalOut[0];
            display: "p=204,220;i=cloud;b=32,32;o=#76dcdc";
        bigpond: Ethernet;
            parameters:
                numOfHosts = 13,
                numOfRouters = 2,
                numOfAdditionalNodes = 1,
                numOfEachRtrOutports = 1;
            gatesizes:
                routerIn[2],
                routerOut[2],
                additionalIn[1],
                additionalOut[1];
            display: "p=116,240;i=cloud;b=32,32";
        optus: Ethernet;
            parameters:
                numOfHosts = 10,
                numOfRouters = 1,
                numOfAdditionalNodes = 1,
                numOfEachRtrOutports = 1;
            gatesizes:
                routerIn[1],
                routerOut[1],
                additionalIn[1],
                additionalOut[1];
            display: "p=160,132;i=cloud;b=32,32";
        Johnny_PC: TCPNode;
            parameters:
                local_addr = (10 << 24) + 2,
                server_addr = (10 << 24) + 4,

                IPForward = false;
            gatesizes:
                in[1],
                out[1];
            display: "p=160,48;i=pc;b=38,32";
        Eric_PC: TCPNode;
            parameters:
                local_addr = (10 << 24) + 1,
                server_addr = (10 << 24) + 254,

                IPForward = false;
            gatesizes:
                in[1],
                out[1];
            display: "p=48,144;i=pc;b=38,32";
        Ahmet_PC: TCPSvrNode;
            parameters:
                local_addr = (10 << 24) + 2,

                IPForward = false;
            gatesizes:
                in[1],
                out[1];
            display: "p=308,268;i=pc;b=38,32";
    connections nocheck:
        Eric_PC.out[0] --> bigpond.additionalIn[0];
        Eric_PC.in[0] <-- bigpond.additionalOut[0];

        Johnny_PC.out[0] --> optus.additionalIn[0];
        Johnny_PC.in[0] <-- optus.additionalOut[0];

        Ahmet_PC.out[0] --> ctie.additionalIn[0];
        Ahmet_PC.in[0] <-- ctie.additionalOut[0];

        // optus connection
        optus.routerOut[0] --> bigpond.routerIn[0];
        optus.routerIn[0] <-- bigpond.routerOut[0];

        // bigpond connection
        bigpond.routerOut[1] --> ecse.routerIn[0];
        bigpond.routerIn[1] <-- ecse.routerOut[0];

        // ecse/ctie connection
        ecse.routerOut[1] --> ctie.routerIn[0];
        ecse.routerIn[1] <-- ctie.routerOut[0];

endmodule

network largeTestNetwork : LargeTestNetwork
endnetwork

