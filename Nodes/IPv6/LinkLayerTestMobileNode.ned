//
// Copyright (C) 2001, 2004 Monash University, Australia
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
    "InterfaceTable",	
    "MobilityManager",
    "PhysicalLayer",
    "LinkLayer6",
    "BRSrcModel",
    "BRSink";

//
// Node To test wireless link layer behaviour
//
module LinkLayerTestMobileNode
    parameters:
        brModelType: string,
	beginCollectionTime: numeric,
        endCollectionTime: numeric;
    gates:
        in: wlin;
        out: wlout;
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
            gatesizes:
                extSignalIn[1],
                extSignalOut[1];
            display: "p=220,169;i=block/ifcard";
        phyLayer: PHYLayer;
            gatesizes:
                in[1],
                out[1],
                linkIn[1],
                linkOut[1];
            display: "p=216,249;b=168,8;o=#4359d7,#4159d7";
        mobilityManager: MobilityManager;
            display: "i=block/cogwheel;p=352,169";
    connections nocheck:

        brSrcModel.brOut --> linkLayer.netwIn;
        linkLayer.netwOut --> brSink.brIn;

        phyLayer.linkOut[0] --> linkLayer.physIn;
        wlin --> phyLayer.in[0];
    display: "b=417,277";
endmodule

