// -*- Ned -*-
// $Header: /home/cvs/IPv6Suite/IPv6SuiteWithINET/Examples/IPv6/TestNetwork/Attic/UDPNetwork.ned,v 1.1 2005/02/09 06:15:57 andras Exp $
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
// @file   UDPNetwork.ned
// @author Johnny Lai
// @date   26 May 2004
// 
// @brief Test AbstractUDPNode which has new UDP interface 
// 
import  "WorldProcessor",
	"UDPNode";

module UDPNetwork
    submodules:
        worldProcessor: WorldProcessor;
	    display: "p=100,184;i=bwgen_s;b=17,17";
        client1: UDPNode;
	    parameters:
	    gatesizes:
	        in[1],
		out[1];
        client2: UDPNode;
	    gatesizes:
		in[1],
		out[1];
    connections nocheck:
        client1.out[0] --> client2.in[0];
	client1.in[0] <-- client2.out[0];
endmodule

network udpNet : UDPNetwork
endnetwork
