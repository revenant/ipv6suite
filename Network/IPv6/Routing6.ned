//
// Copyright (C) 2000 Institut fuer Telematik, Universitaet Karlsruhe
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

// -------------------------------------------------
// file: Routing6.ned
// Compound Module: Routing6
// Simple Module: RoutingAlgorithm
// Simple Module: RoutingProtocol
// ------
// Responsibilities:
// Routing Algorithms and Protocols running on a particular interface.
// author: Johnny Lai
// -------------------------------------------------


simple RoutingAlgorithm
       parameters:
endsimple

///Distributes routing topology per interface
simple RoutingProtocol
       parameters:
endsimple

module RAArray
    parameters:
    RoutingAlgorithmType: string;
    submodules:
        //Forwarding of packets
        algorithm: RoutingAlgorithmType like RoutingAlgorithm;
        parameters:
            display: "p=131,186;b=40,24,rect";
    connections nocheck:
endmodule

module RPArray
    parameters:
    RoutingProtocolType: string;
    submodules:
    protocol: RoutingProtocolType like RoutingProtocol;
        parameters:
            display: "p=133,102;b=40,24,rect";
    connections nocheck:
endmodule

module Routing6
    parameters:
    RACount: numeric,
    RPCount: numeric;
    gates:

    submodules:
        ra: RAArray[RACount];
    rp: RPArray[RPCount];
    connections:

    display: "p=10,10;b=260,235,rect";
endmodule
