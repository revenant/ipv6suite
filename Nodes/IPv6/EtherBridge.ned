// Copyright (C) 2001, 2004 CTIE, Monash University
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
//   file: EtherSwitch
//   Purpose: implementation of Ethernet bridge
//   author: Eric Wu

import
    "EtherRelayUnit",
    "LinkLayer6";


module EtherBridge
    parameters:
        entryTimeout : numeric;
    gates:
        out: out[];
        in: in[];
    submodules:
        blackboard: Blackboard;
            display: "p=62,60;i=block/blackboard";
        ethernet: LinkLayer6[sizeof(in)];
            parameters:
                NWIName = "EtherModuleBridge",
                nwi_delay = 0;
            display: "p=136,124;i=card2";
        relayUnit: EtherRelayUnit;
            parameters:
                numOfPorts = sizeof(in),
                entryTimeout = entryTimeout;
            gatesizes:
                ethIn[sizeof(in)],
                ethOut[sizeof(in)];
            display: "p=248,188;i=bwxconn";
    connections nocheck:
        for i=0..sizeof(in)-1 do
            in[i] --> ethernet[i].physIn;
            out[i] <-- ethernet[i].physOut;
        endfor;

        // ds <--> relayUnit
        for i=0..sizeof(in)-1 do
            relayUnit.ethOut[i] --> ethernet[i].netwIn;
            ethernet[i].netwOut --> relayUnit.ethIn[i];
        endfor;
    display: "p=10,10;b=320,298";
endmodule

