// -*- Ned -*-
// Copyright (C) 2002, 2004 Johnny Lai
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
// @file   HMIPv6Sait.ned
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
    "WirelessMobileNode";                          

channel HMIPv6SaitInternetCable 
//    delay 5e-2; 
//    delay 2e-1;
//    delay 5e-1;
      delay 1e-2;
    datarate 10e9; 
endchannel 

channel HMIPv6SaitIntranetCable 
    delay 1.5e-6; // propagation delay for 30 meter link
    datarate 100e6; 
endchannel 

channel SaitMapDomainCable
//Large delay means large map domain/ small means small map domain
    delay 2e-2;
//    delay 2e-3;
//never did this not about to start either
//    delay 5e-2;
    datarate 1e9;
endchannel

module HMIPv6Sait 
    submodules: 
        worldProcessor: WorldProcessor; 
            display: "p=672,31;i=bwgen_s;b=17,17";
        client1: MobileNode; 
            parameters: 
	    gatesizes: 
	        wlin[1],
	        wlout[1];
            display: "b=1,1;p=32,316;i=laptop3";
        server: UDPNode; 
            parameters: 
	    //Complains about invalid type D for this
//                IPForward = false; 
            gatesizes: 
                in[1], 
                out[1]; 
            display: "b=38,32;p=407,41;i=pc";
        ar: Router6; 
            gatesizes: 
                in[5], 
                out[5]; 
            display: "p=480,208;i=router;b=32,32";
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
        map: Router6; 
            gatesizes: 
                in[4], 
                out[4]; 
            display: "p=280,136;i=router;b=32,32";
        ar2: Router6; 
            gatesizes: 
                in[5], 
                out[5]; 
            display: "p=480,416;i=router;b=32,32";
        apa: AccessPoint; 
            gatesizes: 
                in[1], 
                out[1]; 
            display: "p=250,436;i=switch1_s;b=32,11";
        apb: AccessPoint; 
            gatesizes: 
                in[1], 
                out[1]; 
            display: "p=400,436;i=switch1_s;b=32,11";
        apc: AccessPoint; 
            gatesizes: 
                in[1], 
                out[1]; 
            display: "p=550,436;i=switch1_s;b=32,11";
        apd: AccessPoint; 
            gatesizes: 
                in[1], 
                out[1]; 
            display: "p=700,436;i=switch1_s;b=32,11";
        
    connections nocheck: 
        ar.in[0] <-- HMIPv6SaitIntranetCable <-- ap1.out[0]; 
        ar.out[0] --> HMIPv6SaitIntranetCable --> ap1.in[0]; 
                                                   
        ap2.out[0] --> HMIPv6SaitIntranetCable --> ar.in[1]; 
        ap2.in[0] <-- HMIPv6SaitIntranetCable <-- ar.out[1]; 
                                                   
        ap3.out[0] --> HMIPv6SaitIntranetCable --> ar.in[2]; 
        ap3.in[0] <-- HMIPv6SaitIntranetCable <-- ar.out[2]; 
                                                   
        ap4.out[0] --> HMIPv6SaitIntranetCable --> ar.in[3]; 
        ap4.in[0] <-- HMIPv6SaitIntranetCable <-- ar.out[3]; 

        map.out[0] --> HMIPv6SaitInternetCable --> server.in[0]; 
        map.in[0] <-- HMIPv6SaitInternetCable <-- server.out[0]; 

	//Using this prob. will not show much improvement with HMIP                                                      
        //ar.out[4] --> internetCable --> map.in[2]; 
        //ar.in[4] <-- internetCable <-- map.out[2];

	//Using this will not show improvement with PCOAF if hmip on
        //ar.out[4] --> intranetCable --> map.in[2]; 
        //ar.in[4] <-- intranetCable <-- map.out[2];

        ar.out[4] --> SaitMapDomainCable --> map.in[2]; 
        ar.in[4] <-- SaitMapDomainCable <-- map.out[2];


	map.out[1] --> HMIPv6SaitInternetCable --> ha.in[0];
	map.in[1] <-- HMIPv6SaitInternetCable <-- ha.out[0];

        hap.out[0] --> HMIPv6SaitIntranetCable --> ha.in[1]; 
        hap.in[0] <-- HMIPv6SaitIntranetCable <-- ha.out[1]; 

        ar2.in[0] <-- HMIPv6SaitIntranetCable <-- apa.out[0]; 
        ar2.out[0] --> HMIPv6SaitIntranetCable --> apa.in[0]; 
                                                   
        apb.out[0] --> HMIPv6SaitIntranetCable --> ar2.in[1]; 
        apb.in[0] <-- HMIPv6SaitIntranetCable <-- ar2.out[1]; 
                                                   
        apc.out[0] --> HMIPv6SaitIntranetCable --> ar2.in[2]; 
        apc.in[0] <-- HMIPv6SaitIntranetCable <-- ar2.out[2]; 
                                                   
        apd.out[0] --> HMIPv6SaitIntranetCable --> ar2.in[3]; 
        apd.in[0] <-- HMIPv6SaitIntranetCable <-- ar2.out[3]; 

        ar2.out[4] --> SaitMapDomainCable --> map.in[3]; 
        ar2.in[4] <-- SaitMapDomainCable <-- map.out[3];
                                                   
    display: "p=2,10;b=763,763";
endmodule 

network hmipv6SaitNet : HMIPv6Sait 
endnetwork 

