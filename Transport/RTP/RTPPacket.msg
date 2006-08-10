// -*- C++ -*-
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

//Include any c++ code in these sections


//
// Simple RTP packet
//
message RTPPacket
{
 properties:
  customize = true;
  omitGetVerb=true;

 fields:
  //size is 12 bytes assuming 0 csrc identifiers
  unsigned int seqNo; //16 bits
  //timestamp 32 bits (part of cmessage already)
  //identifies the source of rtp stream (use module id? or some other method to
  //map nodename to ssrc)
  unsigned int ssrc; //32 bits 
  unsigned int payloadLength; //in bytes
};


// Simple RTCP packet
message RTCPPacket
{
 properties:
  customize = true;
  omitGetVerb=true;

 fields:
  //header 8 bytes
  unsigned int ssrc; //32 bits
}
