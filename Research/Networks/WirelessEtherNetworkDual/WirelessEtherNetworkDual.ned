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
    "DualIntMobileNode",
    "WorldProcessor";



//
// Ethernet cable, with a delay that corresponds to a 30 meter link.
//
channel EtherLink
    delay 1.5e-7; // propagation delay for 30 meter link
endchannel


//
// Network to test wireless handovers. Two mobile nodes move between
// wireless access points, while they are pinging the printer
// on the Ethernet LAN also containing the APs and further hosts.
//
module WirelessEtherNetworkDual
    submodules:
        worldProcessor: WorldProcessor;
            display: "p=68,432;i=bwgen_s";
        ms1: DualIntMobileNode;
            gatesizes:
                wlin[2];
            display: "p=60,120;i=laptop";
        ap1: AccessPoint;
            gatesizes:
                in[1],
                out[1];
            display: "p=100,156;i=pctower1";
        ap2: AccessPoint;
            gatesizes:
                in[1],
                out[1];
            display: "p=216,152;i=pctower1";
        ap3: AccessPoint;
            gatesizes:
                in[1],
                out[1];
            display: "p=292,152;i=pctower1";
        hub: Hub;
            gatesizes:
                in[6],
                out[6];
            display: "p=160,252;i=switch1";
        swoon: UDPNode;
            gatesizes:
                in[1],
                out[1];
            display: "p=52,336;i=pc";
        gdaley: UDPNode;
            gatesizes:
                in[1],
                out[1];
            display: "p=160,340;i=pc";
        printer: UDPNode;
            gatesizes:
                in[1],
                out[1];
            display: "p=284,340;i=printer";
    connections nocheck:
        ap1.out[0] --> EtherLink --> hub.in[0];
        ap1.in[0] <-- EtherLink <-- hub.out[0];

        ap2.out[0] --> EtherLink --> hub.in[1];
        ap2.in[0] <-- EtherLink <-- hub.out[1];

        ap3.out[0] --> EtherLink --> hub.in[5];
        ap3.in[0] <-- EtherLink <-- hub.out[5];

        swoon.out[0] --> EtherLink --> hub.in[2];
        swoon.in[0] <-- EtherLink <-- hub.out[2];

        gdaley.out[0] --> EtherLink --> hub.in[3];
        gdaley.in[0] <-- EtherLink <-- hub.out[3];

        printer.out[0] --> EtherLink --> hub.in[4];
        printer.in[0] <-- EtherLink <-- hub.out[4];
    display: "p=10,10;b=471,488";
endmodule

network wirelessEtherNetworkDual : WirelessEtherNetworkDual
endnetwork
