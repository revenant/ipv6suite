// -*- C++ -*-
// Copyright (C) 2004 Johnny Lai
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


/**
 * @file   EHNDStateHost.cc
 * @author Johnny Lai
 * @date   25 Aug 2004
 *
 * @brief  Implementation of EHNDStateHost
 *
 * @todo We really need to get rid of the promiscous dependencies of base-> derived class
 *
 */

//Headers for libcwd debug streams have to be first (remove if not used)
#include "sys.h"
#include "debug.h"

#include <boost/cast.hpp>

#include "cTTimerMessageCB.h"
#include "EHNDStateHost.h"
#include "MIPv6MNEntry.h"
#include "HMIPv6CDSMobileNode.h"
#include "EHCDSMobileNode.h"
#include "EHTimers.h"
#include "EHTimedAlgorithm.h"
#include "opp_utils.h" //OPP_Global::ContextSwitcher
//for tests
#include "NeighbourDiscovery.h"
#include "RoutingTable6.h"
#include "IPv6Mobility.h"
#include "HMIPv6ICMPv6NDMessage.h"

namespace EdgeHandover
{

  EHNDStateHost* EHNDStateHost::create(NeighbourDiscovery* mod, const std::string& handoverType)
  {
    return new EHTimedAlgorithm(mod);
  }

  ///Timer belongs to NeighbourDiscovery module and should be unique to the constants in NeighbourDiscovery.cc
  //@{
  const int Tmr_EHCallback = 9000;
  const int Tmr_EHBMapChangedCB = 9001;
  //@}

  EHNDStateHost::EHNDStateHost(NeighbourDiscovery* mod):
    HMIPv6NDStateHost(mod),
    ehcds(boost::polymorphic_downcast<EHCDSMobileNode*>(&hmipv6cdsMN))
{

  cTimerMessage* tmr = new EHCallback(Tmr_EHCallback, (cSimpleModule*)nd, this,
                   &EdgeHandover::EHNDStateHost::invokeMapAlgorithmCallback,
                                      "EHInvokeMapAlgorithmCallback");

  ///Create timer message with our callback (timer used if we want timed
  ///notification otherwise used as a callback pointer)
  mob->setEdgeHandoverCallback(tmr);
 
    ///Bloody ridiculous spent over 2 hours doing diff things to figure out why
    ///this did not work. Turned out NeighbourDiscovery was not an implicit
    ///cSimpleModule when it should be(so much for type safety and this happened
    ///on every bloody compiler gcc34/icc)!!!!

//   new EHCallback(Tmr_EHCallback, (cSimpleModule*)nd, this,
//                  &EdgeHandover::EHNDStateHost::invokeMapAlgorithmCallback,
//                  "EHInvokeMapAlgorithmCallback"));

  ehcds->bcoaChangedNotifier = 
    new BoundMapChangedCB(Tmr_EHBMapChangedCB, (cSimpleModule*)nd, this,
                          &EdgeHandover::EHNDStateHost::invokeBoundMapChangedCallback,
                          "EHInvokedBoundMapChangedCallback");
}

EHNDStateHost::~EHNDStateHost()
{
  if (mob->edgeHandoverCallback() && mob->edgeHandoverCallback()->isScheduled())
    mob->edgeHandoverCallback()->cancel();
  delete mob->edgeHandoverCallback();
  delete ehcds->bcoaChangedNotifier;
}

/**
   @brief callback function invoked by mob->edgeHandoverCallback()->callFunc()

   Needed so subclasses can override the virtual mapAlgorithm and be invoked
   from here as only invokeMapAlgorithm is bound at compile time.

*/
void EHNDStateHost::invokeMapAlgorithmCallback(IPv6Datagram* dgram)
{
  this->mapAlgorithm();
}

/**
   @brief callback function invoked when bmap has changed

   Descendants can be notified of when the bound Map has changed by overriding
   boundMapChanged. This accessory function is needed since
   invokeBoundMapChangedCallback is bound at compile time.

   Additionally since this class has formRemoteCOA it will set the bmap/bcoa
   directly in EHCDSMobileNode (data member ehcds).  The previous bmap is passed
   into boundMapChanged.

 */
ipv6_addr EHNDStateHost::invokeBoundMapChangedCallback(HierarchicalMIPv6::HMIPv6MAPEntry map,
                                                  unsigned int ifIndex)
{
  ehcds->bcoa = formRemoteCOA(map, ifIndex);
  ipv6_addr old_map = ehcds->boundMapAddr();
  ehcds->bmap = map.addr();
  OPP_Global::ContextSwitcher s(nd);
  boundMapChanged(old_map);
  return map.addr();
}

typedef HierarchicalMIPv6::MAPOptions MAPOptions;
typedef HierarchicalMIPv6::HMIPv6MAPEntry HMIPv6MAPEntry;

static const  HMIPv6MAPEntry invalidMAP;

HMIPv6MAPEntry EHNDStateHost::selectMAP(MAPOptions &maps, MAPOptions::iterator& new_end)
{
  HMIPv6NDStateHost::selectMAP(maps, new_end);

  std::partial_sort(maps.begin(), maps.begin() + 1, new_end,
                    std::greater<HMIPv6MAPEntry>());
  if (maps.begin() == new_end)
    return invalidMAP;
  return *maps.begin();
}

void EHNDStateHost::boundMapChanged(const ipv6_addr& old_map)
{
  Dout(dc::eh, rt->nodeName()<<" "<<nd->simTime()<<" bound map was "<< old_map
       <<" now is "<<ehcds->boundMapAddr());
}

};
