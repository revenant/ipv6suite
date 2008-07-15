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


import "DynamicTopologyBuilder";

simple Switch
    gates:
        in: in[];
        out: out[];
endsimple

simple Queue
    gates:
        in: in[];
        out: out[];
endsimple

module ClosedQueueingNetwork
       //error in opp3
       //machines: host0, host1, host2, host3;
       parameters:
         //number of switches
        N : numeric const,
        // number of queues in a tandem
        Q : numeric const,
        // service mean time in units
        S : numeric const,
        // unit time
        T : numeric const,
        // number of jobs that are initially assigned to each switch
        J : numeric const,
        // factor
        factor : numeric const;
       submodules:
       dtb: DynamicTopologyBuilder;
       connections nocheck:
endmodule

network cqn : ClosedQueueingNetwork
	on: host0, host1, host2, host3;
endnetwork

