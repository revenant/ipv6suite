// -*- Ned -*-
//
// Copyright (C) 2002 Eric Wu
//
// This file is part of IPv6Suite
//
// IPv6Suite is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// IPv6Suite is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.


//
// @file   MIPv6NetworkMacBridge.ned
// @author Eric Wu
// @date   10 Oct 2003
//
// @brief  test network for dedicated MAC bridge
//


import
    "EtherHub",
    "Router6",
    "UDPNode",
    "WorldProcessor",
    "WirelessAccessPoint",
    "EtherBridge",
    "WirelessMobileNode",
    "Cables";


module MIPv6NetworkMacBridge
    submodules:
        worldProcessor: WorldProcessor;
            display: "p=316,40;i=bwgen_s;b=17,17";
        client1: MobileNode; //
            parameters:
                // network parameters
                //# routingFile = "client1.irt",
                IPForward = false;
            display: "b=40,37;p=96,102;i=laptop3";
        server4: UDPNode;
            parameters:
                // network parameters
                //# routingFile = "server4.irt",
                IPForward = false;
            gatesizes:
                in[1],
                out[1];
            display: "b=38,32;p=39,441;i=pc";
        ha: Router6;
            gatesizes:
                in[4],
                out[4];
            display: "p=96,200;i=router;b=32,32";
        router1: Router6;
            gatesizes:
                in[4],
                out[4];
            display: "b=32,32;p=250,198;i=router";
        ap1: AccessPoint;
            gatesizes:
                in[1],
                out[1];
            display: "p=152,152;i=switch1_s;b=32,11";
        ap2: AccessPoint;
            gatesizes:
                in[1],
                out[1];
            display: "p=304,152;i=switch1_s;b=32,11";
        ap3: AccessPoint;
            gatesizes:
                in[1],
                out[1];
            display: "p=304,352;i=switch1_s;b=32,11";
        ap4: AccessPoint;
            gatesizes:
                in[1],
                out[1];
            display: "p=152,352;i=switch1_s;b=32,11";
        router2: Router6;
            gatesizes:
                in[4],
                out[4];
            display: "p=256,372;i=router;b=32,32";
        router3: Router6;
            gatesizes:
                in[5],
                out[5];
            display: "p=92,368;i=router;b=32,32";
        hub1: Hub;
            gatesizes:
                in[3],
                out[3];
            display: "p=100,152;b=24,16";
        hub2: Hub;
            gatesizes:
                in[3],
                out[3];
            display: "p=252,152;b=24,16";
        hub3: Hub;
            gatesizes:
                in[3],
                out[3];
            display: "p=308,392;b=24,16";
        hub4: Hub;
            gatesizes:
                in[3],
                out[3];
            display: "p=140,400;b=24,16";
        bridge: EtherBridge;
            parameters:
            entryTimeout = 10;
            gatesizes:
                in[4],
                out[4];
            display: "p=212,284;b=24,24";
    connections nocheck:

        // Correct for ping test
        ha.in[0] <-- intranetCable <-- hub1.out[0];
        ha.out[0] --> intranetCable --> hub1.in[0];

        hub1.in[1] <-- intranetCable <-- ap1.out[0];
        hub1.out[1] --> intranetCable --> ap1.in[0];

        ap2.out[0] --> intranetCable --> hub2.in[1];
        ap2.in[0] <-- intranetCable <-- hub2.out[1];

        hub2.out[0] --> intranetCable --> router1.in[0];
        hub2.in[0] <-- intranetCable <-- router1.out[0];

        router1.out[1] --> intranetCable --> ha.in[1];
        router1.in[1] <-- intranetCable <-- ha.out[1];

        ap3.out[0] --> intranetCable --> hub3.in[1];
        ap3.in[0] <-- intranetCable <-- hub3.out[1];

        hub3.out[0] --> intranetCable --> router2.in[0];
        hub3.in[0] <-- intranetCable <-- router2.out[0];

        router2.out[1] --> internetCable --> router1.in[2];
        router2.in[1] <-- internetCable <-- router1.out[2];

        ap4.out[0] --> intranetCable --> hub4.in[1];
        ap4.in[0] <-- intranetCable <-- hub4.out[1];

        hub4.out[0] --> intranetCable --> router3.in[0];
        hub4.in[0] <-- intranetCable <-- router3.out[0];

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

    hub1.in[2] <-- internetCable <-- bridge.out[0];
    hub1.out[2] --> internetCable --> bridge.in[0];

    hub2.in[2] <-- internetCable <-- bridge.out[1];
    hub2.out[2] --> internetCable --> bridge.in[1];

    hub3.in[2] <-- internetCable <-- bridge.out[2];
    hub3.out[2] --> internetCable --> bridge.in[2];

    hub4.in[2] <-- internetCable <-- bridge.out[3];
    hub4.out[2] --> internetCable --> bridge.in[3];


    display: "p=2,10;b=461,539";
endmodule


network mipv6Network2 : MIPv6NetworkMacBridge
endnetwork

