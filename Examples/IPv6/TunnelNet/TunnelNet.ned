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
// Example network consisting of four routers, a client and a server.
// Used to test IPv6 tunnelling functionality.
//
module TunnelNet
    submodules:
        worldProcessor: WorldProcessor;
            display: "p=60,48;i=bwgen_s;b=17,17";
        client: UDPNode;
            parameters:
                IPForward = false;
            gatesizes:
                in[1],
                out[1];
            display: "b=36,32;p=42,194;i=comp";
        // clients
        server: UDPNode;
            parameters:
                IPForward = false;
            gatesizes:
                in[1],
                out[1];
            display: "b=38,32;p=311,193;i=pc";
        routerA: Router6;
            gatesizes:
                in[3],
                out[3];
            display: "b=32,15;p=82,110;i=ipc";
        routerB: Router6;
            gatesizes:
                in[3],
                out[3];
            display: "b=32,15;p=178,38;i=ipc";
        routerC: Router6;
            gatesizes:
                in[3],
                out[3];
            display: "b=32,15;p=274,110;i=ipc";
        routerD: Router6;
            gatesizes:
                in[3],
                out[3];
            display: "b=32,15;p=178,182;i=ipc";
    connections nocheck:
        client.out[0] --> routerA.in[0];
        client.in[0] <-- routerA.out[0];

        server.out[0] --> routerC.in[0];
        server.in[0] <-- routerC.out[0];

        routerA.out[1] --> routerB.in[0];
        routerA.in[1] <-- routerB.out[0];

        routerA.out[2] --> routerD.in[0];
        routerA.in[2] <-- routerD.out[0];

        routerB.out[1] --> routerD.in[1];
        routerB.in[1] <-- routerD.out[1];

        routerB.out[2] --> routerC.in[1];
        routerB.in[2] <-- routerC.out[1];

        routerD.out[2] --> routerC.in[2];
        routerD.in[2] <-- routerC.out[2];

    display: "p=10,10;b=341,231";
endmodule

network tunnelNet : TunnelNet
endnetwork

