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


//
// FIXME obsolete comment -- probably none of it is true any more --AV
//
// Receive IP datagram for local delivery
// - strip off IP header
// - buffer fragments for ip_fragmenttime
// - wait until all fragments of one fragment number are received
// - discard without notification if not all fragments arrive in
//      ip_fragmenttime
// - Defragment once all fragments have arrived
// - send Transport packet up to the transport layer
// - send ICMP packet to ICMP module
// - send IGMP group management packet to Multicast module
// - send tunneled IP datagram to PreRouting
//
simple IPv6LocalDeliver
    parameters:
        procDelay,
        numOfTransportProt : numeric const,
        fragmentTimeout : string;
    gates:
        in: routingIn;
        in: multicastIn;
        out: mobilityOut;
        out: multicastOut;
        out: ICMPOut;
        out: preRoutingOut;
        out: transportOut[];
        out: tunnelOut;
        out: errorOut;
        out: routingOut; //For routing header
endsimple

