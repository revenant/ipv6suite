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
            display: "p=68,432;i=bwgen_s;b=17,17";
        ms1: MobileNode; 
            display: "p=60,120;i=laptop;b=32,29";
        ms2: MobileNode; 
            display: "p=60,120;i=laptop;b=32,29";
        ms3: MobileNode; 
            display: "p=60,120;i=laptop;b=32,29";
        ms4: MobileNode; 
            display: "p=60,120;i=laptop;b=32,29";
        ms5: MobileNode; 
            display: "p=60,120;i=laptop;b=32,29";
        ms6: MobileNode; 
            display: "p=60,120;i=laptop;b=32,29";
        ms7: MobileNode; 
            display: "p=60,120;i=laptop;b=32,29";
        ms8: MobileNode; 
            display: "p=60,120;i=laptop;b=32,29";
//        ms9: MobileNode; 
//            display: "p=60,120;i=laptop;b=32,29";
//        ms10: MobileNode; 
//            display: "p=60,120;i=laptop;b=32,29";
//        ms11: MobileNode; 
//            display: "p=60,120;i=laptop;b=32,29";
//        ms12: MobileNode; 
//            display: "p=60,120;i=laptop;b=32,29";
//        ms13: MobileNode; 
//            display: "p=60,120;i=laptop;b=32,29";
//        ms14: MobileNode; 
//            display: "p=60,120;i=laptop;b=32,29";
//        ms15: MobileNode; 
//            display: "p=60,120;i=laptop;b=32,29";
//        ms16: MobileNode; 
//            display: "p=60,120;i=laptop;b=32,29";
//        ms17: MobileNode; 
//            display: "p=60,120;i=laptop;b=32,29";
//        ms18: MobileNode; 
//            display: "p=60,120;i=laptop;b=32,29";
//        ms19: MobileNode; 
//            display: "p=60,120;i=laptop;b=32,29";
//        ms20: MobileNode; 
//            display: "p=60,120;i=laptop;b=32,29";
        ap1: AccessPoint; 
            gatesizes: 
                in[1], 
                out[1]; 
            display: "p=100,156;i=pctower1;b=40,24";
        ap2: AccessPoint; 
            gatesizes: 
                in[1], 
                out[1]; 
            display: "p=216,152;i=pctower1;b=18,31";
        gdaley: UDPNode; 
            gatesizes: 
                in[2], 
                out[2]; 
            display: "p=160,340;i=pc;b=38,32";
    connections nocheck: 
        ap1.out[0] --> gdaley.in[0];
        ap1.in[0] <-- gdaley.out[0]; 
                                                
        ap2.out[0] --> gdaley.in[1];
        ap2.in[0] <-- gdaley.out[1];
                                                
    display: "p=10,10;b=471,488";
endmodule 

network wirelessTest : WirelessTest 
endnetwork 

