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


//
// Simple ping process that generates ping requests and calculates the
// packet loss and round trip parameters.
//
simple Ping6App
	parameters:
		interval,
		packetSize,
		hopLimit,
		count,
		deadline,
		startTime,
//Can't use bool in ini file just keeps complaining about allow type `L' for
//parameter and I've tried true/false T/F
//		printPing: bool, 
		printPing,
		destination: string,
		source: string;
	gates:
		in: pingIn;
		out: pingOut;
endsimple
