// $Header: /home/cvs/IPv6Suite/IPv6SuiteWithINET/Network/IPv6/Attic/MLDv2Message.cc,v 1.1 2005/02/09 06:15:58 andras Exp $ 
// Copyright (C) 2001, 2002 CTIE, Monash University 
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

/**
 *  @file MLDv2Message.cc
 *  @brief MLDv2 manage Query and Report Message
 *
 *  @author Wally Chen
 *
 *  @date    28/11/2002
 *
 */

#include "MLDv2Message.h"

MLDv2Message::MLDv2Message(ICMPv6Type type, size_t size):ICMPv6Message(type),  _opt(size)
{
}


MLDv2Message& MLDv2Message::operator=(const MLDv2Message& rhs )
{
  MLDv2Message::operator=(rhs);
      //Identifier and sequence number are set when _opt_info is copied in ICMPv6Message
  return* this;
}

void MLDv2Message::setMaxRspCode(unsigned int maxDelay)
{
  assert(0xffff>=maxDelay);
  setOptInfo((optInfo()&0|(maxDelay<<(16))));
}

unsigned int MLDv2Message::MaxRspCode()
{
  return (optInfo()>>16);
}

void MLDv2Message::setNoMAR(unsigned int num)
{
  assert(0xffff>=num);
  setOptInfo((optInfo()&0|(num)));
}

unsigned int MLDv2Message::NoMAR()
{
  return (optInfo()&0xFFFF);
}

void MLDv2Message::setMAR(char *src, int offset, int size)
{
  memcpy(&_opt[offset],src,size);
}

// Function for General Query
const ipv6_addr MLDv2Message::MA() const
{
  ipv6_addr addr;
  memcpy(&addr.extreme,&_opt[0],sizeof(unsigned int));
  memcpy(&addr.high,&_opt[4],sizeof(unsigned int));
  memcpy(&addr.normal,&_opt[8],sizeof(unsigned int));
  memcpy(&addr.low,&_opt[12],sizeof(unsigned int));
  return addr;
}

const bool MLDv2Message::S() const
{
  if(0x8&_opt[16])
    return true;
  else
    return false;
}
  
const char MLDv2Message::QRV() const
{
  return (0x7&_opt[16]);
}
  
const char MLDv2Message::QQIC() const
{ return _opt[17]; }
  
const short int MLDv2Message::NS() const
{ 
  short int rtNS;
  memcpy(&rtNS,&_opt[18],sizeof(short int));
  return rtNS;
}
  
void MLDv2Message::setMA(ipv6_addr _info)
{
  memcpy(&_opt[0],&_info.extreme,sizeof(unsigned int));
  memcpy(&_opt[4],&_info.high,sizeof(unsigned int));
  memcpy(&_opt[8],&_info.normal,sizeof(unsigned int));
  memcpy(&_opt[12],&_info.low,sizeof(unsigned int));
}

void MLDv2Message::setS_Flag(bool S)
{
  char _MLQF = _opt[16];
  if(S)
    _MLQF = _MLQF | 0x8;
  else
    _MLQF = _MLQF &0xF7;
  _opt[16] = _MLQF;
}

void MLDv2Message::setQRV(char QRVcode)
{
  char _MLQF = _opt[16];
  assert(0x7>=QRVcode);
  _MLQF = _MLQF &0xF8;
  _MLQF += QRVcode;
  _opt[16] = _MLQF;
}

void MLDv2Message::setQQIC(char _info)
{ _opt[17] = _info; }

void MLDv2Message::setNS(short int _info)
{
  memcpy(&_opt[18],&_info,sizeof(short int));
}

void MLDv2Message::setOpt(char* src, int offset, size_t size)
{
  memcpy(&_opt[offset],src,size);
}

void MLDv2Message::getOpt(char* dest, int offset, size_t size)
{
  memcpy(dest,&_opt[offset],size);
}

const ipv6_addr MLDv2Message::getIPv6addr(int offset)
{
  ipv6_addr addr;
  memcpy(&addr.extreme,&_opt[offset],sizeof(unsigned int));
  memcpy(&addr.high,&_opt[offset+4],sizeof(unsigned int));
  memcpy(&addr.normal,&_opt[offset+8],sizeof(unsigned int));
  memcpy(&addr.low,&_opt[offset+12],sizeof(unsigned int));
  return addr;
}

int MLDv2Message::optLength()
{
  return sizeof(_opt.size());
}