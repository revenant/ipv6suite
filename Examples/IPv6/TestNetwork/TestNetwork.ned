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
    "Router6",
    "TCPNode",
    "UDPNode";


//
// Model of a test network: a server and three clients connected via a router.
//
module ClientServerNetwork
    submodules:
        worldProcessor: WorldProcessor;
            display: "p=140,168;i=bwgen_s";
        client1: UDPNode;
            parameters:
                // network parameters
                IPForward = false;
            gatesizes:
                in[2],
                out[2];
            display: "p=66,50;i=comp";
        // clients
        client2: UDPNode;
            parameters:
                // network parameters
                IPForward = false;
            gatesizes:
                in[1],
                out[1];
            display: "p=201,45;i=comp";
        client3: TCPNode;
            parameters:
                // TCP parameters
                local_addr = (10 << 24) + 3,
                server_addr = (10 << 24) + 4,
                // network parameters
                IPForward = false;
            gatesizes:
                in[1],
                out[1];
            display: "p=208,164;i=comp";
        server4: UDPNode;
            parameters:
                // network parameters
                IPForward = false;
            gatesizes:
                in[2],
                out[2];
            display: "p=63,161;i=pc";
        router: Router6;
            gatesizes:
                in[4],
                out[4];
            display: "p=138,102;i=ipc";
    connections nocheck:
        client1.out[0] --> router.in[0];
        client1.in[0] <-- router.out[0];

        client2.out[0] --> router.in[1];
        client2.in[0] <-- router.out[1];

        client3.out[0] --> router.in[2];
        client3.in[0] <-- router.out[2];

        //Correct for ping test
        server4.out[1] --> router.in[3];
        server4.in[1] <-- router.out[3];

        client1.out[1] --> server4.in[0];
        client1.in[1] <-- server4.out[0];

                                          //This caused it not to reply to NS from addr res from ping that arrived
                                          //at server4.in[0] not in[1]

                                          //server4.out[0] --> router.in[3];
                                          //server4.in[0] <-- router.out[3];

                                          //client1.out[1] --> server4.in[1];
                                          //client1.in[1] <-- server4.out[1];

    display: "p=10,10;b=236,187";
endmodule


//
// Consists only one router; used to test reading routing files.
//
module TestNetwork
    submodules:
        worldProcessor: WorldProcessor;
            display: "p=140,168;i=bwgen_s";
        routingTableTest: Router6;
           gatesizes:
                in[1],
                out[1];
    connections nocheck:

    display: "p=10,10;b=236,187";
endmodule


network tcpclservnet : ClientServerNetwork
endnetwork


network testnet : TestNetwork
endnetwork
