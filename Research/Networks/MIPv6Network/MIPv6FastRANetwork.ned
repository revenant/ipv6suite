// -*- Ned -*-
// $Header: /home/cvs/IPv6Suite/IPv6SuiteWithINET/Research/Networks/MIPv6Network/MIPv6FastRANetwork.ned,v 1.1 2005/02/09 06:15:57 andras Exp $
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
// @file   MIPv6FastRANetwork.ned
// @author Johnny Lai
// @date   26 Oct 2003
// 
// @brief  TestFastRA when MN moves to new subnet and sends RS
// 


import  
    "EtherHub", 
    "Router6", 
    "UDPNode", 
    "WorldProcessor", 
    "WirelessAccessPoint", 
    "WirelessMobileNode",
    "Cables";
                          
 
module MIPv6FastRANetwork 
    submodules: 
        worldProcessor: WorldProcessor; 
            display: "p=316,40;i=bwgen_s;b=17,17";
        client1: MobileNode; 
            parameters: 
                IPForward = false; 
	    gatesizes: 
	        wlin[1],
	        wlout[1];
            display: "b=32,32;p=88,118;i=laptop3";
        server4: UDPNode; 
            parameters: 
                IPForward = false; 
            gatesizes: 
                in[1], 
                out[1]; 
            display: "b=38,32;p=55,249;i=pc";
        ha: Router6; 
            gatesizes: 
                in[5], 
                out[5]; 
            display: "p=216,248;i=router;b=32,32";
        ap1: AccessPoint; 
            gatesizes: 
                in[1], 
                out[1]; 
            display: "p=134,158;i=switch1_s;b=32,32";
        ap2: AccessPoint; 
            gatesizes: 
                in[1], 
                out[1]; 
            display: "p=306,158;i=switch1_s;b=32,32";
        ap3: AccessPoint; 
            gatesizes: 
                in[1], 
                out[1]; 
            display: "p=306,330;i=switch1_s;b=32,11";
        ap4: AccessPoint; 
            gatesizes: 
                in[1], 
                out[1]; 
            display: "p=134,330;i=switch1_s;b=32,11";
    connections nocheck: 
                         
        // Correct for ping test
        ha.in[0] <-- intranetCable <-- ap1.out[0]; 
        ha.out[0] --> intranetCable --> ap1.in[0]; 
                                                   
        ap2.out[0] --> intranetCable --> ha.in[1]; //router1.in[0]; 
        ap2.in[0] <-- intranetCable <-- ha.out[1]; //router1.out[0]; 
                                                   
                                                   //        router1.out[1] --> intranetCable --> ha.in[1]; 
                                                   //        router1.in[1] <-- intranetCable <-- ha.out[1]; 
                                                   
        ap3.out[0] --> intranetCable --> ha.in[2]; //router2.in[0]; 
        ap3.in[0] <-- intranetCable <-- ha.out[2]; //router2.out[0]; 
                                                   
                                                   //        router2.out[1] --> internetCable --> router1.in[2]; 
                                                   //        router2.in[1] <-- internetCable <-- router1.out[2]; 
                                                   
        ap4.out[0] --> intranetCable --> ha.in[3]; //router3.in[0]; 
        ap4.in[0] <-- intranetCable <-- ha.out[3]; //router3.out[0]; 
                                                   
                                                   //        router3.out[1] --> internetCable --> router2.in[2]; 
                                                   //        router3.in[1] <-- internetCable <-- router2.out[2]; 
                                                   
                                                   //        router3.out[2] --> intranetCable --> server4.in[0]; 
                                                   //        router3.in[2] <-- intranetCable <-- server4.out[0]; 
                                                   
                                                   
        ha.out[4] --> internetCable --> server4.in[0]; 
        ha.in[4] <-- internetCable <-- server4.out[0]; 
                                                       
                                                       //        ha.in[2] <-- intranetCable <-- router3.out[3]; 
                                                       //        ha.out[2] --> intranetCable --> router3.in[3]; 
                                                       
                                                       //        ha.in[3] <-- intranetCable <-- router2.out[3]; 
                                                       //        ha.out[3] --> intranetCable --> router2.in[3]; 
                                                       
                                                       //        router1.out[3] --> internetCable --> router3.in[4]; 
                                                       //        router1.in[3] <-- internetCable <-- router3.out[4]; 
                                                       
    display: "p=2,10;b=425,415";
endmodule 

network mipv6fastRANet : MIPv6FastRANetwork 
endnetwork 

