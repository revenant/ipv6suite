//
// Copyright (C) 2001, 2003 Monash University, Australia
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


import  //
    "EtherHub",
    "Router6",
    "UDPNode",
    "WorldProcessor",
    "WirelessAccessPoint",
    "WirelessMobileNode",
    "Cables",
    "FlatNetworkConfigurator6";




//
// A MIPv6 test network containing a server, several routers with
// wireless access points, and a laptop as mobile node.
//
// Can be used to test handovers between access points.
// Mobile node is pinging the server while it's moving.
//
module MIPv6NetworkSimulMove
    submodules:
        worldProcessor: WorldProcessor;
            display: "p=316,40;i=bwgen_s";
        client1: MobileNode; //
            parameters:
                IPForward = false;
            gatesizes:
                wlin[1],
                wlout[1];
            display: "p=88,118;i=laptop3";
        server4: MobileNode;
            parameters:
                //# routingFile = "server4.irt",
                IPForward = false;
            gatesizes:
                wlin[1],
                wlout[1];
            display: "p=88,369;i=laptop3";
        ha1: Router6;
            gatesizes:
                in[4],
                out[4];
            display: "p=95,200;i=router";
        router1: Router6;
            gatesizes:
                in[4],
                out[4];
            display: "p=250,198;i=router";
        ap1: AccessPoint;
            gatesizes:
                in[1],
                out[1];
            display: "p=135,160;i=switch1_s";
        ap2: AccessPoint;
            gatesizes:
                in[1],
                out[1];
            display: "p=306,158;i=switch1_s";
        ap3: AccessPoint;
            gatesizes:
                in[1],
                out[1];
            display: "p=300,340;i=switch1_s";
        ap4: AccessPoint;
            gatesizes:
                in[1],
                out[1];
            display: "p=142,338;i=switch1_s";
        router2: Router6;
            gatesizes:
                in[4],
                out[4];
            display: "p=248,292;i=router";
        ha2: Router6;
            gatesizes:
                in[5],
                out[5];
            display: "p=92,296;i=router";
        ewu: UDPNode;
            parameters:
                IPForward = false;
            gatesizes:
                in[1],
                out[1];
            display: "p=32,296;i=pc";
        configurator6: FlatNetworkConfigurator6;
            parameters:
                moduleTypes = "Router6 MobileNode UDPNode",
                nonIPModuleTypes = "AccessPoint ";
            display: "p=396,396;i=block/cogwheel";
    connections nocheck:

        // Correct for ping test
        ha1.in[0] <-- intranetCable <-- ap1.out[0];
        ha1.out[0] --> intranetCable --> ap1.in[0];

        ap2.out[0] --> intranetCable --> router1.in[0];
        ap2.in[0] <-- intranetCable <-- router1.out[0];

        router1.out[1] --> intranetCable --> ha1.in[1];
        router1.in[1] <-- intranetCable <-- ha1.out[1];

        ap3.out[0] --> intranetCable --> router2.in[0];
        ap3.in[0] <-- intranetCable <-- router2.out[0];

        router2.out[1] --> internetCable --> router1.in[2];
        router2.in[1] <-- internetCable <-- router1.out[2];

        ap4.out[0] --> intranetCable --> ha2.in[0];
        ap4.in[0] <-- intranetCable <-- ha2.out[0];

        ha2.out[1] --> internetCable --> router2.in[2];
        ha2.in[1] <-- internetCable <-- router2.out[2];

        ha2.out[2] --> intranetCable --> ewu.in[0];
        ha2.in[2] <-- intranetCable <-- ewu.out[0];

        ha1.in[2] <-- intranetCable <-- ha2.out[3];
        ha1.out[2] --> intranetCable --> ha2.in[3];

        ha1.in[3] <-- intranetCable <-- router2.out[3];
        ha1.out[3] --> intranetCable --> router2.in[3];

        router1.out[3] --> internetCable --> ha2.in[4];
        router1.in[3] <-- internetCable <-- ha2.out[4];

    display: "p=2,10;b=461,539";
endmodule


network mipv6NetworkSimulMove : MIPv6NetworkSimulMove
endnetwork

