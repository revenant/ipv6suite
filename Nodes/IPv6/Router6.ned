//
// Copyright (C) 2002, 2004 Eric Wu
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
// Definition of an IPNode with transport layers

import
    "Blackboard",
    "InterfaceTable",
    "NetworkLayer6",
    "LinkLayer6",
    "ErrorHandling6",
    "PhysicalLayer";


//
// IPv6 router with the number of ports as parameter.
//
module Router6
    gates:
        in: in[];
        out: out[];

    submodules:
        blackboard: Blackboard;
            display: "p=84,59;i=block/blackboard";
        interfaceTable: InterfaceTable;
            display: "p=83,124;i=block/table";
        errorHandling: ErrorHandling6;
            display: "p=244,115;i=block/sink_vs";
        networkLayer: NetworkLayer6;
            parameters:
                IPForward = true,
                numOfPorts = sizeof(in);
            gatesizes:
                physIn[sizeof(in)],
                physOut[sizeof(in)];
            display: "p=184,133;i=block/network2";
        linkLayers: LinkLayer6[sizeof(in)];
            parameters:
                nwi_delay = 0;
            display: "p=130,212,r;i=block/ifcard";
        phyLayer: PHYLayer;
            gatesizes:
                in[sizeof(in)],
                out[sizeof(in)],
                linkIn[sizeof(in)],
                linkOut[sizeof(in)];
            display: "p=180,276;b=180,8;o=#4359d7,#4159d7";
    connections nocheck:
        networkLayer.errorOut --> errorHandling.in;

        // Link Layer <-> Network Layer
        for i=0..sizeof(in)-1 do
            networkLayer.physOut[i] --> linkLayers[i].netwIn;
            linkLayers[i].netwOut --> networkLayer.physIn[i];
        endfor;

        // connections to network outside
        for i=0..sizeof(in)-1 do
            in[i] --> phyLayer.in[i];
            out[i] <-- phyLayer.out[i];
        endfor;

        // Link Layer <-> Physical Layer
        for i=0..sizeof(in)-1 do
            phyLayer.linkOut[i] --> linkLayers[i].physIn;
            phyLayer.linkIn[i] <-- linkLayers[i].physOut;
        endfor;
endmodule

