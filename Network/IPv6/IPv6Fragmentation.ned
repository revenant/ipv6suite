//
// Copyright (C) 2000 Institut fuer Telematik, Universitaet Karlsruhe
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.


//
// Performs IPv6 Fragmentation.
//
// Responsibilities:
//  - receive valid IP datagram from Routing or Multicast
//  - fragment datagram if size > MTU[output port]
//  - send fragments to IPOutput[output port]
//
// author: Jochen Reber
// date: 2.5.00, 9.5.00, 20.5.00, 9.6.00
//
simple IPv6Fragmentation
    parameters:
        procDelay,
        numOfPorts: const;
    gates:
        in: routingIn, multicastIn, addrReslnIn;
        out: errorOut, outputOut [];
endsimple

