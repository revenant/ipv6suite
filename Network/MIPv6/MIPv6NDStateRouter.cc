// -*- C++ -*-
//
// Copyright (C) 2001 CTIE, Monash University
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
   @file MIPv6NDStateRouter.cc
   Porpose: Definition of MIPv6NDStateRouter class that implements handles MIPv6 specific ICMP messages

   Author: Eric Wu
   Date: 16.4.02
*/

#include <iomanip> //setprecision
#include <memory> //auto_ptr

#include "MIPv6NDStateRouter.h"
#include "NDTimers.h"
#include "NeighbourDiscovery.h"
//#include "ICMPv6Message.h"
#include "MIPv6ICMPv6NDMessage.h"
#include "RoutingTable6.h"
#include "IPv6Datagram.h"
#include "ipv6_addr.h"
#include "cTimerMessage.h"
#include "IPv6CDS.h"

namespace MobileIPv6
{

MIPv6NDStateRouter::MIPv6NDStateRouter(NeighbourDiscovery* mod)
  : NDStateRouter(mod)
{}

MIPv6NDStateRouter::~MIPv6NDStateRouter(void)
{}

std::auto_ptr<ICMPv6Message>  MIPv6NDStateRouter::processMessage(std::auto_ptr<ICMPv6Message> msg )
{
  return NDStateRouter::processMessage(msg);

  //TODO handle DHAAD messages if that's msg is
}

ICMPv6NDMRtrAd* MIPv6NDStateRouter
::createRA(const Interface6Entry::RouterVariables& rtrVar, size_t ifidx)
{
  MIPv6ICMPv6NDMRtrAd* rtrAd = new MIPv6ICMPv6NDMRtrAd(static_cast<unsigned int>(rtrVar.advDefaultLifetime),
                                                       rtrVar.advCurHopLimit,
                                                       rtrVar.advReachableTime,
                                                       rtrVar.advRetransTmr,
                                                       rtrVar.advPrefixList,
                                                       rtrVar.advManaged,
                                                       rtrVar.advOther
#ifdef USE_MOBILITY
                                                       ,rtrVar.advHomeAgent
#endif
                                                       );

#ifdef USE_MOBILITY
  // New Advertisement Interval option included in RA
  rtrAd->setAdvInterval(
    static_cast<unsigned long>(rtrVar.maxRtrAdvInt * 1000));
#endif

  return rtrAd;
}

} // end namesapce MobileIPv6

