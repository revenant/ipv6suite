// -*- C++ -*-
// Copyright (C) 2004 Johnny Lai
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

  const unsigned int MAX_PORT = (1<<16)-1;
  const unsigned int MIN_RANDOM_PORT = 1025;
}};



//
// Enumerations of different UDP types stored into kind
//
enum UDPKind
{
  KIND_BIND = 8;
  KIND_DATA = 9;
};

//
// Base UDP packet class. All UDP application specific messages should inherit
// from this. (to save on the over the wire cost of the message during parall sim)
message UDPPacketBase
{
 fields:
  unsigned short srcPort;
  unsigned short destPort;
  //length is part of cMessage already
  //checksum is not needed anyway;
}

// This is the intra layer representation of UDP messages
message UDPAppInterfacePacket
{
 fields:
  string destIPAddr;
  string srcIPAddr;
};
