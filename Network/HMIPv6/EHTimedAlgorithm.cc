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
 * @file   EHTimedAlgorithm.cc
 * @author Johnny Lai
 * @date   30 Aug 2004
 *
 * @brief  Implementation of EHTimedAlgorithm
 *
 *
 */

//Headers for libcwd debug streams have to be first (remove if not used)
#include "sys.h"
#include "debug.h"

#include <boost/cast.hpp>

#include "EHTimedAlgorithm.h"
#include "IPv6Mobility.h"
#include "NeighbourDiscovery.h"
#include "cTimerMessage.h"
#include "MIPv6MNEntry.h"
#include "EHTimers.h"
#include "MIPv6CDSMobileNode.h"
#include "HMIPv6CDSMobileNode.h"
#include "EHCDSMobileNode.h"
#include "MIPv6MStateMobileNode.h"
namespace EdgeHandover
{

///For non omnetpp csimplemodule derived classes
EHTimedAlgorithm::EHTimedAlgorithm(NeighbourDiscovery* mod):
  EHNDStateHost(mod), interval(10)
{
  //Read xml info on Timed algorithm params

  mob->edgeHandoverCallback()->rescheduleDelay(interval);
}

EHTimedAlgorithm::~EHTimedAlgorithm()
{
}

/**
   We can either try binding exactly every x seconds or every x seconds from
   last receipt of MAP BA.

   Currently doing every x seconds. Add boolean to do other way. May
   not ever bind with HA if we actually visit many MAPs before interval expires
   (in this case will need to distinguish between MAPs?)
 */
void EHTimedAlgorithm::mapAlgorithm()
{
  Dout(dc::eh, mob->nodeName()<<" "<<nd->simTime()<<" May do map algorithm");
  if (!hmipv6cdsMN.isMAPValid() ||
      hmipv6cdsMN.currentMap().addr() == ehcds->boundMapAddr())
  {
    if (!mob->edgeHandoverCallback()->isScheduled())
        mob->edgeHandoverCallback()->rescheduleDelay(interval);
    return; //unless state of binding has changed or is about to expire
  }

  Dout(dc::eh|flush_cf, mob->nodeName()<<" "<<nd->simTime()<<" invoking map algorithm");

  if (!mob->edgeHandoverCallback()->isScheduled())
  {
    //Called when timer expires
    ipv6_addr peerAddr = Loki::Field<0>((boost::polymorphic_downcast<EdgeHandover::EHCallback*>
                                     (mob->edgeHandoverCallback()))->args)->srcAddress();

    MobileIPv6::bu_entry* bue = mipv6cdsMN->findBU(peerAddr);
    if (!bue)
    {
      Dout(dc::warning|flush_cf, mob->nodeName()<<" "<<nd->simTime()<<" bue for "<<peerAddr<<" not found.");
      //Caused by HA not sending us back BA and so we have a really old bound
      //map which may have been removed as we forward from old bmaps for 5
      //seconds only
    }
    else
    {
    MobileIPv6::MIPv6MStateMobileNode::instance()->sendBUToAll(
      hmipv6cdsMN.remoteCareOfAddr(), mipv6cdsMN->homeAddr(), bue->lifetime(), mob);
    Dout(dc::eh, mob->nodeName()<<" "<<nd->simTime()
           <<" mapAlgorithm sent bu to all based on BA from bue-"<<*bue);
    }
    mob->edgeHandoverCallback()->rescheduleDelay(interval);
  }
  else
  {
    //Called directly from processBA
    Dout(dc::eh, mob->nodeName()<<" "<<nd->simTime()<<" delaying binding with HA until "
         <<mob->edgeHandoverCallback()->arrivalTime());

    //Bind every x seconds from map ba
    //mob->edgeHandoverCallback()->rescheduleDelay(interval);
  }
}

void EHTimedAlgorithm::boundMapChanged(const ipv6_addr& old_map)
{

  Dout(dc::eh, mob->nodeName()<<" "<<nd->simTime()<<" rescheduling for period="<<interval
       <<" as bound map changed to "<<ehcds->boundMapAddr()<<" from "<<old_map);
  mob->edgeHandoverCallback()->rescheduleDelay(interval);
}


}; //namespace EdgeHandover
