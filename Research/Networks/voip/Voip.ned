// -*- Ned -*-
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
// @file   Voip.ned
// 
// @brief  Added an extra AR because the very first handover is not counted
// 


import  
    "WorldProcessor", 
    "Router6", 
    "UDPNode", 
    "WorldProcessor", 
    "WirelessAccessPoint", 
    "WirelessMobileNode"; 
                          
                          
                          
 
channel VoipIntranetCable 
    datarate 100e6;
    delay 10e-3;
endchannel 

channel VoipInternetCable 
    datarate 100e6; 
    delay 50e-3;
endchannel 

module Voip 
    submodules: 
        worldProcessor: WorldProcessor; 
            display: "p=264,31;i=bwgen_s";
        mn: MobileNode; 
            parameters: 
	    gatesizes: 
	        wlin[1],
	        wlout[1];
            display: "p=40,92;i=laptop3";
        cn: UDPNode; 
            parameters: 
            //Complains about invalid type D for this
            //                IPForward = false; 
            gatesizes: 
                in[1], 
                out[1]; 
            display: "p=407,41;i=pc";
        ha: Router6; 
            gatesizes: 
                in[2], 
                out[2]; 
            display: "p=72,56;i=router";
        crh: Router6; 
            gatesizes: 
                in[3], 
                out[3]; 
            display: "p=280,80;i=router";
        crv: Router6; 
            gatesizes: 
                in[5], 
                out[5]; 
            display: "p=296,192;i=router";
        ar: Router6; 
            gatesizes: 
                in[2], 
                out[2]; 
            display: "p=168,296;i=router";
        hap: AccessPoint; 
            gatesizes: 
                in[1], 
                out[1]; 
            display: "p=66,170;i=switch1_s";
        apa: AccessPoint; 
            gatesizes: 
                in[1], 
                out[1]; 
            display: "p=106,320;i=switch1_s";
        ar2: Router6; 
            gatesizes: 
                in[2], 
                out[2]; 
            display: "p=304,296;i=router";
        apb: AccessPoint; 
            gatesizes: 
                in[1], 
                out[1]; 
            display: "p=266,320;i=switch1_s";
        ar3: Router6; 
            gatesizes: 
                in[2], 
                out[2]; 
            display: "p=368,248;i=router";
        apc: AccessPoint; 
            gatesizes: 
                in[1], 
                out[1]; 
            display: "p=426,320;i=switch1_s";
        ar4: Router6; 
            gatesizes: 
                in[2], 
                out[2]; 
            display: "p=480,216;i=router";
        apd: AccessPoint; 
            gatesizes: 
                in[1], 
                out[1]; 
            display: "p=586,320;i=switch1_s";
    connections nocheck: 
        cn.out[0] --> VoipIntranetCable --> crh.in[1]; 
        cn.in[0] <-- VoipIntranetCable <-- crh.out[1]; 
                                                            
        ha.out[0] --> VoipIntranetCable --> crh.in[0]; 
        ha.in[0] <-- VoipIntranetCable <-- crh.out[0]; 
                                                            
        crh.out[2] --> VoipInternetCable --> crv.in[3]; 
        crh.in[2] <-- VoipInternetCable <-- crv.out[3]; 
                                                             
        ar.out[0] --> VoipIntranetCable --> crv.in[0]; 
        ar.in[0] <-- VoipIntranetCable <-- crv.out[0]; 
                                                            
        ar2.out[0] --> VoipIntranetCable --> crv.in[1]; 
        ar2.in[0] <-- VoipIntranetCable <-- crv.out[1]; 
                                                             
        ar3.out[0] --> VoipIntranetCable --> crv.in[2]; 
        ar3.in[0] <-- VoipIntranetCable <-- crv.out[2]; 
                                                             
        ar4.out[0] --> VoipIntranetCable --> crv.in[4]; 
        ar4.in[0] <-- VoipIntranetCable <-- crv.out[4]; 
                                                             
        ha.out[1] --> VoipIntranetCable --> hap.in[0]; 
        ha.in[1] <-- VoipIntranetCable <-- hap.out[0]; 
                                                            
        ar.out[1] --> VoipIntranetCable --> apa.in[0]; 
        ar.in[1] <-- VoipIntranetCable <-- apa.out[0]; 
                                                            
        ar2.out[1] --> VoipIntranetCable --> apb.in[0]; 
        ar2.in[1] <-- VoipIntranetCable <-- apb.out[0]; 
                                                             
        ar3.out[1] --> VoipIntranetCable --> apc.in[0]; 
        ar3.in[1] <-- VoipIntranetCable <-- apc.out[0]; 
                                                             
        ar4.out[1] --> VoipIntranetCable --> apd.in[0]; 
        ar4.in[1] <-- VoipIntranetCable <-- apd.out[0]; 
                                                             
    display: "p=10,10;b=674,426";
endmodule 

network voipNet : Voip 
endnetwork 

