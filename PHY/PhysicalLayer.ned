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
// Prototype module for a physical layer, used in PHYLayer. 
// All physical layer implementations must have the same gates and 
// (at least) these parameters.
//
simple PhysicalLayer
    parameters:
        PHYName : string;
    gates:
        in: in[];
        out: out[];
        in: linkIn[];
        out: linkOut[];
endsimple


//
// Placeholder for the physical layer, with the concrete interface type
// left as parameter. Represents several (a configurable number of) interfaces.
//
// The actual interface type is a string parameter (works via "<tt>like</tt>");
// possible values are PHYWiredModule and PHYWirelessModule (both are obsolete 
// and have evolved into PHYSimple). (These modules don't have NED declarations, 
// only the C++ implementations.)
//
module PHYLayer
    parameters:
        PHYName : string;
    gates:
        in: in[];
        out: out[];
        in: linkIn[];
        out: linkOut[];
    submodules:
        physicalLayer: PHYName like PhysicalLayer;
            parameters:
                PHYName = PHYName;
            gatesizes:
                in[sizeof(in)],
                out[sizeof(out)],
                linkIn[sizeof(linkIn)],
                linkOut[sizeof(linkOut)];
            display: "p=196,200;b=216,48";
    connections nocheck:
        for i=0..sizeof(in)-1 do
            in[i] --> physicalLayer.in[i];
            out[i] <-- physicalLayer.out[i];

            linkOut[i] <-- physicalLayer.linkOut[i];
            linkIn[i] --> physicalLayer.linkIn[i];
        endfor;
endmodule

