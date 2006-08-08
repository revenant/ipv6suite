//
// Copyright (C) 2006 Johnny Lai
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
    "UDPNode",
    "WirelessAccessPoint",
    "WirelessMobileNode",
    "WorldProcessor";



//
// Ethernet cable, with a delay that corresponds to a 30 meter link.
//
channel EtherLink
    delay 1.5e-7; // propagation delay for 30 meter link
endchannel


//
// Network to test Wireless behaviour with Voip traffic
//
module MultiBaselineNetwork
    parameters:
        numOfClients : numeric;
    submodules:
        worldProcessor: WorldProcessor;
            display: "p=260,264;i=bwgen_s";
//	   mn: MobileNode[numOfClients];
//	       gatesizes:
//		   wlin[1],
//		   wlout[1];
//	       display: "p=50,70;i=laptop";
        mn:  MobileNode
            gatesizes:
                wlin[1],
                wlout[1];
            display: "p=50,70;i=laptop";
        mn2: MobileNode
            gatesizes:
                wlin[1],
                wlout[1];
            display: "p=100,70;i=laptop";
        ap: AccessPoint;
            gatesizes:
                in[1],
                out[1];
            display: "p=150,70;i=pctower1";
        cn: UDPNode;
            gatesizes:
                in[1],
                out[1];
            display: "p=150,130;i=pc";
    connections nocheck:
        ap.out[0] --> EtherLink --> cn.in[0];
        ap.in[0] <-- EtherLink <-- cn.out[0];

    display: "p=10,10;b=303,288";
endmodule

network multibaselineNet : MultiBaselineNetwork
endnetwork
