//
// Copyright (C) 2004 Eric Wu
// Copyright (C) 2004 Andras Varga
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
// Configures \IPv6 addresses and routing tables for a "flat" network,
// "flat" meaning that all hosts and routers will have the same
// network address and will only differ in the host part.
//
simple DynamicBRLoader
    parameters:
        numNodes: numeric,
	srcPrefix: string,
	destPrefix: string,
	rangeMinX: numeric,
	rangeMinY: numeric,
	rangeMaxX: numeric,
	rangeMaxY: numeric;
endsimple

simple DynamicIPv6CBRLoader
    parameters:
        numNodes: numeric,
	srcPrefix: string,
	destPrefix: string,
	rangeMinX: numeric,
	rangeMinY: numeric,
	rangeMaxX: numeric,
	rangeMaxY: numeric,

	msgType: numeric,	// msg type
	tStart: numeric const,	// time to start
	bitRate: numeric,	// bit rate to achieve (bits/s)
	fragmentLen: numeric;	// fragment length (bytes)
endsimple
