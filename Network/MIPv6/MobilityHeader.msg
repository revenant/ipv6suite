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
#include "types_typedef.h"
#include "MobilityHeaderBase.h"
#include "ipv6_addr.h"
}};

struct u_int64;
struct u_int16;
struct ipv6_addr;


enum MIPv6HeaderType {
  MIPv6MHT_BRR = 0;
  MIPv6MHT_HOTI = 1;
  MIPv6MHT_COTI = 2;
  MIPv6MHT_HOT = 3;
  MIPv6MHT_COT = 4;
  MIPv6MHT_BU = 5;
  MIPv6MHT_BA = 6;
  MIPv6MHT_BE = 7;
};

class MobilityHeaderBase;

message BRR extends MobilityHeaderBase
{
 properties:
  omitGetVerb=true;
 fields:
  int kind enum(MIPv6HeaderType) = MIPv6MHT_BRR;
  //no options allowed  
};

message HOTI extends MobilityHeaderBase
{
 properties:
  omitGetVerb=true;
  customize = true;
 fields:
  int kind enum(MIPv6HeaderType) = MIPv6MHT_HOTI;
  int homeCookie;
  //u_int64 hc;
  //length +8
};

message COTI extends MobilityHeaderBase
{
 properties:
  omitGetVerb=true;
  customize = true;
 fields:
  int kind enum(MIPv6HeaderType) = MIPv6MHT_COTI;
  int careOfCookie;
  //u_int64 coc;
  //length +8
};

message HOT extends MobilityHeaderBase
{
 properties:
  omitGetVerb=true;
  customize = true;
 fields:
  int kind enum(MIPv6HeaderType) = MIPv6MHT_HOT;
  u_int16 hni;
  int homeCookie;
// u_int64 hic;
// u_int64 hkt;
  //length +16
};

message COT extends MobilityHeaderBase
{
 properties:
  omitGetVerb=true;
  customize = true;
 fields:
  int kind enum(MIPv6HeaderType) = MIPv6MHT_COT;
  u_int16 coni;
  int careOfCookie;
//  u_int64 cic;
//  u_int64 cokt;
  //length +16
};

message BU extends MobilityHeaderBase
{
 properties:
  omitGetVerb=true;
  customize = true;
 fields:
  int kind enum(MIPv6HeaderType) = MIPv6MHT_BU;
  bool ack;
  bool homereg;
  bool llac = true;
  //bool kmmc;
  bool mapreg = false;
  u_int16 sequence;
  u_int16 expires;
  // options allowed  
  //auth, ni, acoa
  //length (+4)
};


//
// Enumerations of BA status
//

enum  	BAStatus {
  BAS_ACCEPTED = 0;
  BAS_PREFIX_DISC = 1;
  BAS_REASON_UNSPECIFIED = 128;
  BAS_ADMIN_PROHIBIT = 129;
  BAS_INSUFF_RESOURCE = 130;
  BAS_HR_NOT_SUPPORTED = 131;
 BAS_NOT_HOME_SUBNET = 132;
 BAS_NOT_HA_FOR_MN = 133;
  BAS_DAD_FAILED = 134;
  BAS_SEQ_OUT_OF_WINDOW = 135;
 BAS_UNREC_HONI = 136;
 BAS_UNREC_CONI = 137;
  BAS_UNREC_BOTHNI = 138;
  BAS_REG_TYPE_CHANGE_DIS = 139;
};

// BAS_ROU_DUE_TO_LOW_TRAFFIC = 142, BAS_INVALID_AUTH = 143, BAS_TOO_OLD_HONI = 144,
//  BAS_TOO_OLD_CONI = 145

message BA extends MobilityHeaderBase
{
 properties:
  omitGetVerb=true;
  customize = true;
 fields:
  int kind enum(MIPv6HeaderType) = MIPv6MHT_BA;
  unsigned int status enum(BAStatus) =   BAS_ACCEPTED;
  //bool kmmc;
  u_int16 sequence;
  u_int16 lifetime;
  //options
  //bauth, bra
  //length (+4)
};

message BE extends MobilityHeaderBase
{
 properties:
  omitGetVerb=true;
  customize = true;
 fields:
  int kind enum(MIPv6HeaderType) = MIPv6MHT_BE;
  ipv6_addr hoa;
  //true means uncrecognized Mobility Header and false is unknown binding for hoa dest opt
  bool status = false;
  //length (+16)
  //no options allowed  
};

