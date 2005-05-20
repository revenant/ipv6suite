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
// Simple model to generate Variable Bit Rate Traffic for testing of the MAC layer.
//
// Typically pixPerFrame adn frameRate are specified as constant values. The variable
// number of bits/pixel is generated internally.
//

simple VBRSrcModel
    parameters:
	destAddr: string,	// destination MAC address
	tStart: numeric const,	// time to start
	fragmentLen: numeric,	// fragment length
	pixPerFrame: numeric,	// frame resolution (pixels/frame)
	frameRate: numeric;	// frame rate (frames/s)
    gates:
        in: brIn;
        out: brOut;
endsimple

