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
    "UDPNode";


//
// Network model with a server and three clients, connecting to two routers
// via PPP. 
//
module PingNetwork
    submodules:
        worldProcessor: WorldProcessor;
            display: "p=100,184;i=bwgen_s;b=17,17";
        client1: UDPNode;
            parameters:
                // network parameters
                IPForward = false;
            gatesizes:
                in[1],
                out[1];
            display: "b=36,32;p=42,162;i=comp";
        client2: UDPNode;
            parameters:
                // network parameters
                IPForward = false;
            gatesizes:
                in[1],
                out[1];
            display: "b=36,32;p=41,53;i=comp";
        client3: UDPNode;
            parameters:
                // network parameters
                IPForward = false;
            gatesizes:
                in[1],
                out[1];
            display: "b=36,32;p=272,52;i=comp";
        server4: UDPNode;
            parameters:
                // network parameters
                IPForward = false;
            gatesizes:
                in[1],
                out[1];
            display: "b=38,32;p=271,161;i=pc";
        routerInt: Router6;
            gatesizes:
                in[3],
                out[3];
            display: "b=32,15;p=130,94;i=ipc";
        routerExt: Router6;
            gatesizes:
                in[3],
                out[3];
            display: "b=32,15;p=210,94;i=ipc";
    connections nocheck:
        client1.out[0] --> routerInt.in[0];
        client1.in[0] <-- routerInt.out[0];

        client2.out[0] --> routerInt.in[1];
        client2.in[0] <-- routerInt.out[1];

        routerInt.out[2] --> routerExt.in[2];
        routerInt.in[2] <-- routerExt.out[2];

        client3.out[0] --> routerExt.in[0];
        client3.in[0] <-- routerExt.out[0];

        server4.out[0] --> routerExt.in[1];
        server4.in[0] <-- routerExt.out[1];

    display: "p=10,10;b=317,207";
endmodule

network pingNet : PingNetwork
endnetwork

