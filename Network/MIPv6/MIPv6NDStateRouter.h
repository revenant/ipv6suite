// -*- C++ -*-
//
// Copyright (C) 2001 CTIE, Monash University
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
   @file MIPv6NDStateRouter.h

   @brief Definition of MIPv6NDStateRouter class that handles MIPv6 specific
   ICMP messages

   @author Eric Wu
   @date 16.4.02
*/

#ifndef __MIPV6NDSTATEROUTER_H__
#define __MIPV6NDSTATEROUTER_H__

#ifndef NDSTATEROUTER_H__
#include "NDStateRouter.h"
#endif // NDSTATEROUTER_H__

using namespace IPv6NeighbourDiscovery;

namespace MobileIPv6
{

/**
 * @class MIPv6NDStateRouter
 *
 * @brief Router processing of MIPv6 specific ICMPv6 messages
 */

class MIPv6NDStateRouter : public NDStateRouter
{
 public:
  MIPv6NDStateRouter(NeighbourDiscovery* mod);
  virtual ~MIPv6NDStateRouter(void);

  virtual std::auto_ptr<ICMPv6Message> processMessage(
    std::auto_ptr<ICMPv6Message> msg );

  virtual void print(){};

 protected:
  virtual ICMPv6NDMRtrAd* createRA(const IPv6InterfaceData::RouterVariables& rtr, size_t ifidx);
};

} // end namesapce MobileIPv6

#endif // __MIPV6NDSTATEROUTER_H__
