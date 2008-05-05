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
#include "MIPv6MStateMobileNode.h"
#include "RoutingTable6.h"

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
    //Possible that while we have set a valid MAP we have not actually received a BA from it yet 
    //so callback args are not set. This happens due to lossy property of wireless env.
    if (!((IPv6Datagram*)(mob->edgeHandoverCallback()->contextPointer())))
    {
      Dout(dc::eh, " Unable to do anything as BA not received from Map yet");
      mob->edgeHandoverCallback()->rescheduleDelay(interval);
      return;
    }

    Dout(dc::eh, mob->nodeName()<<" mip->coa="<<mipv6cdsMN->careOfAddr()
	 <<" hmip->rcoa="<<hmipv6cdsMN.remoteCareOfAddr()<<" eh->bcoa="<<
	 ehcds->boundCoa());

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
	     <<" mapAlgorithm sent bu to all based on BA from bue: "<<*bue);

	delete dgram;
      }
    }
    mob->edgeHandoverCallback()->rescheduleDelay(interval);
  }
  else
  {
    //Called directly from processBA
    assert(mob->edgeHandoverCallback()->contextPointer());
    IPv6Datagram* dgram = (IPv6Datagram*)(mob->edgeHandoverCallback()->contextPointer());
    ipv6_addr peerAddr = dgram->srcAddress();
    MobileIPv6::bu_entry* bue = mipv6cdsMN->findBU(peerAddr);

    if (hmipv6cdsMN.isMAPValid())
      //regardless of distance field treat single map option
      //as meaning eh is enabled in simulated network. 
      //but that is configuration error prone so hence enforce this
      //for now so at least picks up config error which we manually
      //correct instead. TODO add robust code so compatible with 
      //HMIPv6 networks too by modifying EHNDStateHost::discoverMAP
      //to only add MAPs colocated in ARs.
      assert(hmipv6cdsMN.currentMap().distance() == 1);

    if (ehcds->boundMapAddr() == IPv6_ADDR_UNSPECIFIED)
    {
      delete ((IPv6Datagram*)(mob->edgeHandoverCallback()->contextPointer()));
      mob->edgeHandoverCallback()->setContextPointer(0);
    

      Dout(dc::eh, mob->nodeName()<<" "<<nd->simTime()
           <<" First binding with HA so doing it straight away from bue: "<<*bue);
      mnRole->sendBUToAll(
        rt->mipv6cds->hmipv6cdsMN->remoteCareOfAddr(), mipv6cdsMN->homeAddr(), bue->lifetime());

      //Bind every x seconds from map ba
      /*
      mob->edgeHandoverCallback()->cancel();
      mob->edgeHandoverCallback()->rescheduleDelay(interval);
      */
      return;
    }
    else if (bue->isMobilityAnchorPoint() &&
	     hmipv6cdsMN.remoteCareOfAddr() != IPv6_ADDR_UNSPECIFIED &&
	     hmipv6cdsMN.remoteCareOfAddr() != ehcds->boundCoa())
    {
      delete ((IPv6Datagram*)(mob->edgeHandoverCallback()->contextPointer()));
      mob->edgeHandoverCallback()->setContextPointer(0);
      mnRole->sendBUToAll(hmipv6cdsMN.remoteCareOfAddr(), mipv6cdsMN->homeAddr(), bue->lifetime());      
      Dout(dc::eh, mob->nodeName()<<" "<<nd->simTime()
           <<" mapAlgorithm sent bu to all based on BA from MAP bue: "<<*bue);
      return;
    }
    else if (peerAddr == mipv6cdsMN->primaryHA()->addr() &&
	     hmipv6cdsMN.isMAPValid() &&
      //make sure network is configured (eh not compatible with hmip then!)
      //to have only maps at ARs. otherwise enforce below      
	     hmipv6cdsMN.currentMap().distance() == 1)
    {
      ///If the HA's BA is acknowledging binding with another MAP besides the
      ///currentMap's then this code block needs to be revised accordingly. It
      ///is possible for currentMap to have changed whilst updating HA.
      if (bue->careOfAddr() != hmipv6cdsMN.remoteCareOfAddr())
      {
	Dout(dc::warning|flush_cf, "Bmap at HA is not the same as what we thought it was coa(HA)"
	     <<" perhaps due to BA not arriving to us previously? "
	     <<bue->careOfAddr()<<" our record of rcoa "<<hmipv6cdsMN.remoteCareOfAddr());
	assert(bue->careOfAddr() == hmipv6cdsMN.remoteCareOfAddr());
      }
      else
      {
	Dout(dc::eh|flush_cf, mob->nodeName()<<" bmap is now "<<hmipv6cdsMN.currentMap().addr()
	     <<" inport="<<dgram->outputPort());
	///outputPort should contain outer header's inputPort so we know which
	///iface packet arrived on (see IPv6Encapsulation::handleMessage decapsulateMsgIn branch)
	///original inport needed so we can configure the proper bcoa for multi homed MNs
	assert(dgram->outputPort() >= 0);
	ehcds->setBoundMap(hmipv6cdsMN.currentMap(), dgram->outputPort());
      }
      delete ((IPv6Datagram*)(mob->edgeHandoverCallback()->contextPointer()));
      mob->edgeHandoverCallback()->setContextPointer(0);
      return;
    } //bu from HA



      //    Dout(dc::eh, mob->nodeName()<<" "<<nd->simTime()<<" delaying binding with HA until "
      //         <<mob->edgeHandoverCallback()->arrivalTime());

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
