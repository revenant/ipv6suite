//
// Copyright (C) 2001, 2002 CTIE, Monash University
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

/**
 *  @file MLDMessage.cc
 *  @brief Manage requests to the routing table and the interface table
 *
 *  @author Chainarong Kit-Opas
 *
 *  @date    28/11/2002
 *
 */

#include <assert.h>
#include "MLDMessage.h"

MLDMessage::MLDMessage(ICMPv6Type type)
  :ICMPv6Message(),multicast_addr(IPv6_ADDR_UNSPECIFIED)
{
  setType(type);
}
MLDMessage::MLDMessage(const MLDMessage& src)
  :ICMPv6Message(src)
{
  operator=(src);
}

MLDMessage& MLDMessage::operator=(const MLDMessage& rhs )
{
  ICMPv6Message::operator=(rhs);
  multicast_addr = rhs.multicast_addr;
      //Identifier and sequence number are set when _opt_info is copied in ICMPv6Message
  return* this;
}

void MLDMessage::setDelay(unsigned int maxDelay)
{
  assert(0xffff>=maxDelay);
  setOptInfo((optInfo()&0|(maxDelay<<(16))));
}

void MLDMessage::setAddress(ipv6_addr mul_addr)
{
  multicast_addr = mul_addr;
}

unsigned int MLDMessage::showDelay()
{
  return (optInfo()>>16);
}

ipv6_addr MLDMessage::showAddress()
{
  return multicast_addr;
}




