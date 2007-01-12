// -*- C++ -*-
//
// Copyright (C) 2002, 2003 CTIE, Monash University
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
 * @file MIPv6MStateHomeAgent.h
 * @author Eric Wu
 * @date 16.4.2002

 * @brief Implements functionality of Home Agent
 *
 */

#ifndef __MIPV6MSTATEHOMEAGENT_H__
#define __MIPV6MSTATEHOMEAGENT_H__

#ifndef __MIPV6MSTATECORRESPONDENTNODE_H
#include "MIPv6MStateCorrespondentNode.h"
#endif // __MIPV6MSTATECORRESPONDENTNODE_H

class IPv6Mobility;
class IPv6Datagram;
class MIPv6MobilityHeaderBase;
class IPv6Encapsulation;

namespace MobileIPv6
{

/**
 * @class MIPv6MStateHomeAgent
 * @brief Handling of mobility messages from HA perspective
 * @ingroup MobilityRoles
 */

class MIPv6MStateHomeAgent : public MIPv6MStateCorrespondentNode
{
 public:
  MIPv6MStateHomeAgent(IPv6Mobility* mob);
  virtual ~MIPv6MStateHomeAgent(void);

  virtual bool processMobilityMsg(IPv6Datagram* dgram);

 protected:
  virtual bool processBU(BU* bu, IPv6Datagram* dgram);

  virtual void registerBCE(BU* bu, const ipv6_addr& hoa, IPv6Datagram* dgram);

  virtual bool deregisterBCE(BU* bu, const ipv6_addr& hoa, unsigned int ifIndex);


  ///Returns global address on certain interface used as HA's address
  ipv6_addr globalAddr(unsigned int ifIndex) const;
private:
  MIPv6MStateHomeAgent(void);
  IPv6Encapsulation* tunMod;
};

} // end namespace MobileIPv6

#endif // __MIPV6MSTATEHOMEAGENT_H__
