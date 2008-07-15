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
// Simple model to generate Constant Bit Rate Traffic for testing of the MAC layer.
//
// The fragment length will dictate the pace at which the bits will be spread over
// the second.
//
simple CBRSrcModel
    parameters:
	msgType: numeric,	// msg type
	destAddr: string,	// destination MAC address
	tStart: numeric const,	// time to start
	bitRate: numeric,	// bit rate to achieve (bits/s)
	fragmentLen: numeric;	// fragment length (bytes)
    gates:
        out: brOut;
endsimple

simple IPv6CBRSrcModel
    parameters:
	msgType: numeric,	// msg type
	destAddr: string,	// destination IPv6 address
	tStart: numeric const,	// time to start
	bitRate: numeric,	// bit rate to achieve (bits/s)
	fragmentLen: numeric;	// fragment length (bytes)
    gates:
        out: brOut;
endsimple

