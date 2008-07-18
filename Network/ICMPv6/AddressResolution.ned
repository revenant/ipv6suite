//
// Copyright (C) 2001 Monash University, Australia
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


//
// Perform address resolution, store datagrams pending address resolution
//#FIXME more detailed?
//
simple AddressResolution
    parameters:
        numOfPorts : numeric;
    gates:
        in: pendingQueueIn;  // pending datagrams awaiting addr res come in here
        in: ICMPIn;  // NA responses from addr res NS arrive here using direct sending
        out: fragmentationOut;  // datagrams come out here with ll addr and outputPort
        out: outputOut[];  // NS come out here to Output
        out: ICMPOut;  // ICMP error messages go to ICMP
endsimple
