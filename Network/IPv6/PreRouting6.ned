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
// file: PreRouting6
// Compound Module: PreRouting6
// Responsibilities:
// receive IP datagram
// check for header error -> throw away and notify ICMP Module on error
// hop counter decrement
// send correct datagram to Routing Module
// author: Jochen Reber
// date: 2.5.00, 9.5.00, 20.5.00, 9.6.00
// -------------------------------------------------

import "NF_IP_PRE_ROUTING";

simple PreRouting6Core
    parameters:
        procdelay : numeric const;
    gates:
        in: inputQueueIn;
        in: localDeliverIn;
    in: tunnelExit;
        out: errorOut;
        out: routingOut;
endsimple

module PreRouting6
    parameters:
        procdelay : numeric const;
    gates:
        in: queueIn;
        in: localDeliverIn;
    in: tunnelExit;
        out: routingOut;
        out: errorOut;
    submodules:
        core: PreRouting6Core;
            parameters:
                procdelay = procdelay;
            display: "p=175,192;b=40,24,rect";
    connections nocheck:
        queueIn --> core.inputQueueIn;
        localDeliverIn --> core.localDeliverIn;
    tunnelExit --> core.tunnelExit;
        core.routingOut --> routingOut;
        core.errorOut --> errorOut;

endmodule

