// -*- Ned -*-
// Copyright (C) 2008 Johnny Lai
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


//
// @file   RTPVoip.ned
// @author Johnny Lai
// @date   15 Apr 2008
// 
// @brief  Voip Application that uses RTP as base class
//  as no inheritance of ned params they are repeated here
// 


simple RTPVoip
  parameters:
    //assume dest uses these ports and we ourselves also use these ports
    port: numeric const, //if odd, base of pair is 1 less RTCP port is base+1
    //factor used to calculate said value based on sessionBandwidth or actual	
    // value when > 1 in bytes

    rtcpBandwidth: numeric const, 
    //very basic voip app settings
    destAddrs: string, // list of \IP addresses, separated by spaces
    startTime: numeric const,
    stopTime: numeric const,
    frameLength: numeric const, //in units of seconds
    framesPerPacket: numeric const, //number of frames to put into packet
    bitrate: numeric const, //in bitsps

    //e-model values
    lookahead: numeric const, //in seconds
    Ie: numeric const,
    Bpl: numeric const,
   
    //dummy variables from RTP unused as calculated from above
    payloadLength: numeric const,
    packetisationInterval: numeric const;
    gates:
        in: from_udp;
        out: to_udp;
endsimple
