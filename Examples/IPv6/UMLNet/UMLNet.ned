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
    //"Router6",
    //"TCPNode",
    //"UDPNode";
    "UMLClient",
    "UMLSwitch",
    "WorldProcessor";


//
// Network for testing UML (User Mode Linux) integration. Part of the
// model is a real application running in User Mode Linux.
//
module UMLNet
    submodules:
        worldProcessor: WorldProcessor;
            display: "p=60,264;i=bwgen_s;b=17,17";
        client1: UMLClient;
            parameters:
                // network parameters
                IPv6routingFile = "network.xml",

                IPForward = false;
            gatesizes:
                in[1],
                out[1];
            display: "b=36,32;p=42,162;i=comp";
        switch1: UMLSwitch;
            parameters:
                 // network parameters
                IPv6routingFile = "network.xml",

                IPForward = false;
            gatesizes:
                in[1],
                out[1];
            display: "b=32,15;p=130,94;i=ipc";
    connections nocheck:
        client1.out[0] --> switch1.in[0];
        client1.in[0] <-- switch1.out[0];
    display: "p=10,10;b=317,207";
endmodule

network umlnet : UMLNet
endnetwork

