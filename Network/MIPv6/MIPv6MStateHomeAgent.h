// -*- C++ -*-
// $Header: /home/cvs/IPv6Suite/IPv6SuiteWithINET/Network/MIPv6/MIPv6MStateHomeAgent.h,v 1.1 2005/02/09 06:15:58 andras Exp $
// Copyright (C) 2002, 2003 CTIE, Monash University 
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
 * @file MIPv6MStateHomeAgent.h
 * @author Eric Wu
 * @date 16.4.2002

 * @brief Implements functionality of Home Agent
 *
 */

#ifndef __MIPV6MSTATEHOMEAGENT_H__
#define __MIPV6MSTATEHOMEAGENT_H__

#include <omnetpp.h>

#ifndef __MIPV6MOBILTIYSTATE_H__
#include "MIPv6MobilityState.h"
#endif // __MIPV6MOBILTIYSTATE_H__

class IPv6Mobility;
class IPv6Datagram;
class MIPv6MobilityHeaderBase;

namespace MobileIPv6
{

class MIPv6MStateHomeAgent : public MIPv6MobilityState
{ 
 public:
  static MIPv6MobilityState* instance(void);

  virtual ~MIPv6MStateHomeAgent(void);

  virtual void processMobilityMsg(IPv6Datagram* dgram,
                                  MIPv6MobilityHeaderBase*& mhb,
                                  IPv6Mobility* mod);

 protected:
  virtual bool processBU(IPv6Datagram* dgram, BU* bu, IPv6Mobility* mod);

  virtual void registerBCE(IPv6Datagram* dgram, BU* bu, IPv6Mobility* mob);

  virtual bool deregisterBCE(BU* bu, unsigned int ifIndex, IPv6Mobility* mob);

  static MIPv6MStateHomeAgent* _instance;  

  ///Returns global address on certain interface used as HA's address
  ipv6_addr globalAddr(unsigned int ifIndex,  IPv6Mobility* mod) const;
  
  MIPv6MStateHomeAgent(void);
};
  
} // end namespace MobileIPv6

#endif // __MIPV6MSTATEHOMEAGENT_H__
