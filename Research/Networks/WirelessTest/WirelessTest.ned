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


module WirelessTest
    parameters:
        num_clients : numeric const;
    submodules:
        worldProcessor: WorldProcessor;
            display: "p=68,432;i=bwgen_s";
        ms1: MobileNode;
            display: "p=60,120;i=laptop";
        ms2: MobileNode;
            display: "p=60,120;i=laptop";
        ms3: MobileNode;
            display: "p=60,120;i=laptop";
        ms4: MobileNode;
            display: "p=60,120;i=laptop";
        ms5: MobileNode;
            display: "p=60,120;i=laptop";
        ms6: MobileNode;
            display: "p=60,120;i=laptop";
        ms7: MobileNode;
            display: "p=60,120;i=laptop";
        ms8: MobileNode;
            display: "p=60,120;i=laptop";
//        ms9: MobileNode;
//            display: "p=60,120;i=laptop";
//        ms10: MobileNode;
//            display: "p=60,120;i=laptop";
//        ms11: MobileNode;
//            display: "p=60,120;i=laptop";
//        ms12: MobileNode;
//            display: "p=60,120;i=laptop";
//        ms13: MobileNode;
//            display: "p=60,120;i=laptop";
//        ms14: MobileNode;
//            display: "p=60,120;i=laptop";
//        ms15: MobileNode;
//            display: "p=60,120;i=laptop";
//        ms16: MobileNode;
//            display: "p=60,120;i=laptop";
//        ms17: MobileNode;
//            display: "p=60,120;i=laptop";
//        ms18: MobileNode;
//            display: "p=60,120;i=laptop";
//        ms19: MobileNode;
//            display: "p=60,120;i=laptop";
//        ms20: MobileNode;
//            display: "p=60,120;i=laptop";
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
        gdaley: UDPNode;
            gatesizes:
                in[2],
                out[2];
            display: "p=160,340;i=pc";
    connections nocheck:
        ap1.out[0] --> gdaley.in[0];
        ap1.in[0] <-- gdaley.out[0];

        ap2.out[0] --> gdaley.in[1];
        ap2.in[0] <-- gdaley.out[1];

    display: "p=10,10;b=471,488";
endmodule

network wirelessTest : WirelessTest
endnetwork

