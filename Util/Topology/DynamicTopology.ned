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
    "DynamicTopologyBuilder",
    "WorldProcessor",
    "Router6",
    "TCPNode",
    "UDPNode";

//# FIXME duplicate - make them a separare NED file?
channel internetCable
	delay 0.012
	datarate 10e9
endchannel

//# FIXME duplicate - make them a separare NED file?
channel intranetCable
	delay 0.002
	datarate 100e6
endchannel


//
// Network with dynamically built topology
//
module DynamicTopology
       submodules:
	 worldProcessor: WorldProcessor;
         dtb: DynamicTopologyBuilder;
	 connections nocheck:
endmodule

network dynamicTestNet: DynamicTopology
endnetwork
