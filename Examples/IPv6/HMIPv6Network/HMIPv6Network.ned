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
    "Router6",
    "UDPNode",
    "WorldProcessor",
    "WirelessAccessPoint",
    "WirelessMobileNode",
    "Cables";



//
// HMIPv6 test network containing a server, several routers with
// wireless access points, a MAP and a laptop as mobile node.
//
// Router 1 and 2 are in the MAP domain, and router 3 is not part
// of the domain. The network can be used to test inter-domain 
// handovers.
//
module HMIPv6Network
    submodules:
        worldProcessor: WorldProcessor;
            display: "p=316,40;i=bwgen_s;b=17,17";
        client1: MobileNode;
            parameters:
                IPForward = false;
	    gatesizes: 
	        wlin[1],
	        wlout[1];
            display: "b=32,32;p=88,118;i=laptop3";
        server4: UDPNode;
            parameters:
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
                in[5],
                out[5];
            display: "b=32,32;p=250,198;i=router";
        ap1: AccessPoint;
            gatesizes:
                in[1],
                out[1];
            display: "p=144,168;i=switch1_s;b=32,32";
        ap2: AccessPoint;
            gatesizes:
                in[1],
                out[1];
            display: "p=296,168;i=switch1_s;b=19,20";
        ap3: AccessPoint;
            gatesizes:
                in[1],
                out[1];
            display: "p=304,408;i=switch1_s;b=19,20";
        ap4: AccessPoint;
            gatesizes:
                in[1],
                out[1];
            display: "p=144,408;i=switch1_s;b=19,20";
        router2: Router6;
            gatesizes:
                in[5],
                out[5];
            display: "p=256,372;i=router;b=32,32";
        router3: Router6;
            gatesizes:
                in[6],
                out[6];
            display: "p=92,368;i=router;b=32,32";
        map1: Router6;
            gatesizes:
                in[3],
                out[3];
            display: "p=308,276;i=router2;b=55,45";
    connections nocheck:

        //Correct for ping test
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

        map1.out[0] --> internetCable --> router1.in[4];
        map1.in[0] <-- internetCable <-- router1.out[4];

        map1.out[1] --> internetCable --> router2.in[4];
        map1.in[1] <-- internetCable <-- router2.out[4];

        map1.out[2] --> internetCable --> router3.in[5];
        map1.in[2] <-- internetCable <-- router3.out[5];

    display: "p=2,10;b=461,539";
endmodule

network hmipv6Network : HMIPv6Network
endnetwork

