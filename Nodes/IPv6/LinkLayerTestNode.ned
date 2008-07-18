// -*- Ned -*-
//
// Copyright (C) 2002, 2004 Johnny Lai
//
// This file is part of IPv6Suite
//
// IPv6Suite is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// IPv6Suite is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

import
    "Blackboard",
    "InterfaceTable",
    "PhysicalLayer",
    "LinkLayer6",
    "BRSrcModel",
    "BRSink";

//
// Node to test link layer behaviour
//
// @author Johnny Lai
//
module LinkLayerTestNode
    parameters:
        brModelType: string;
    gates:
        in: in;
        out: out;
    submodules:
        blackboard: Blackboard;
            display: "p=62,60;i=block/blackboard";
        interfaceTable: InterfaceTable;
            display: "p=60,150;i=block/table";
        brSrcModel: brModelType like BRSrcModel;
            display: "p=219,89;b=40,24";
        brSink: BRSink;
            display: "p=219,89;b=40,24";
        linkLayer: LinkLayer6;
            parameters:
                nwi_delay = 0;
            display: "p=121,258;i=block/ifcard";
        phyLayer: PHYLayer;
            gatesizes:
                in[1],
                out[1],
                linkIn[1],
                linkOut[1];
            display: "p=125,346;b=160,8;o=#4359d7,#4159d7";
    connections nocheck:

        // connections to network outside
        in --> phyLayer.in[0];
        out <-- phyLayer.out[0] display "m=,0,80";

        phyLayer.linkOut[0] --> linkLayer.physIn;
        phyLayer.linkIn[0] <-- linkLayer.physOut;

        brSrcModel.brOut --> linkLayer.netwIn;
        linkLayer.netwOut --> brSink.brIn;

    display: "b=216,358";
endmodule

