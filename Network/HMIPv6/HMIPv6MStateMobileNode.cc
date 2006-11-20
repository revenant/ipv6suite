// -*- C++ -*-
// Copyright (C) 2006 Johnny Lai
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
 * @file   HMIPv6MStateMobileNode.cc
 * @author Johnny Lai
 * @date   20 Nov 2006
 *
 * @brief  Implementation of HMIPv6MStateMobileNode
 *
 * @todo separate EH stuff out
 *
 */

//Headers for libcwd debug streams have to be first (remove if not used)
#include "sys.h"
#include "debug.h"

#include "HMIPv6MStateMobileNode.h"

#include "InterfaceTable.h"
#include "IPv6Mobility.h"
#include "MIPv6CDSMobileNode.h"
#include "MIPv6MNEntry.h"
#include "IPv6Encapsulation.h"
#include "RoutingTable6.h" 
#include "MobilityHeaders.h"

#include "HMIPv6CDSMobileNode.h"
using HierarchicalMIPv6::HMIPv6CDSMobileNode;
#if EDGEHANDOVER
#include "NeighbourDiscovery.h"
#include "EHNDStateHost.h"
#include "EHTimers.h"
#include "EHCDSMobileNode.h"
#endif //EDGEHANDOVER


using MobileIPv6::bu_entry;

namespace HierarchicalMIPv6
{

///For non omnetpp csimplemodule derived classes
HMIPv6MStateMobileNode::HMIPv6MStateMobileNode(IPv6Mobility* mod):
  MIPv6MStateMobileNode(mod)
{
  InterfaceTable *ift = mod->ift;
  hmipv6cds = new HierarchicalMIPv6::HMIPv6CDSMobileNode(mipv6cds, ift->numInterfaceGates());
  mipv6cds->hmipv6cdsMN = hmipv6cds;
  if (mob->edgeHandover())
  {
    ehcds = new EdgeHandover::EHCDSMobileNode(mipv6cds, ift->numInterfaceGates());
    mipv6cds->ehcds = ehcds;
  }
}

HMIPv6MStateMobileNode::~HMIPv6MStateMobileNode()
{
  delete hmipv6cds;
  delete ehcds;
}

void HMIPv6MStateMobileNode::initialize(int stage)
{
  MIPv6MStateMobileNode::initialize(stage);
}

bool HMIPv6MStateMobileNode::updateTunnelsFrom
(ipv6_addr budest, ipv6_addr coa, unsigned int ifIndex,
 bool homeReg, bool mapReg)
{
  if (homeReg || mapReg)
  {
    MIPv6MStateMobileNode::updateTunnelsFrom(budest, coa, ifIndex, homeReg, mapReg);

    HMIPv6CDSMobileNode* hmipv6cds = mipv6cds->hmipv6cdsMN;

    size_t vIfIndex = tunMod->findTunnel(coa, budest);
    if (homeReg)
    {
      //trigger on HA to lcoa tunnel if coa was formed from map prefix

      //list of maps in here may be smaller or bigger than in bul depends on
      //whether the map list tracks the actual maps in current domain @see 
      //hmipv6ndstatehost::discoverMap
      ipv6_addr map = hmipv6cds->findMapOwnsCoa(coa);
      if (map != IPv6_ADDR_UNSPECIFIED)
      {
	bu_entry* bue = mipv6cdsMN->findBU(map);
	if (bue)
	{
	  vIfIndex = tunMod->findTunnel(bue->careOfAddr(), map);
	  assert(vIfIndex);
	  //trigger on ha only after BA for maximum correctness but nonoptimal
	  //operation?
	  tunMod->tunnelDestination(mipv6cdsMN->primaryHA()->addr(), vIfIndex);
	}
      }
    }
    else
    {
      //trigger on HA to different lcoa tunnel as we moved to different AR only
      //if this map is registered at HA
      if (mipv6cdsMN->primaryHA().get())
      {
	bu_entry* bue = mipv6cdsMN->findBU(mipv6cdsMN->primaryHA()->addr());
	if (bue && ipv6_prefix(budest, EUI64_LENGTH).matchPrefix(
	      bue->careOfAddr()))
	{
	  mob->bubble("trigger on HA for lcoa -> MAP during map reg");
	  // vIfIndex points to tunnel lcoa -> map
	  tunMod->tunnelDestination(mipv6cdsMN->primaryHA()->addr(), vIfIndex);
	}
      }
      //Find all cn entries and trigger them to new lcoa tunnel if their coa
      //belongs to this map's prefix (if there was a map handover then will need
      //to wait for bu to cn b4 trigger to diff map)
      vector<ipv6_addr> addrs =
	mipv6cdsMN->findBUToCNCoaMatchPrefix(budest);
      for (vector<ipv6_addr>::iterator it = addrs.begin(); it != addrs.end();
	   ++it)
      {       
	tunMod->tunnelDestination(*it, vIfIndex);
      }
    }
  }
  else //not (home or map reg)
  {
    //trigger on cn address to lcoa tunnel if coa formed from map prefix

    if (!mipv6cdsMN->awayFromHome())
    {
      //remove tunnel to cn
      //tunMod->untunnelDestination(budest);
      // should not be necessary as action of removing tunnels removes associated triggers
      return true;
    }

    HMIPv6CDSMobileNode* hmipv6cds = mipv6cds->hmipv6cdsMN;

    //list of maps in here may be smaller or bigger than in bul depends on
    //whether the map list tracks the actual maps in current domain @see 
    //hmipv6ndstatehost::discoverMap
    ipv6_addr map = hmipv6cds->findMapOwnsCoa(coa);
    if (map != IPv6_ADDR_UNSPECIFIED)
    {
      bu_entry* bue = mipv6cdsMN->findBU(map);
      if (bue)
      {
	size_t vIfIndex = tunMod->findTunnel(bue->careOfAddr(), map);
	assert(vIfIndex);
	tunMod->tunnelDestination(budest, vIfIndex);
      }
    }
  }

  return true;
}

bool HMIPv6MStateMobileNode::processBA(BA* ba, IPv6Datagram* dgram)
{
  bool cont = MIPv6MStateMobileNode::processBA(ba, dgram);

  if (!cont)
    return cont;

  simtime_t now = mob->simTime();
  bu_entry* bue = mipv6cdsMN->findBU(dgram->srcAddress());

  if (hmipv6cds->isMAPValid() && hmipv6cds->currentMap().addr() == dgram->srcAddress())
  {
    Dout(dc::hmip, mob->nodeName()<<" "<<now<<" BA from MAP "
	 <<dgram->srcAddress());

    //Assuming single interface for now if assumption not true revise code
    //accordingly
    assert(dgram->inputPort() == 0);
    if (!mob->rt->addrAssigned(hmipv6cds->remoteCareOfAddr(), dgram->inputPort()))
    {
      IPv6Address addrObj(hmipv6cds->remoteCareOfAddr());
      addrObj.setPrefixLength(EUI64_LENGTH);
      addrObj.setStoredLifetimeAndUpdate(ba->lifetime());
      addrObj.setPreferredLifetime(ba->lifetime());
      mob->rt->assignAddress(addrObj, dgram->inputPort());
    }

    if (mipv6cdsMN->careOfAddr() != hmipv6cds->remoteCareOfAddr())
    {
#if EDGEHANDOVER
      if (!mob->edgeHandover())
      {
#endif //EDGEHANDOVER
	Dout(dc::hmip, " sending BU to all coa="
	     <<hmipv6cds->remoteCareOfAddr()<<" hoa="<<mipv6cdsMN->homeAddr());

	//Map is skipped
	sendBUToAll(hmipv6cds->remoteCareOfAddr(), mipv6cdsMN->homeAddr(),
		    bue->lifetime());
#if EDGEHANDOVER
      }
      else
      {
	if (!mob->edgeHandoverCallback())
	  //Exit code of 254
	  DoutFatal(dc::fatal, "You forgot to set the callback for edge handover cannot proceed");
	else
	{
	  //if rcoa does not prefix match the dgram->srcAddress we ignore
	  //since we do not want to bind with HA using a coa from a previous MAP.

	  Dout(dc::eh, mob->nodeName()<<" invoking eh callback based on BA from bue "<<*bue
	       <<" coa="<<mipv6cdsMN->careOfAddr() <<" rcoa="<<hmipv6cds->remoteCareOfAddr()
	       <<" bcoa "<<ehcds->boundCoa());
	  mob->edgeHandoverCallback()->setContextPointer(dgram->dup());
	  assert(dgram->inputPort() == 0);
	  mob->edgeHandoverCallback()->callFunc();

	  //Not something we should do as routerstate is for routers only
	  //               EdgeHandover::EHNDStateHost* ehstate =
	  //                 boost::polymorphic_downcast<EdgeHandover::EHNDStateHost*>
	  //                 (boost::polymorphic_downcast<NeighbourDiscovery*>(
	  //                   OPP_Global::findModuleByType(mob->rt, "NeighbourDiscovery"))->getRouterState());

	  //               ehstate->invokeMapAlgorithmCallback(dgram);

	}
      }
#endif //EDGEHANDOVER
    }
  }
#if EDGEHANDOVER
  else if (hmipv6cds->isMAPValid() && mob->edgeHandover() &&
	   dgram->srcAddress() == mipv6cdsMN->primaryHA()->addr() &&
	   ///Since we've restricted boundMaps to be only ARs (existing
	   ///MIPv6RouterEntry when calling setBoundMap) they have to have a
	   ///distance of 1.
	   //Not true as we may want to bind with a previous map along the
	   //edge so it will have distance > 1
	   hmipv6cds->currentMap().distance() >= 1)
  {
    ///If the HA's BA is acknowledging binding with another MAP besides the
    ///currentMap's then this code block needs to be revised accordingly. It
    ///is possible for currentMap to have changed whilst updating HA.
    if (bue->careOfAddr() != hmipv6cds->remoteCareOfAddr())
    {
      Dout(dc::warning|flush_cf, "Bmap at HA is not the same as what we thought it was coa(HA)"
	   <<" perhaps due to BA not arriving to us previously? "
	   <<bue->careOfAddr()<<" our record of rcoa "<<hmipv6cds->remoteCareOfAddr());
      //assert(bue->careOfAddr() == hmipv6cds->remoteCareOfAddr());
    }

    Dout(dc::eh|flush_cf, mob->nodeName()<<" bmap is now "<<hmipv6cds->currentMap().addr()
	 <<" inport="<<dgram->outputPort());
    ///outputPort should contain outer header's inputPort so we know which
    ///iface packet arrived on (see IPv6Encapsulation::handleMessage decapsulateMsgIn branch)
    ///original inport needed so we can configure the proper bcoa for multi homed MNs
    assert(dgram->outputPort() >= 0);
    ehcds->setBoundMap(hmipv6cds->currentMap(), dgram->outputPort());
  }
#endif //EDGEHANDOVER
  return false;
}

} // namespace HierarchicalMIPv6

