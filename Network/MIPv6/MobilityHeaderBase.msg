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
  //#include "MobilityOptionBase.h"
  //putting cplusplus at end fails :( [does not output at end of header file]
  //namespace MobileIPv6 {
}};

//class MobilityOptionBase;

//
// Base Mobility Header class
//
message MobilityHeaderBase
{
 properties:
  omitGetVerb=true;
  customize = true;
 fields:
  //overide cMessage::kind 
  //can't use as no inclusion of msg ability so will need two defn of MIPv6HeaderType
  //abstract int kind enum(MIPv6HeaderType);
  //usually 59 i.e. IPPROTO_NONE so ignore for now
  //int payload; (encapsulatedMsg->kind())
  //length is full length of header (inc. first 8 octets)
  //must be multiple of 8 octets (so we do +1 till %8 = 0 )
  //checksum we don't simulate
  //can't handle pointers :(
  //abstract MobilityOptionBase* options[]; 
};

