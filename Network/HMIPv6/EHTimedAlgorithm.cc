// -*- C++ -*-
// Copyright (C) 2004, 2008 Johnny Lai
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
#include "RoutingTable6.h"
#include "MIPv6MStateMobileNode.h"

namespace EdgeHandover
{

///For non omnetpp csimplemodule derived classes
EHTimedAlgorithm::EHTimedAlgorithm(NeighbourDiscovery* mod):
  EHNDStateHost(mod), interval(10)
{
  //Read xml info on Timed algorithm params
  interval = mob->par("timeInterval");
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
  using MobileIPv6::MIPv6RouterEntry;
  using HierarchicalMIPv6::HMIPv6CDSMobileNode;

  HMIPv6CDSMobileNode& hmipv6cdsMN = *(rt->mipv6cds->hmipv6cdsMN);

  Dout(dc::eh, mob->nodeName()<<" "<<nd->simTime()<<" May do map algorithm");
  if (!hmipv6cdsMN.isMAPValid() ||
      hmipv6cdsMN.currentMap().addr() == ehcds->boundMapAddr())
  {
    //Fix for over optimistic mip6 behaviour which can cause race conditions for
    //advanced heuristics that do not bind at every handover
    if (hmipv6cdsMN.isMAPValid() && mipv6cdsMN->careOfAddr() != ehcds->boundCoa() && 
	hmipv6cdsMN.remoteCareOfAddr() == ehcds->boundCoa())
    {
      /* This didn't work needed the actual sending of BU below to work
      //destaddr is ha/cns and coa is boundcoa
      unsigned int ifIndex = 0;
      bool homeReg = true;
      bool mapReg = !homeReg;
      mstateMN->updateTunnelsFrom(mipv6cdsMN->primaryHA()->addr(), ehcds->boundMapAddr(), ifIndex, homeReg, mapReg);
      //trigger cns on old coa tunnel
      */
      Dout(dc::eh, " Does this really happen i.e. can we get here now?");
      assert(false);
      MobileIPv6::bu_entry* bue = mipv6cdsMN->findBU(rt->mipv6cds->hmipv6cdsMN->currentMap().addr());
      mnRole->sendBUToAll(
        rt->mipv6cds->hmipv6cdsMN->remoteCareOfAddr(), mipv6cdsMN->homeAddr(), bue->lifetime());

    }
    if (!mob->edgeHandoverCallback()->isScheduled())
        mob->edgeHandoverCallback()->rescheduleDelay(interval);
    return; //unless state of binding has changed or is about to expire
  }

  if (!mob->edgeHandoverCallback()->isScheduled())
  {
    Dout(dc::eh|flush_cf, mob->nodeName()<<" "<<nd->simTime()<<" invoking map algorithm");

    Dout(dc::eh, mob->nodeName()<<" mip->coa="<<mipv6cdsMN->careOfAddr()
	 <<" hmip->rcoa="<<hmipv6cdsMN.remoteCareOfAddr()<<" eh->bcoa="<<
	 ehcds->boundCoa());

      //hmipv6cdsMN.remoteCareOfAddr() can be 0::0 because we may not have sent a
      //map handover to that yet. (use bcoa if never want to change bmap or
      //as case for timed send reg to map first then to all)

      //dgram used for port no (assume 0) and src addr in debug msg only
      if (hmipv6cdsMN.remoteCareOfAddr() == IPv6_ADDR_UNSPECIFIED ||
	  ehcds->boundCoa() != hmipv6cdsMN.remoteCareOfAddr())
      {
	IPv6Datagram* dgram = new IPv6Datagram();
	dgram->setInputPort(0);
        preprocessMapHandover(hmipv6cdsMN.currentMap(),
			      mipv6cdsMN->currentRouter(), dgram);
	Dout(dc::eh, mob->nodeName()<<" "<<nd->simTime()
	     <<" EHTimedAlgorithm::mapAlgorithm sent bu to new MAP: "<<hmipv6cdsMN.currentMap());

	delete dgram;
      }
    mob->edgeHandoverCallback()->rescheduleDelay(interval);
  }
  else
  {
    EHNDStateHost::mapAlgorithm();
  }
}

void EHTimedAlgorithm::boundMapChanged(const ipv6_addr& old_map)
{

  Dout(dc::eh, mob->nodeName()<<" "<<nd->simTime()<<" rescheduling for period="<<interval
       <<" as bound map changed to "<<ehcds->boundMapAddr()<<" from "<<old_map);
  mob->edgeHandoverCallback()->rescheduleDelay(interval);
}


}; //namespace EdgeHandover
