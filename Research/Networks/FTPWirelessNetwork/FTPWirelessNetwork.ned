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
    "EtherHub",
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
// Network to test Wireless MS ftp transfer to fixed node
//
module FTPWirelessNetwork
    submodules:
        worldProcessor: WorldProcessor;
            display: "p=260,264;i=bwgen_s";
        ms1: MobileNode;
            gatesizes:
                wlin[1],
                wlout[1];
            display: "p=50,70;i=laptop";
        ap1: AccessPoint;
            gatesizes:
                in[1],
                out[1];
            display: "p=150,70;i=pctower1";
        hub: Hub;
            gatesizes:
                in[2],
                out[2];
            display: "p=150,100;i=switch1";
        server: UDPNode;
            gatesizes:
                in[1],
                out[1];
            display: "p=150,130;i=pc";
    connections nocheck:
        ap1.out[0] --> EtherLink --> hub.in[0];
        ap1.in[0] <-- EtherLink <-- hub.out[0];

        server.out[0] --> EtherLink --> hub.in[1];
        server.in[0] <-- EtherLink <-- hub.out[1];

    display: "p=10,10;b=303,288";
endmodule

network ftpWirelessNetwork : FTPWirelessNetwork
endnetwork

