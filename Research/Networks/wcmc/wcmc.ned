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
// @file   wcmc.ned
// @author Johnny Lai
// @date   26 Oct 2003
//
// @brief  TestFastRA when MN moves to new subnet and sends RS
//


import
    "WorldProcessor",
    "Router6",
    "UDPNode",
    "WirelessAccessPoint",
    "WirelessMobileNode";

channel wcmcCable
    datarate 100e6;
    delay 1e-3;
endchannel

module wcmc
    submodules:
        worldProcessor: WorldProcessor;
            display: "p=316,40;i=bwgen_s";
        client1: MobileNode;
            parameters:
                IPForward = false;
        gatesizes:
            wlin[1],
            wlout[1];
            display: "p=88,118;i=laptop3";
        server4: UDPNode;
            parameters:
                IPForward = false;
            gatesizes:
                in[1],
                out[1];
            display: "p=55,249;i=pc";
        ha: Router6;
            gatesizes:
                in[5],
                out[5];
            display: "p=216,248;i=router";
        ap1: AccessPoint;
            gatesizes:
                in[1],
                out[1];
            display: "p=134,158;i=switch1_s";
        ap2: AccessPoint;
            gatesizes:
                in[1],
                out[1];
            display: "p=306,158;i=switch1_s";
        ap3: AccessPoint;
            gatesizes:
                in[1],
                out[1];
            display: "p=306,330;i=switch1_s";
        ap4: AccessPoint;
            gatesizes:
                in[1],
                out[1];
            display: "p=134,330;i=switch1_s";
    connections nocheck:

        ha.in[0] <-- wcmcCable <-- ap1.out[0];
        ha.out[0] --> wcmcCable --> ap1.in[0];

        ap2.out[0] --> wcmcCable --> ha.in[1]; 
        ap2.in[0] <-- wcmcCable <-- ha.out[1]; 

        ap3.out[0] --> wcmcCable --> ha.in[2]; 
        ap3.in[0] <-- wcmcCable <-- ha.out[2]; 


        ap4.out[0] --> wcmcCable --> ha.in[3]; 
        ap4.in[0] <-- wcmcCable <-- ha.out[3]; 


        ha.out[4] --> wcmcCable --> server4.in[0];
        ha.in[4] <-- wcmcCable <-- server4.out[0];

    display: "p=2,10;b=425,415";
endmodule

network wcmcNet : wcmc
endnetwork

