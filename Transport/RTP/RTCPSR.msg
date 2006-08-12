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
#include "RTCPPacket.h"
}};

class RTCPReports;

message RTCPRR extends RTCPReports
{
 properties:
  omitGetVerb=true;
  customize = true;
}

message RTCPSR extends RTCPReports
{
 properties:
  omitGetVerb=true;
  customize = true;
 fields:
  //length part of cmessage
  //sender info 5*4 bytes
  //(32 in sim enough as max session length of 18.2 hours and 16us accuracy 
  //NTP timestamp 8 bytes  (part of cmessage)
  //RTP timestamp 4 bytes (supposed to be same time as NTP but in units of RTP time)
  //total no. of rtp packets
  unsigned int packetCount; //4 bytes
  //payload octets not inc. header/padding
  unsigned int octetCount;  //4 bytes  
}
