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
// Hyper-mega version of WirelessEtherNetwork, with a thousand wireless
// access points and a lot of mobile nodes.
//
module WirelessEtherNetwork2
    submodules:
        worldProcessor: WorldProcessor;
            display: "p=68,432;i=bwgen_s";
        ms1: MobileNode;
            gatesizes:
                wlin[1],
                wlout[1];
            display: "p=60,120;i=laptop";
        ms2: MobileNode;
            gatesizes:
                wlin[1],
                wlout[1];
            display: "p=510,120;i=laptop";
        ms3: MobileNode;
            gatesizes:
                wlin[1],
                wlout[1];
            display: "p=60,220;i=laptop";
        ms4: MobileNode;
            gatesizes:
                wlin[1],
                wlout[1];
            display: "p=510,220;i=laptop";
        ap1: AccessPoint;
            gatesizes:
                in[1],
                out[1];
            display: "p=100,160;i=pctower1";
        ap2: AccessPoint;
            gatesizes:
                in[1],
                out[1];
            display: "p=180,160;i=pctower1";
        ap3: AccessPoint;
            gatesizes:
                in[1],
                out[1];
            display: "p=260,160;i=pctower1";
        ap4: AccessPoint;
            gatesizes:
                in[1],
                out[1];
            display: "p=340,160;i=pctower1";
        ap5: AccessPoint;
            gatesizes:
                in[1],
                out[1];
            display: "p=420,160;i=pctower1";
        ap6: AccessPoint;
            gatesizes:
                in[1],
                out[1];
            display: "p=500,160;i=pctower1";
        hub1: Hub;
            gatesizes:
                in[4],
                out[4];
            display: "p=136,260;i=switch1";
        hub2: Hub;
            gatesizes:
                in[7],
                out[7];
            display: "p=292,260;i=switch1";
        hub3: Hub;
            gatesizes:
                in[4],
                out[4];
            display: "p=440,260;i=switch1";
        swoon: UDPNode;
            gatesizes:
                in[1],
                out[1];
            display: "p=200,340;i=pc";
        gdaley: UDPNode;
            gatesizes:
                in[1],
                out[1];
            display: "p=290,340;i=pc";
        printer: UDPNode;
            gatesizes:
                in[1],
                out[1];
            display: "p=380,340;i=printer";
        ewu: UDPNode;
            gatesizes:
                in[1],
                out[1];
            display: "p=110,340;i=pc";
        johnny: UDPNode;
            gatesizes:
                in[1],
                out[1];
            display: "p=470,340;i=pc";
    connections nocheck:
        ap1.out[0] --> EtherLink --> hub1.in[0];
        ap1.in[0] <-- EtherLink <-- hub1.out[0];

        ap2.out[0] --> EtherLink --> hub1.in[1];
        ap2.in[0] <-- EtherLink <-- hub1.out[1];

        ap3.out[0] --> EtherLink --> hub1.in[2];
        ap3.in[0] <-- EtherLink <-- hub1.out[2];

        hub2.out[0] --> EtherLink --> hub1.in[3];
        hub2.in[0] <-- EtherLink <-- hub1.out[3];

        hub3.out[0] --> EtherLink --> hub2.in[1];
        hub3.in[0] <-- EtherLink <-- hub2.out[1];

        hub3.out[1] --> EtherLink --> ap4.in[0];
        hub3.in[1] <-- EtherLink <-- ap4.out[0];

        hub3.out[2] --> EtherLink --> ap5.in[0];
        hub3.in[2] <-- EtherLink <-- ap5.out[0];

        hub3.out[3] --> EtherLink --> ap6.in[0];
        hub3.in[3] <-- EtherLink <-- ap6.out[0];

        swoon.out[0] --> EtherLink --> hub2.in[2];
        swoon.in[0] <-- EtherLink <-- hub2.out[2];

        gdaley.out[0] --> EtherLink --> hub2.in[3];
        gdaley.in[0] <-- EtherLink <-- hub2.out[3];

        printer.out[0] --> EtherLink --> hub2.in[4];
        printer.in[0] <-- EtherLink <-- hub2.out[4];

        ewu.out[0] --> EtherLink --> hub2.in[5];
        ewu.in[0] <-- EtherLink <-- hub2.out[5];

        johnny.out[0] --> EtherLink --> hub2.in[6];
        johnny.in[0] <-- EtherLink <-- hub2.out[6];

    display: "p=10,10;b=568,464";
endmodule

network wirelessEtherNetwork2 : WirelessEtherNetwork2
endnetwork

