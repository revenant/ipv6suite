// -*- Ned -*-
//
// Copyright (C) 2002 Johnny Lai
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
// @file   HMIPv6Simple.ned
// @author Johnny Lai
// @date   22 Nov 2003
//
// @brief
//


import
    "Router6",
    "UDPNode",
    "WorldProcessor",
    "WirelessAccessPoint",
    "WirelessMobileNode",
    "Cables";







channel pcoafForwardingInternetCable
//    delay 5e-2;
//    delay 1e-1;
    delay 5e-1;
    datarate 10e9;
endchannel

channel pcoafForwardingIntranetCable
    delay 1.5e-6; // propagation delay for 30 meter link
    datarate 100000000;
endchannel

module PCOAForwarding
    submodules:
        worldProcessor: WorldProcessor;
            display: "p=672,31;i=bwgen_s;b=17,17";
        client1: MobileNode;
            parameters:
                IPForward = false;
        gatesizes:
            wlin[1],
            wlout[1];
            display: "b=1,1;p=32,334;i=laptop3";
        server: UDPNode;
            parameters:
                IPForward = false;
            gatesizes:
                in[1],
                out[1];
            display: "b=38,32;p=407,41;i=pc";
        router: Router6;
            gatesizes:
                in[6],
                out[6];
            display: "p=216,128;i=router;b=32,32";
        ap1: AccessPoint;
            gatesizes:
                in[1],
                out[1];
            display: "p=250,286;i=switch1_s;b=32,11";
        ap2: AccessPoint;
            gatesizes:
                in[1],
                out[1];
            display: "p=400,286;i=switch1_s;b=32,11";
        ap3: AccessPoint;
            gatesizes:
                in[1],
                out[1];
            display: "p=550,286;i=switch1_s;b=32,11";
        ap4: AccessPoint;
            gatesizes:
                in[1],
                out[1];
            display: "p=700,286;i=switch1_s;b=32,11";
        ha: Router6;
            gatesizes:
                in[2],
                out[2];
            display: "p=72,56;i=router;b=32,32";
        hap: AccessPoint;
            gatesizes:
                in[1],
                out[1];
            display: "p=100,286;i=switch1_s;b=32,11";
    connections nocheck:
        router.in[0] <-- intranetCable <-- ap1.out[0];
        router.out[0] --> intranetCable --> ap1.in[0];

        ap2.out[0] --> intranetCable --> router.in[1];
        ap2.in[0] <-- intranetCable <-- router.out[1];

        ap3.out[0] --> intranetCable --> router.in[2];
        ap3.in[0] <-- intranetCable <-- router.out[2];

        ap4.out[0] --> intranetCable --> router.in[3];
        ap4.in[0] <-- intranetCable <-- router.out[3];



        router.out[4] --> pcoafForwardingInternetCable --> server.in[0];
        router.in[4] <-- pcoafForwardingInternetCable <-- server.out[0];

        router.out[5] --> pcoafForwardingInternetCable --> ha.in[0];
        router.in[5] <-- pcoafForwardingInternetCable <-- ha.out[0];

        hap.out[0] --> pcoafForwardingIntranetCable --> ha.in[1];
        hap.in[0] <-- pcoafForwardingIntranetCable <-- ha.out[1];

    display: "p=2,10;b=763,411";
endmodule

network pcoaForwardingNet : PCOAForwarding
endnetwork

