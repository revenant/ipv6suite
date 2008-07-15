// -*- Ned -*-
// Copyright (C) 2004 Johnny Lai
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
// @file   TwoRouters.ned
// @author Johnny Lai
// @date   17 Aug 2004
// 
// @brief  Similar to PingNetwork -r 1 in topology but shows full autoconf
// process as routers advertise
// 


import  
    "WorldProcessor", 
    "Router6", 
    "UDPNode"; 
               
 
channel TwoRoutersInternetCable 
    delay 1e-2; 
    datarate 1e6; 
endchannel 

module TwoRouters 
    submodules: 
        worldProcessor: WorldProcessor; 
            display: "p=188,32;i=bwgen_s";
        client1: UDPNode; 
            gatesizes: 
                in[1], 
                out[1]; 
            display: "p=58,50;i=comp";
        client2: UDPNode; 
            gatesizes: 
                in[1], 
                out[1]; 
            display: "p=49,165;i=comp";
        server1: UDPNode; 
            gatesizes: 
                in[1], 
                out[1]; 
            display: "p=272,44;i=comp";
        server2: UDPNode; 
            gatesizes: 
                in[1], 
                out[1]; 
            display: "p=271,161;i=pc";
        routerInt: Router6; 
            gatesizes: 
                in[3], 
                out[3]; 
            display: "p=130,94;i=ipc";
        routerExt: Router6; 
            gatesizes: 
                in[3], 
                out[3]; 
            display: "p=210,94;i=ipc";
    connections nocheck: 
        client1.out[0] --> routerInt.in[0]; 
        client1.in[0] <-- routerInt.out[0]; 
                                            
        client2.out[0] --> routerInt.in[1]; 
        client2.in[0] <-- routerInt.out[1]; 
                                            
        routerInt.out[2] --> TwoRoutersInternetCable --> routerExt.in[2]; 
        routerInt.in[2] <-- TwoRoutersInternetCable <-- routerExt.out[2]; 
                                                                          
        server1.out[0] --> routerExt.in[0]; 
        server1.in[0] <-- routerExt.out[0]; 
                                            
        server2.out[0] --> routerExt.in[1]; 
        server2.in[0] <-- routerExt.out[1]; 
                                            
endmodule 

network twoRouters : TwoRouters 
endnetwork 

