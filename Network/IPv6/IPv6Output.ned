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



import "NF_IP_POST_ROUTING";


//
// IPv6Output core module, with its main task being TTL check.
//
// Responsibilities:
//  - receive complete datagram from IPFragmentation
//  - check hop counter (ttl) and throw away packet and notify ICMP if ttl==0
//  - otherwise  send it on to output queue
// FIXME doc out of date -- none of it is true any more!!! --AV
//
// @author Johnny Lai
// based on IPOutputCore by Jochen Reber
//
simple IPv6Output
    parameters:
        procDelay: numeric const;
    gates:
        in: fragmentationIn;
        in: neighbourDiscoveryIn;
        in: addrReslnIn;
        in: mobilityIn;
        //For unicast packets from ND module
        in: neighbourDiscoveryDirectIn;
        out: queueOut;
        out: errorOut;
endsimple


