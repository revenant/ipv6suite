// -*- C++ -*-
// Copyright (C) 2006 Johnny Lai
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
cplusplus 
{{ 
// abit round about but ...
#include "RTPPacket.h"
}};

//report block 6 * 4 bytes
struct RTCPReportBlock 
{
 fields:
  unsigned int ssrc; //32 bits
  short fractionLost; //1 byte
  unsigned int cumPacketsLost; //3 bytes
  unsigned int seqNoReceived; //4
  //supposed to be 4 bytes but we'll use float for easier calc
  simtime_t jitter; //4
  unsigned int lastSR; //4
  unsigned int delaySinceLastSR; //4
}

class RTCPPacket;

message RTCPReports extends RTCPPacket
{
 properties:
  customize = true;
  omitGetVerb=true;

 fields:
  //only 1 report block as we do not handle conferences yet
  abstract RTCPReportBlock reportBlocks[]; 
}

message RTCPGoodBye extends RTCPPacket
{
 properties:
  customize = true;
  //optional reason so add 4 bytes
}

//Actually SDES with CNAME chunk only to be exact
message RTCPSDES extends RTCPPacket
{
 properties:
  customize = true;
  omitGetVerb=true;

 fields:
  // 2 bytes + length of string (mod 4)
  //chunk (only cname right now)
  abstract string cname;
}
