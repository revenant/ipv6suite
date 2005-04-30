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
    "Cables";


//
// A MIPv6 test network containing a server, several routers with
// wireless access points, and a laptop as mobile node.
//
// Can be used to test handovers between access points.
// Mobile node is pinging the server while it's moving.
//
module MIPv6Network
    submodules:
        worldProcessor: WorldProcessor;
            display: "p=436,72;i=misc/sun";
        client1: MobileNode; //
            parameters:
                //# routingFile = "client1.irt",
                IPForward = false;
            gatesizes:
                wlin[1],
                wlout[1];
            display: "p=120,142;i=device/palm";
        server4: UDPNode;
            parameters:
                //# routingFile = "server4.irt",
                IPForward = false;
            gatesizes:
                in[1],
                out[1];
            display: "p=39,521;i=device/pc2";
        ha: Router6;
            gatesizes:
                in[4],
                out[4];
            display: "p=120,224;i=device/router";
        router1: Router6;
            gatesizes:
                in[4],
                out[4];
            display: "p=410,222;i=device/router";
        ap1: AccessPoint;
            gatesizes:
                in[1],
                out[1];
            display: "p=190,190;i=device/accesspoint";
        ap2: AccessPoint;
            gatesizes:
                in[1],
                out[1];
            display: "p=474,182;i=device/accesspoint";
        ap3: AccessPoint;
            gatesizes:
                in[1],
                out[1];
            display: "p=474,578;i=device/accesspoint";
        ap4: AccessPoint;
            gatesizes:
                in[1],
                out[1];
            display: "p=182,578;i=device/accesspoint";
        router2: Router6;
            gatesizes:
                in[4],
                out[4];
            display: "p=416,524;i=device/router";
        router3: Router6;
            gatesizes:
                in[5],
                out[5];
            display: "p=116,520;i=device/router";
        configurator6: FlatNetworkConfigurator6;
            parameters:
                moduleTypes = "Router6 MobileNode UDPNode",
                nonIPModuleTypes = "AccessPoint ";
            display: "p=340,76;i=block/cogwheel";
    connections nocheck:

        // Correct for ping test
        ha.in[0] <-- intranetCable <-- ap1.out[0];
        ha.out[0] --> intranetCable --> ap1.in[0];

        ap2.out[0] --> intranetCable --> router1.in[0];
        ap2.in[0] <-- intranetCable <-- router1.out[0];

        router1.out[1] --> intranetCable --> ha.in[1];
        router1.in[1] <-- intranetCable <-- ha.out[1];

        ap3.out[0] --> intranetCable --> router2.in[0];
        ap3.in[0] <-- intranetCable <-- router2.out[0];

        router2.out[1] --> internetCable --> router1.in[2];
        router2.in[1] <-- internetCable <-- router1.out[2];

        ap4.out[0] --> intranetCable --> router3.in[0];
        ap4.in[0] <-- intranetCable <-- router3.out[0];

        router3.out[1] --> internetCable --> router2.in[2];
        router3.in[1] <-- internetCable <-- router2.out[2];

        router3.out[2] --> intranetCable --> server4.in[0];
        router3.in[2] <-- intranetCable <-- server4.out[0];

        ha.in[2] <-- intranetCable <-- router3.out[3];
        ha.out[2] --> intranetCable --> router3.in[3];

        ha.in[3] <-- intranetCable <-- router2.out[3];
        ha.out[3] --> intranetCable --> router2.in[3];

        router1.out[3] --> internetCable --> router3.in[4];
        router1.in[3] <-- internetCable <-- router3.out[4];

    display: "b=613,705";
endmodule


network mipv6Network : MIPv6Network
endnetwork

