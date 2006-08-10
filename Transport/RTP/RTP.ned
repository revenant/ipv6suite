// -*- ned -*-
// Copyright (C) 2006 
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

simple RTP
  parameters:
    //assume dest uses these ports and we ourselves also use these ports
    port: numeric const, //if odd, base of pair is 1 less RTCP port is base+1
    sessionBandwidth: numeric const,
    startTime: numeric const,
    destAddrs: string; // list of \IP addresses, separated by spaces
    gates:
        in: from_udp;
        out: to_udp;
endsimple
