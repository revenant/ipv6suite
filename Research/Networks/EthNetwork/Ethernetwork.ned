//
// Copyright (C) 2001, 2004 Monash University, Australia
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
    "Router6", 
    "UDPNode", 
    "WorldProcessor"; 
                      
                      
                      
                      
 
//
// Model of an Ethernet LAN, with several hosts (UDPNode) connected
// via a hub, and an additional host connected through a router.
//
module Ethernetwork //
    parameters: 
        numOfClients : numeric; 
    submodules: 
        worldProcessor: WorldProcessor; 
            display: "p=76,384;i=bwgen_s;b=17,17";
        server: UDPNode; 
            gatesizes: 
                in[1], 
                out[1]; 
            display: "p=308,340;i=bwcomp;b=36,32";
        router: Router6; 
            gatesizes: 
                in[2], 
                out[2]; 
            display: "p=244,272;i=bwgen;b=34,34";
        client: UDPNode[numOfClients]; 
            gatesizes: 
                in[1], 
                out[1]; 
            display: "b=36,32;p=122,122;i=comp";
        ethernetHub: Hub; 
            gatesizes: 
                in[numOfClients+1], 
                out[numOfClients+1]; 
            display: "b=32,32;p=218,190;i=router";
    connections nocheck: 
                         
        router.out[0] --> ethernetHub.in[0]; 
        router.in[0] <-- ethernetHub.out[0]; 
                                             
        for i=0..numOfClients-1 do 
            client[i].out[0] --> ethernetHub.in[i+1]; 
            client[i].in[0] <-- ethernetHub.out[i+1]; 
        endfor; 

        router.out[1] --> server.in[0]; 
        router.in[1] <-- server.out[0]; 
                                        
    display: "p=10,10;b=553,432";
endmodule 

network ethernetwork : Ethernetwork 
endnetwork 

