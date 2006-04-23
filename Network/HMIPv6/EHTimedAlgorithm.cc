// -*- C++ -*-
// Copyright (C) 2004 Johnny Lai
//
// This file is part of IPv6Suite
//
// IPv6Suite is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// IPv6Suite is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this program; if not, write to the Free Software
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
#include "RoutingTable6.h"

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
    //Possible that while we have set a valid MAP we have not actually received a BA from it yet 
    //so callback args are not set. This happens due to lossy property of wireless env.
    if (!((IPv6Datagram*)(mob->edgeHandoverCallback()->contextPointer())))
    {
      Dout(dc::eh, " Unable to do anything as BA not received from Map yet");
      mob->edgeHandoverCallback()->rescheduleDelay(interval);
      return;
    }

    //Called when timer expires
    ipv6_addr peerAddr = ((IPv6Datagram*)(mob->edgeHandoverCallback()->contextPointer()))->srcAddress();
    delete ((IPv6Datagram*)(mob->edgeHandoverCallback()->contextPointer()));
    mob->edgeHandoverCallback()->setContextPointer(0);

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
      mnRole->sendBUToAll(
        rt->mipv6cds->hmipv6cdsMN->remoteCareOfAddr(), mipv6cdsMN->homeAddr(), bue->lifetime());
      Dout(dc::eh, mob->nodeName()<<" "<<nd->simTime()
           <<" mapAlgorithm sent bu to all based on BA from bue: "<<*bue);
    }
    mob->edgeHandoverCallback()->rescheduleDelay(interval);
  }
  else
  {
    //Called directly from processBA
    if (ehcds->boundMapAddr() == IPv6_ADDR_UNSPECIFIED)
    {
      ipv6_addr peerAddr = ((IPv6Datagram*)(mob->edgeHandoverCallback()->contextPointer()))->srcAddress();
      delete ((IPv6Datagram*)(mob->edgeHandoverCallback()->contextPointer()));
      mob->edgeHandoverCallback()->setContextPointer(0);
    
      MobileIPv6::bu_entry* bue = mipv6cdsMN->findBU(peerAddr);

      mnRole->sendBUToAll(
        rt->mipv6cds->hmipv6cdsMN->remoteCareOfAddr(), mipv6cdsMN->homeAddr(), bue->lifetime());
      Dout(dc::eh, mob->nodeName()<<" "<<nd->simTime()
           <<" First binding with HA so doing it straight away from bue: "<<*bue);

      //Bind every x seconds from map ba
      /*
      mob->edgeHandoverCallback()->cancel();
      mob->edgeHandoverCallback()->rescheduleDelay(interval);
      */
      return;
    }

    Dout(dc::eh, mob->nodeName()<<" "<<nd->simTime()<<" delaying binding with HA until "
         <<mob->edgeHandoverCallback()->arrivalTime());

    //Bind every x seconds from map ba
    /*
    mob->edgeHandoverCallback()->cancel();
    mob->edgeHandoverCallback()->rescheduleDelay(interval);
    */

  }
}

void EHTimedAlgorithm::boundMapChanged(const ipv6_addr& old_map)
{

  Dout(dc::eh, mob->nodeName()<<" "<<nd->simTime()<<" rescheduling for period="<<interval
       <<" as bound map changed to "<<ehcds->boundMapAddr()<<" from "<<old_map);
  mob->edgeHandoverCallback()->rescheduleDelay(interval);
}


}; //namespace EdgeHandover
