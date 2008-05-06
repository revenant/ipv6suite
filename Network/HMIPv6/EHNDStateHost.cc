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
#include <boost/bind.hpp>

#include "EHNDStateHost.h"
#include "MIPv6MNEntry.h"
#include "HMIPv6CDSMobileNode.h"
#include "MIPv6CDSMobileNode.h"
#include "EHCDSMobileNode.h"
#include "cSignalMessage.h"
#include "EHTimers.h"
#include "EHTimedAlgorithm.h"
#include "EHHopAlgorithm.h"
#include "opp_utils.h" //OPP_Global::ContextSwitcher
#include "IPv6Utils.h"
//for tests
#include "NeighbourDiscovery.h"
#include "RoutingTable6.h"
#include "IPv6Mobility.h"
#include "HMIPv6ICMPv6NDMessage.h"
#include "MIPv6CDS.h"
#include "InterfaceTable.h"
#include "MIPv6MStateMobileNode.h"

namespace EdgeHandover
{

  EHNDStateHost* EHNDStateHost::create(NeighbourDiscovery* mod, const std::string& handoverType)
  {
    if ( handoverType == "Timed")
      return new EHTimedAlgorithm(mod);
    else if (handoverType == "Hop")
      return new EHHopAlgorithm(mod);
    return 0;
  }

  ///Timer belongs to NeighbourDiscovery module and should be unique to the constants in NeighbourDiscovery.cc
  //@{
  const int Tmr_EHCallback = 9000;
  const int Tmr_EHBMapChangedCB = 9001;
  //@}

  EHNDStateHost::EHNDStateHost(NeighbourDiscovery* mod):
      HMIPv6NDStateHost(mod), ehcds(rt->mipv6cds->ehcds)
{

  EHCallback* tmr = 
    new EHCallback("EHInvokeMapAlgorithmCallback", Tmr_EHCallback);
  (*tmr)=boost::bind(&EdgeHandover::EHNDStateHost::invokeMapAlgorithmCallback,
			   this);

  ///Create timer message with our callback (timer used if we want timed
  ///notification otherwise used as a callback pointer)
  mob->setEdgeHandoverCallback(tmr);

  check_and_cast<cSimpleModule*>(nd);
  ehcds->bcoaChangedNotifier = boost::bind(&EdgeHandover::EHNDStateHost::invokeBoundMapChangedCallback,
					   this, _1, _2);
}

EHNDStateHost::~EHNDStateHost()
{
  if (mob->edgeHandoverCallback() && mob->edgeHandoverCallback()->isScheduled())
    mob->edgeHandoverCallback()->cancel();
  delete mob->edgeHandoverCallback();
}

/**
   @brief callback function invoked by mob->edgeHandoverCallback()->callFunc()

   Needed so subclasses can override the virtual mapAlgorithm and be invoked
   from here as only invokeMapAlgorithm is bound at compile time.
   
   Datagram is passed via context pointer in EHCallback
*/
void EHNDStateHost::invokeMapAlgorithmCallback()
{
  cContextSwitcher switchContext(nd);
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
ipv6_addr EHNDStateHost::invokeBoundMapChangedCallback(const HierarchicalMIPv6::HMIPv6MAPEntry& map,
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

/**
   @brief override of base class

   @note currentMap needs to be set to the map we are sending to HA
   via preprocessMapHandover if we want it to become the new bcoa.
*/

std::auto_ptr<RA> EHNDStateHost::discoverMAP(std::auto_ptr<RA> rtrAdv)
{
  using HierarchicalMIPv6::HMIPv6ICMPv6NDOptMAP;
  using MobileIPv6::MIPv6RouterEntry;
  using HierarchicalMIPv6::HMIPv6CDSMobileNode;

  IPv6Datagram* dgram = check_and_cast<IPv6Datagram*>(rtrAdv->encapsulatedMsg());
  const ipv6_addr& ll_addr = dgram->srcAddress();
  boost::shared_ptr<MIPv6RouterEntry> accessRouter = ehcds->mipv6cds->mipv6cdsMN->findRouter(ll_addr);
  assert(accessRouter);

  if (mipv6cdsMN->currentRouter() != accessRouter)
  {
    Dout(dc::mobile_move|dc::hmip, rt->nodeName()<<" "//<<setprecision(6)
	 <<nd->simTime()<<" EH MAP algorithm detected movement");
    std::ostream& os = IPv6Utils::printRoutingInfo(false, 0, "", false);
    os <<rt->nodeName() <<" "<<rt->simTime()<<" EH MAP algorithm detected movement"<<"\n";

    if (mipv6cdsMN->primaryHA() == accessRouter && !mipv6cdsMN->bulEmpty())
    {
      Dout(dc::notice|dc::mipv6|dc::mobile_move,  rt->nodeName()<<" "<<nd->simTime()
	   <<" Returning home case detected");
      returnHome();
      relinquishRouter(mipv6cdsMN->currentRouter(), accessRouter);
      return rtrAdv; 
    }

    //Make sure we use the latest router as default router otherwise LBUs are
    //sent via old router
    relinquishRouter(mipv6cdsMN->currentRouter(), accessRouter);
    mipv6cdsMN->setAwayFromHome(true);
  }

  if (!mipv6cdsMN->awayFromHome())
    return rtrAdv;

  HMIPv6CDSMobileNode& hmipv6cdsMN = *(ehcds->mipv6cds->hmipv6cdsMN);
  MAPOptions maps = rtrAdv->mapOptions();
  //assert(maps.size() == 1);
  //assume current AR is a map as shown by map option size of 1
  //dist field can be forwarding limit in future
  ipv6_addr oldMap = hmipv6cdsMN.currentMapAddr();

  const HMIPv6ICMPv6NDOptMAP& mapOpt = *(maps.begin());
  HMIPv6CDSMobileNode::Maps& mapEntries = hmipv6cdsMN.mapEntries();

  if (mapOpt.addr() == oldMap)
    return rtrAdv;

  //can test for prior existence to see if revisited or not

  //continue adding or should we keep it in a ring buffer (no need as MRL
  //already records last visited although would be nice if it was able to record
  //history of moves)
  mapEntries[mapOpt.addr()] = mapOpt;


  InterfaceEntry *ie = ift->interfaceByPortNo(accessRouter->re.lock()->ifIndex());
  assert(dgram->inputPort() >= 0);
  assert((unsigned int)dgram->inputPort() ==accessRouter->re.lock()->ifIndex());
  ipv6_addr lcoa = mipv6cdsMN->formCareOfAddress(accessRouter, ie);
  assert(hmipv6cdsMN.localCareOfAddr() != lcoa);

  //have not got a bound map yet so forget updating our location
  if (ehcds->boundMapAddr() == IPv6_ADDR_UNSPECIFIED)
  {
    //just make first map we see our bound map for now
    preprocessMapHandover(mapOpt,
                          accessRouter, dgram);
    return rtrAdv;
  }

  //current Map in EH is simply last visited MAP/current MAP at AR.
  hmipv6cdsMN.setCurrentMap(mapOpt.addr());

  ipv6_addr currentMap = hmipv6cdsMN.currentMapAddr();
  //arhandover operates on current map and since we want to continue binding
  //with bmap we do this
  hmipv6cdsMN.setCurrentMap(ehcds->boundMapAddr());
  arhandover(lcoa, dgram);
  hmipv6cdsMN.setCurrentMap(currentMap);
  this->hopAlgorithm();

  return rtrAdv;
}

void EHNDStateHost::mapAlgorithm()
{
  using MobileIPv6::MIPv6RouterEntry;
  using HierarchicalMIPv6::HMIPv6CDSMobileNode;

  //Possible that while we have set a valid MAP we have not actually received a BA from it yet 
  //so callback args are not set. This happens due to lossy property of wireless env.
  if (!((IPv6Datagram*)(mob->edgeHandoverCallback()->contextPointer())))
  {
    Dout(dc::eh, " Unable to do anything as BA not received from Map yet");
    return;
  }

  assert(mob->edgeHandoverCallback()->contextPointer());
  IPv6Datagram* dgram = (IPv6Datagram*)(mob->edgeHandoverCallback()->contextPointer());
  ipv6_addr peerAddr = dgram->srcAddress();
  MobileIPv6::bu_entry* bue = mipv6cdsMN->findBU(peerAddr);
  assert(bue);

  if (hmipv6cdsMN.isMAPValid())
    //regardless of distance field treat single map option
    //as meaning eh is enabled in simulated network. 
    //but that is configuration error prone so hence enforce this
    //for now so at least picks up config error which we manually
    //correct instead. TODO add robust code so compatible with 
    //HMIPv6 networks too by modifying EHNDStateHost::discoverMAP
    //to only add MAPs colocated in ARs.
    assert(hmipv6cdsMN.currentMap().distance() == 1);
  else
  {
    delete ((IPv6Datagram*)(mob->edgeHandoverCallback()->contextPointer()));
    mob->edgeHandoverCallback()->setContextPointer(0);
    return;
  }

  if ((ehcds->boundMapAddr() == IPv6_ADDR_UNSPECIFIED && hmipv6cdsMN.remoteCareOfAddr()
       != IPv6_ADDR_UNSPECIFIED)|| (bue->isMobilityAnchorPoint() && 
	   hmipv6cdsMN.remoteCareOfAddr() != IPv6_ADDR_UNSPECIFIED &&
	   //in case we have moved already to a new currentMap and ba manages to
	   //reach us
//	   bue->homeAddr() == hmipv6cdsMN.remoteCareOfAddr() &&
	   hmipv6cdsMN.remoteCareOfAddr() != ehcds->boundCoa()))
  {
    mnRole->sendBUToAll(hmipv6cdsMN.remoteCareOfAddr(), mipv6cdsMN->homeAddr(), bue->lifetime());      
    Dout(dc::eh, mob->nodeName()<<" "<<nd->simTime()
	 <<" mapAlgorithm sent bu to all based on BA from MAP bue: "<<*bue);
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
  } //bu from HA

  delete ((IPv6Datagram*)(mob->edgeHandoverCallback()->contextPointer()));
  mob->edgeHandoverCallback()->setContextPointer(0);

}

void EHNDStateHost::hopAlgorithm()
{
}

HMIPv6MAPEntry EHNDStateHost::selectMAP(MAPOptions &maps, MAPOptions::iterator& new_end)
{
  HMIPv6NDStateHost::selectMAP(maps, new_end);

  std::partial_sort(maps.begin(), maps.begin() + 1, new_end,
                    std::greater<HMIPv6MAPEntry>());
  if (maps.begin() == new_end)
    return invalidMAP;
  return *maps.begin();
}

void EHNDStateHost::returnHome()
{
  HMIPv6NDStateHost::returnHome();
  rt->mipv6cds->ehcds->setNoBoundMap();
}

void EHNDStateHost::boundMapChanged(const ipv6_addr& old_map)
{
  Dout(dc::eh, rt->nodeName()<<" "<<nd->simTime()<<" bound map was "<< old_map
       <<" now is "<<ehcds->boundMapAddr());
}

};
