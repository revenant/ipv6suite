//
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

cplusplus 
{{ 
#include "types_typedef.h"
#include "ipv6_addr.h"
}};

enum MobilityOptType
{
  MOPT_Pad1 = 0; // pad1 (NOT IMPLEMENTED YET!)
  MOPT_PadN = 1; // padN (NOT IMPLEMENTED YET!)
  MOPT_BRA = 2; //binding refresh advice
  MOPT_ACoA = 3; // alternate care-of address
  MOPT_NI = 4; // nonce indices
  MOPT_AUTH = 5; // authorization data
};

class MobilityOptionBase
{
  properties:
  omitGetVerb = true;
  //  customize = true;
  fields:
  //extra kind_var data member but virtual fn ensure correct one is retrived
  //using abstract removes data member kind_var but requires to redefine class
   int kind;
   unsigned int byteLength;
}

struct u_int16;
struct ipv6_addr;

class MIPv6OptBRA extends MobilityOptionBase
{
 properties:
  omitGetVerb = true;
 fields:
  unsigned int byteLength = 4;
  int kind enum(MobilityOptType) = MOPT_BRA;
  //ignore requirement alignments which require padding for now

  //units of 4 seconds
  u_int16 interval;
}

class MIPv6OptACoA extends MobilityOptionBase
{
 properties:
  omitGetVerb = true;
 fields:
  unsigned int byteLength = 18;
  int kind enum(MobilityOptType) = MOPT_ACoA;
  ipv6_addr acoa;
}

class MIPv6OptNI extends MobilityOptionBase
{
 properties:
  omitGetVerb = true;
 fields:
  unsigned int byteLength = 6;
  int kind enum(MobilityOptType) = MOPT_NI;
  u_int16 hni;
  u_int16 coni;
}

class MIPv6OptBAD extends MobilityOptionBase
{
 properties:
  omitGetVerb = true;
 fields:
  unsigned int byteLength = 14;
  int kind enum(MobilityOptType) = MOPT_AUTH;
  ipv6_addr auth;
}
