//
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
// A single unified UDP application interface 
// 
// Replaces the previous UDPAppServer and AppClient because 
//
// 1. distinction should only be in c++ class not as separate ned interfaces
// causing duplicated code UDP6.h.
//
// 2. specific application configuration was creeping into the interface e.g. the
// videoserver app settings minWaitInt/maxWaitInt.  
//
// 3. UDP apps can be both clients and servers.
//
// @author Johnny Lai
// @date   25 Jul 2003
//
simple UDPApplication
    parameters:
	//This may not always be used 
//        IPversion: numeric,

	//address of peer or own address to listen on for multihomed hosts?
	UDPServerAddress: string, 
	UDPPort: numeric, //either port to listen on or peer's port based on server
	server: bool, //determines usage of UDPPort

//UDPVideoStream
   	startTime,

// UDPVideoStreamSvr
	minWaitInt: numeric,
	maxWaitInt: numeric,
	minPacketLen: numeric,
	maxPacketLen: numeric,
	videoSize: numeric,
	speed: numeric;	   


    gates:
        in: physIn; 
        out: physOut; 
endsimple



//
// Currently a wrapper around UDPApplication
//
module UDPAppLayer
        parameters:
	    UDPAppName: string;
        gates:
	   in: physIn;
	   out: physOut;
        submodules:
	   udpApp: UDPAppName like UDPApplication
	   parameters:
	   gatesizes:
		physIn[1],
		physOut[1];
	connections nocheck:
	    udpApp.physOut --> physOut;
	    udpApp.physIn <-- physIn;
endmodule	   
