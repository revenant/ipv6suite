// $Header: /home/cvs/IPv6Suite/IPv6SuiteWithINET/Network/IPv6/Attic/MLDMessage.h,v 1.1 2005/02/09 06:15:58 andras Exp $ 
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
 *  @file MLDMessage.h
 *  @brief Manage requests to the routing table and the interface table
 *
 *  @author Chainarong Kit-Opas
 *
 *  @date    28/11/2002
 *
 */
#include "ICMPv6Message.h"
#include "ipv6_addr.h"

class MLDMessage: public ICMPv6Message
{
 public:
  MLDMessage(const ICMPv6Type type);
  MLDMessage(const MLDMessage& src);
  virtual MLDMessage& operator=(const MLDMessage & rhs);
  void setDelay(unsigned int maxDelay);
  void setAddress(ipv6_addr multicast_addr);
  unsigned int showDelay();
  ipv6_addr showAddress();
  
  

virtual MLDMessage *dup()const
{
      return new MLDMessage(*this);
}
  
virtual const char* classname() const 
{
      return "MLDMessage";
}


 private:
  ipv6_addr multicast_addr;
};


  
