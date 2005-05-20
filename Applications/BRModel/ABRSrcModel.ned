//
// Copyright (C) 2004 Monash University, Australia
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


//
// Simple model to generate Available Bit Rate Traffic for testing of the MAC layer.
//
// Typically activePackets, packetLength, and pause period should be random. tIdle
// can be used to adjust the network load.
// Packets will be fragmented according to the fragment length before sending to
// lower layer.
//

simple ABRSrcModel
    parameters:
	destAddr: string,	// destination MAC address
	tStart: numeric const,	// time to start
	tIdle: numeric,		// idle time between active periods
	tPause: numeric,	// pause time between packets within an active period
	activePackets: numeric, // number of packets within an active period
	packetLen: numeric,	// packet length (bytes)
	fragmentLen: numeric;	// fragment length (bytes)
    gates:
        in: brIn;
        out: brOut;
endsimple

