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
    "Blackboard",
    "Repeater";
    
//
// Wire with 1us delay, which corresponds to 0.2 meters (at 200,000 m/s)
//
channel wire
    delay 1us;
endchannel


//
// Ethernet hub model.
//
// Repeats frames on all ports except arrival port. Can be used with 
// any message type (nothing Ethernet specific).
//
simple Repeater
    gates:
        in: from_net[];
        out: to_net[];
endsimple


//
// Ethernet hub model.
//
// In its current form it's just a wrapper around Repeater.
//
module Hub
    parameters:
    gates:
        in: in[];
        out: out[];
    submodules:
        blackboard: Blackboard;
            display: "p=62,60;i=block/blackboard";
        repeater: Repeater;
            gatesizes:
                from_net[sizeof(in)],
                to_net[sizeof(in)];
            display: "p=174,128;b=40,24";
    connections:
        for i=0..sizeof(in) -1 do
            in[i] --> wire --> repeater.from_net[i];
            repeater.to_net[i] --> wire --> out[i];
        endfor;
endmodule

