//
// Copyright (C) 2002, 2003, 2004 CTIE, Monash University
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
 * @file   HMIPv6NDStateHost.cc
 * @author Johnny Lai
 * @date   04 Sep 2002
 *
 * @brief  Implementation of HMIPv6NDStateHost class
 *
 *
 */


#include <boost/cast.hpp>
#include "sys.h"
#include "debug.h"


#include "HMIPv6NDStateHost.h"

#include "HMIPv6ICMPv6NDMessage.h"
#include "HMIPv6Entry.h"
#include "HMIPv6CDSMobileNode.h"
#include "InterfaceTable.h"
#include "RoutingTable6.h"
#include "IPv6CDS.h"
#include "MIPv6MStateMobileNode.h"
#include "MIPv6MNEntry.h"
#include "NeighbourDiscovery.h"
#include "IPv6Encapsulation.h"
#include "opp_utils.h"
#if EDGEHANDOVER
#include "EHCDSMobileNode.h"
#endif //EDGEHANDOVER
#include <memory>


using MobileIPv6::MIPv6RouterEntry;
using MobileIPv6::bu_entry;


namespace HierarchicalMIPv6
{

HMIPv6NDStateHost::HMIPv6NDStateHost(NeighbourDiscovery* mod)
  :MIPv6NDStateHost(mod),
   hmipv6cdsMN(*(boost::polymorphic_downcast<HMIPv6CDSMobileNode*> (mipv6cdsMN)))
{}

HMIPv6NDStateHost::~HMIPv6NDStateHost()
{}

std::auto_ptr<ICMPv6Message>
HMIPv6NDStateHost::processMessage(std::auto_ptr<ICMPv6Message> msg)
{
  return this->MobileIPv6::MIPv6NDStateHost::processMessage(msg);
}

static const  HMIPv6MAPEntry invalidMAP;

std::auto_ptr<RA> HMIPv6NDStateHost::processRtrAd(std::auto_ptr<RA> rtrAdv)
{
  //Do it here for now to get lcoa autoconf'ed and router added to MRL
  //rtrAdv.reset(MIPv6NDStateHost::processRtrAd(rtrAdv).release());
  rtrAdv = MIPv6NDStateHost::processRtrAd(rtrAdv);

  assert(rtrAdv.get() != 0);
  if (rtrAdv.get() == 0)
  {
    DoutFatal(dc::core|error_cf, rt->nodeName()
         <<" HMIP rtrAdv was invalidated after normal MIPv6 processing");
    return rtrAdv;
  }

  IPv6Datagram* dgram = check_and_cast<IPv6Datagram*>(rtrAdv->encapsulatedMsg());

  //!mipv6cdsMN->primaryHA() does not imply that we do not allow map reg when
  //there is no reg with ha yet. It simply means that we process map options
  //only if we know who the pHA is. Perhaps that is too restrictive? Maybe we
  //can just use map as HA even if we do not have a pHA?
  if (!rtrAdv->hasMapOptions() || !mipv6cdsMN->primaryHA())
    //We have to store map options even if we do not know if we've moved away from home yet
// || !rt->awayFromHome())
  {

    if (!hmipv6cdsMN.isMAPValid())
      return rtrAdv;

    if (mipv6cdsMN->currentRouter() && mipv6cdsMN->currentRouter()->re.lock() &&
    dgram->srcAddress() != mipv6cdsMN->currentRouter()->re.lock()->addr())
    {
      //Store old map address so we can do forwarding from MAP to coa when no map
      //What if many RA and only some have map options? Do we want to trigger any
      //handover due to absence of map option from same router or even different
      //router in same subnet?
    }
    Dout(dc::hmip," Set no map as no map option in RA");
    hmipv6cdsMN.setNoCurrentMap();
    //Prob. should send BU to MAP i.e. tell it to deregister us?
    //Prob. should delete entry from BUL too or implement lifetimes for bule to
    //remove them automatically

    //Remove rcoa and bind with pHA with lcoa? (guess this is a new movement
    //detection method because we can detect movement here too)

    //Assuming 0 because multihomed hosts would be problematic I mean should we
    //check which RA comes from which ifIndex before doing this?
    rt->removeAddress(hmipv6cdsMN.remoteCareOfAddr(), dgram->inputPort());
    const ipv6_addr& ll_addr = dgram->srcAddress();
    boost::shared_ptr<MIPv6RouterEntry> accessRouter = mipv6cdsMN->findRouter(ll_addr);
    //bind to pHA with coa of lcoa?
    MIPv6NDStateHost::handover(accessRouter);
    return rtrAdv;
  }

  return discoverMAP(rtrAdv);
}

///predicate for detecting invalid maps i.e. one we cannot bind to
bool isInvalidMAP(const MAPOptions::value_type& m)
{
   assert(m.dist() > 0);

   //No more m option in latest HMIP drafts as no more extended mode only basic
   assert(!m.m());

   assert(m.addr() != IPv6_ADDR_UNSPECIFIED);

   assert(m.pref() <= 0xF);

   return !m.lifetime() || !m.pref() ?true:false;
}


///Selection of map occurs in this virtual function
HMIPv6MAPEntry HMIPv6NDStateHost::selectMAP(MAPOptions& maps, MAPOptions::iterator& new_end)
{
  new_end = remove_if(maps.begin(), maps.end(), isInvalidMAP);
  std::partial_sort(maps.begin(), maps.begin() + 1, new_end,
                    std::less<HMIPv6MAPEntry>());
  if (maps.begin() == new_end)
    return invalidMAP;
  return *maps.begin();
}


/**
   @warning Assuming map options are not advertised in home subnet (makes
   detection of handover somewhat easier otherwise we'd wait for
   movementDetected() trigger before we can handover to map and we'd have to
   cache it too as a potential map ...)
 */
std::auto_ptr<RA> HMIPv6NDStateHost::discoverMAP(std::auto_ptr<RA> rtrAdv)
{

  MAPOptions maps = rtrAdv->mapOptions();
  assert(maps.size() > 0);

  HMIPv6MAPEntry curMapCopy = hmipv6cdsMN.isMAPValid()?hmipv6cdsMN.currentMap():invalidMAP;
  MAPOptions::iterator new_end;
  HMIPv6MAPEntry bestMap = selectMAP(maps, new_end);

  bool curMapFound = false;

  //Add/Replace entry in mapEntries
  HMIPv6CDSMobileNode::Maps& mapEntries = hmipv6cdsMN.mapEntries();
  for (MAPOptions::const_iterator it = maps.begin(); it != maps.end(); it++)
  {
    const HMIPv6ICMPv6NDOptMAP& mapOpt = *it;
    if (mapEntries.count(mapOpt.addr()) &&
        mapEntries[mapOpt.addr()] != mapOpt)
    {
      //Call virtual function to handle changed map property
      //(origMap, mapOpt)
    }

    if (mapOpt.addr() == curMapCopy.addr())
      curMapFound = true;
    mapEntries[mapOpt.addr()] = mapOpt;
  }



  //Call set_difference to remove unseen map options (will have to convert them
  //both to same type either entry or option and sort them too)
  //comp fn to compare addr only.
  //std::set_difference(mapEntries.begin(), mapEntries.end(), mapOpt.begin(), mapOpt.end(), back_inserter<> , compFunction);


  IPv6Datagram* dgram = check_and_cast<IPv6Datagram*>(rtrAdv->encapsulatedMsg());

  const ipv6_addr& ll_addr = dgram->srcAddress();
  boost::shared_ptr<MIPv6RouterEntry> accessRouter = mipv6cdsMN->findRouter(ll_addr);
  assert(accessRouter);


  if (!mipv6cdsMN->currentRouter())
  {
    //Do this here rather than in MIPv6NDStateHost so we can do map handover rather than
    //just AR handover.
    relinquishRouter(mipv6cdsMN->currentRouter(), accessRouter);
  }

  typedef Loki::cTimerMessageCB<void, TYPELIST_1(ArgMapHandover)> cbSendMapBU;

  //Not in current MAP domain so handover to new map
  if ((!curMapFound && curMapCopy != invalidMAP) ||
      //Found map with better pref or started off with no map
      bestMap.addr() != curMapCopy.addr() ||
      //Don't have a currentMAP
      curMapCopy == invalidMAP)
  {
    //handover choice
    preprocessMapHandover(bestMap, accessRouter, dgram);
    return rtrAdv;
  }
  
  if (curMapCopy != invalidMAP &&
           curMapCopy != hmipv6cdsMN.currentMap())
  {
    if (curMapCopy.distance() != hmipv6cdsMN.currentMap().distance())
    {
      //  movement detected?
      Dout(dc::hmip, rt->nodeName()<<" rtrAdv from "<<dgram->srcAddress()
           <<" MAP option "<<curMapCopy.addr()
           <<" distance changed from "<<curMapCopy.distance()<<" to "
           <<hmipv6cdsMN.currentMap().distance());
      //defer to next commented out if branch
    }
    else
    {
      //TODO Current Map has changed properties.  Usually it may just be lifetime
      //so update lifetime of RCOA
      return rtrAdv;
    }
  }
  
  //do ar handover

//  else if (curMapCopy != invalidMAP &&
//           curMapCopy == hmipv6cdsMN.currentMap())
  {
    InterfaceEntry *ie = ift->interfaceByPortNo(accessRouter->re.lock()->ifIndex());
    ipv6_addr lcoa = mipv6cdsMN->formCareOfAddress(accessRouter, ie);
    if (hmipv6cdsMN.localCareOfAddr() == lcoa)
      return rtrAdv;

    bool assigned = false;
    if (rt->odad())
    {
      assert(ie->ipv6()->addrAssigned(lcoa)||ie->ipv6()->tentativeAddrAssigned(lcoa));
      if (ie->ipv6()->addrAssigned(lcoa) || ie->ipv6()->tentativeAddrAssigned(lcoa))
      {
        assigned = true;
      }
    }
    else if (ie->ipv6()->addrAssigned(lcoa))
    {
      assigned = true;
    }

    if (assigned)
    {
      arhandover(lcoa);
      return rtrAdv;
    }

    //MIPv6NDStateHost used to/supposed to handle this case in sendBU. Don't know
    //how/why it does not now (may be due to eagerHandover and thinking
    //existence of map options should defer to here) . Anyway even if it did it is
    //not optimal as it relies on RtrAdv to trigger actual sending of BU instead
    //of binding handover to dupAddrDetSuccess.
    typedef Loki::cTimerMessageCB<bool, TYPELIST_1(ipv6_addr)> cbSendMapBUAR;
    cbSendMapBUAR* ocb = 0;
    if (callbackAdded(lcoa, 5445))
    {
      ocb = boost::polymorphic_downcast<cbSendMapBUAR*>(addressCallbacks[lcoa]);
      if (ocb && Loki::Field<0>(ocb->args) == lcoa)
        return rtrAdv;
      else
      {
        ocb->cancel();
        delete ocb;
      }
    }

    ocb = new cbSendMapBUAR(5445, nd, this, &HMIPv6NDStateHost::arhandover, "SendMAPBUAR");
    Loki::Field<0>(ocb->args) = lcoa;
    Dout(dc::hmip, rt->nodeName()<<" RtrAdv received from "<<dgram->srcAddress()
         <<" iface="<<dgram->inputPort()<<" HMIP lcoa="<<lcoa<<" not assigned yet undergoing DAD "
         <<"doing arhandover as MAP not changed");
    addCallbackToAddress(lcoa, ocb);
  }

  return rtrAdv;
}


void HMIPv6NDStateHost::
preprocessMapHandover(const HMIPv6MAPEntry& bestMap,
                      boost::shared_ptr<MobileIPv6::MIPv6RouterEntry> accessRouter,
                      IPv6Datagram* dgram)
{
  bool useRcoa = false;
  //Treating them all as meaning always use RCOA as src
  if ( bestMap.i() || bestMap.p())
    useRcoa = true;

  if (bestMap.v())
  {
    Dout(dc::hmip, rt->nodeName()<<" MAP at "<<bestMap.addr()<<" V set - should trigger on HA for lcoa tunnel");
  }

  ///Form LCOA
  //This lcoa registration with MAP should be done by HMIP handover.
  //We leave processBA to do pHA binding of hoa and rcoa
  InterfaceEntry *ie = ift->interfaceByPortNo(accessRouter->re.lock()->ifIndex());

  ipv6_addr lcoa = mipv6cdsMN->formCareOfAddress(accessRouter, ie);
  typedef Loki::cTimerMessageCB<void, TYPELIST_1(ArgMapHandover)> cbSendMapBU;
  cbSendMapBU* ocb = 0;
  if (callbackAdded(lcoa, 5444))
    ocb = boost::polymorphic_downcast<cbSendMapBU*>(addressCallbacks[lcoa]);

  //Checking for outstanding BU that are waiting on DAD of lcoa for the same
  //MAP. If MAP is different I guess we use this one?

  if (ocb)
  {
    const HMIPv6MAPEntry& potentialMap =  boost::get<0>(Loki::Field<0>(ocb->args));
    if (bestMap == potentialMap)
      return;
    else
    {
      Dout(dc::hmip, " Need to change map arg to better map while old map is outstanding on lcoa dad");
      assert(false);
    }
  }

  ipv6_addr rcoa = formRemoteCOA(bestMap, dgram->inputPort());
  assert(dgram->inputPort() > IMPL_INPUT_PORT_LOCAL_PACKET && (unsigned int)dgram->inputPort() == accessRouter->re.lock()->ifIndex());

  ArgMapHandover arg = boost::make_tuple(bestMap, rcoa, lcoa, nd->simTime(), accessRouter->re.lock()->ifIndex());
  //requires tuple_io.hpp
  //cerr<<"arg is "<<arg;
  bool assigned = false;
  if (rt->odad())
  {
    assert(ie->ipv6()->addrAssigned(lcoa)||ie->ipv6()->tentativeAddrAssigned(lcoa));
    if (ie->ipv6()->addrAssigned(lcoa) || ie->ipv6()->tentativeAddrAssigned(lcoa))
    {
//        Dout(dc::hmip, rt->nodeName()<<" rcoa="<<rcoa
//             <<" ODAD assigned from MAPOption "<<bestMap<<" at "<<nd->simTime());
      assigned = true;
    }
  }
  else if (ie->ipv6()->addrAssigned(lcoa))
  {
    assigned = true;
  }

  ///The initial MAP may be false since home domain can have map options
  ///too. But since its hard to use missedRtrAdv to trigger real layer 3
  ///movement culminating in hmip handover will assume it is for now. (to do
  ///so requires storing the arg tuple in some other place besides
  ///addressCallbacks, because that has race conditions between RtrAdvInt and
  ///DAD timeout)

  //if (curMapCopy != invalidMAP)
  movementDetectedCallback(static_cast<cTimerMessage*>(0));

  ///Rest of this section should really be called handover() but then we'd
  ///have to have data members for bestMap/accessRouter||ifindex and forward
  ///calls intelligently to base class unless we really reached this point
  ///(TODO refactor for this case?)
  if (assigned)
  {

    //sending BU to MAP as lcoa assigned

    //hmipv6cdsMN.setFutureRCOA(rcoa);


    Dout(dc::hmip, rt->nodeName()<<" lcoa="<<lcoa
         <<" assigned already so binding with best map MAPOption "<<bestMap<<" at "<<nd->simTime());


    //sendBUtomap
    //Then in processBA send BU to HA if bu hoa was different to coa of primaryha binding
    //oldrcoa is in pha

    //mapHandover(bestMap, rcoa, lcoa, nd->simTime(), accessRouter.lock()->re->ifIndex());
    mapHandover(arg);
  }
  else
  {
    Dout(dc::notice, rt->nodeName()<<" RtrAdv received from "<<dgram->srcAddress()
         <<" iface="<<dgram->inputPort()<<" HMIP lcoa not assigned yet undergoing DAD");

    cbSendMapBU* cb = new cbSendMapBU(5444, nd, this, &HMIPv6NDStateHost::mapHandover, "SendMAPBU-lcoa");
    Loki::Field<0>(cb->args) = arg;

    addCallbackToAddress(lcoa, cb);

  }
}


/*
  @brief sends BU to new map and forwards from old map too

  @todo Add extra arg so we can look at the timer message and check how much
  delay has occurred (to calculate accurate binding lifetimes).

  @callgraph
 */
void HMIPv6NDStateHost::mapHandover(const ArgMapHandover& t)
{
  using boost::get;
  const HMIPv6MAPEntry& bestMap = get<0>(t);
  const ipv6_addr& rcoa = get<1>(t);
  const ipv6_addr& lcoa = get<2>(t);
  simtime_t was = get<3>(t);
  unsigned int ifIndex = get<4>(t);

  //if map valid
  //ipv6_addr oldlcoa = hmipv6cdsMN.localCareOfAddr() != IPv6_ADDR_UNSPECIFIED;

  //rcoa = hmipv6cdsMN.futureRCOA();
  //oldrcoa = hmipv6cdsMN.remoteCareOfAddr();

  //Round down?
  unsigned int lifetime = static_cast<unsigned int> (bestMap.lifetime() - (nd->simTime() - was));
  HMIPv6MAPEntry curMapCopy = hmipv6cdsMN.isMAPValid()?hmipv6cdsMN.currentMap():invalidMAP;

  //Need to wait for BA from new MAP before setting it to currentMAP?
  //Also registering with the HA and perhaps CN too.
  hmipv6cdsMN.setCurrentMap(bestMap.addr());

  //sendBU to new/better MAP initially. (inter AR handover is done by by mip handover)
  //sendBU to HA at processBA of MIPv6 when BA from MAP received. (rcoa is valid now)

  //lifetime should really be min(map.lifetime(), lifetime of lcoa)
  mstateMN->sendMapBU(hmipv6cdsMN.currentMap().addr(), lcoa,
                      rcoa,
                      //This may not be exact lifetime as DAD delay will have reduced this
                      lifetime,
                      ifIndex, mob);
  mob->lbuVector->record(nd->simTime());

  IPv6Encapsulation* tunMod = check_and_cast<IPv6Encapsulation*>
    (OPP_Global::findModuleByType(rt, "IPv6Encapsulation"));
  assert(tunMod != 0);



  //Create rcoa to HA tunnel
  size_t vIfIndex = tunMod->findTunnel(rcoa, mipv6cdsMN->primaryHA()->prefix().prefix);
  //assert(!vIfIndex);

  if (!vIfIndex)
  {
  vIfIndex = tunMod->createTunnel(rcoa, mipv6cdsMN->primaryHA()->prefix().prefix, 0);
    Dout(dc::hmip|dc::encapsulation|dc::debug|flush_cf, rt->nodeName()<<" (mapHandover) reverse tunnel created entry rcoa="
         <<rcoa<<" exit ha="<< mipv6cdsMN->primaryHA()->prefix()<<" vIfIndex="<<hex<<vIfIndex<<dec);
  }
  else
  {
    Dout(dc::hmip|dc::encapsulation, "(mapHandover) reverse tunnel exists already rcoa="
         <<rcoa<<" exit ha="<< mipv6cdsMN->primaryHA()->prefix()<<" vIfIndex="<<hex<<vIfIndex<<dec);
    Dout(dc::hmip|dc::encapsulation, *tunMod);
  }

  //Create lcoa to MAP tunnel
  if (bestMap.v())
  {
    vIfIndex = tunMod->findTunnel(lcoa, hmipv6cdsMN.currentMap().addr());

    //assert(!vIfIndex);
    //Sometimes old tunnel is not removed so it may have been created already when
    //we revisit past ARs
    if (!vIfIndex)
    {
    //assuming single mobile interface at 0
    vIfIndex = tunMod->createTunnel(lcoa, hmipv6cdsMN.currentMap().addr(), 0, mipv6cdsMN->primaryHA()->prefix().prefix);
      Dout(dc::hmip|dc::encapsulation|dc::debug|flush_cf, rt->nodeName()<<" (mapHandover) reverse tunnel created entry lcoa="
           <<lcoa<<" exit map="<< hmipv6cdsMN.currentMap().addr()<<" vIfIndex="<<hex<<vIfIndex<<dec
           <<" V flag set so triggering on HA="<<mipv6cdsMN->primaryHA()->prefix().prefix);

    }
    else
    {
      Dout(dc::hmip|dc::encapsulation, "(mapHandover) reverse tunnel exists already lcoa="
           <<lcoa<<" exit map="<< hmipv6cdsMN.currentMap().addr()<<" vIfIndex="<<hex<<vIfIndex<<dec);
      Dout(dc::hmip|dc::encapsulation, *tunMod);
    }
  }

  if (curMapCopy == invalidMAP)
  {
    Dout(dc::hmip, rt->nodeName()<<" Initial MAP "<<bestMap.addr()
         <<" registration rcoa="<<rcoa<<" lcoa="<<lcoa);
  }
  else
  {

    if (!mipv6cdsMN->primaryHA().get())
      return;

    //Switch to different MAP so send BU to previous MAP
    bu_entry* oldBULE =  mipv6cdsMN->findBU(curMapCopy.addr());
    //ipv6_addr oldRcoa =  mipv6cdsMN->findBU(mipv6cdsMN->primaryHA().lock()->addr());
    ipv6_addr oldRcoa = oldBULE?oldBULE->homeAddr():
      formRemoteCOA(curMapCopy.addr(), ifIndex);
    //We should have a binding to it still I think unless we went into a no map
    //zone for a long time or very short binding lifetime with MAP
    assert(oldBULE);
    InterfaceEntry *ie = ift->interfaceByPortNo(ifIndex);
    if (!ie->ipv6()->addrAssigned(oldRcoa))
      Dout(dc::warning, rt->nodeName()<<" oldRcoa "<<oldRcoa<<" was not assigned as never received BA from MAP.");

    Dout(dc::hmip, rt->nodeName()<<" forwarding from MAP="
         <<curMapCopy.addr()<<" to MAP="<<bestMap.addr()<<" oldrcoa="
         <<oldRcoa<<" rcoa="<<rcoa<<" lcoa="<<lcoa);

#if EDGEHANDOVER
    if (mob->edgeHandover()) //and previous map distance  = 1 then do pcoaf
    {
      //EH is brcoa  -> lcoa

      //However using lcoa means BAcks from HA to brcoa are lost as we move away
      //from the previous MAP/AR.
      EdgeHandover::EHCDSMobileNode* ehcds =
        boost::polymorphic_downcast<EdgeHandover::EHCDSMobileNode*>(mipv6cdsMN);
      assert(ehcds);
      if (ehcds->boundMapAddr() != IPv6_ADDR_UNSPECIFIED)
        assert(ehcds->mapEntries().count(ehcds->boundMapAddr()));

      ipv6_addr bcoa = ehcds->boundCoa();
      if (bcoa != IPv6_ADDR_UNSPECIFIED &&
          ehcds->boundMapAddr() != bestMap.addr() &&
          ehcds->mapEntries()[ehcds->boundMapAddr()].distance() == 1)
      {
//        Dout(dc::eh, rt->nodeName()<<" Forwarding from bmap="<<ehcds->boundMapAddr()<<" to lcoa="
//             <<lcoa);
        Dout(dc::eh, rt->nodeName()<<" forwarding from bmap="<<ehcds->boundMapAddr()<<" to nrcoa="
             <<rcoa);
        mstateMN->sendMapBU(ehcds->boundMapAddr(), rcoa, bcoa,
                            static_cast<unsigned int> (mipv6cdsMN->pcoaLifetime()) * 2,
                            ifIndex, mob);

        HMIPv6MAPEntry& bmap = ehcds->mapEntries()[ehcds->boundMapAddr()];

        //trigger on bound map so that we get this reverse tunnel lcoa->best MAP encapsulation
        tunMod->tunnelDestination(bmap.addr(), vIfIndex);
        
        //create nrcoa to bmap reverse tunnel and trigger on primary HA (bc bmap is really just best MAP in HMIP6)
        if (bmap.v())
        {
          //rcoa in this case is nrcoa
          vIfIndex = tunMod->findTunnel(rcoa, bmap.addr());
          //assert(!vIfIndex);
          //Sometimes old tunnel is not removed so it may have been created already when
          //we revisit past ARs
          if (!vIfIndex)
          {

            //assuming single mobile interface at 0
            vIfIndex = tunMod->createTunnel(rcoa, bmap.addr(), 0, mipv6cdsMN->primaryHA()->prefix().prefix);
            Dout(dc::eh|dc::encapsulation|dc::debug|flush_cf, rt->nodeName()
                 <<" (mapHandover) reverse tunnel created entry nrcoa="<<rcoa
                 <<" exit bmap="<< bmap.addr()<<" vIfIndex="<<hex<<vIfIndex<<dec
                 <<" V flag set so triggering on HA="<<mipv6cdsMN->primaryHA()->prefix().prefix);

          }
          else
          {
            Dout(dc::eh|dc::encapsulation, "(mapHandover) reverse tunnel exists already nrcoa="
                 <<rcoa<<" exit bmap="<< bmap.addr()<<" vIfIndex="<<hex<<vIfIndex<<dec);
            Dout(dc::hmip|dc::encapsulation, *tunMod);
            //trigger on bestmap in case not done
            tunMod->tunnelDestination(mipv6cdsMN->primaryHA()->prefix().prefix, vIfIndex);
          }
        }

      }
    }
#endif // EDGEHANDOVER

/* TODO make pcoaf an xml option
    //PCOAF lifetime?
    //DAD is not done here as addr is already in BUL

    //forwarding to rcoa is better than lcoa in case we move very fast that
    //way we have a chain of tunnels
    mstateMN->sendMapBU(curMapCopy.addr(), rcoa, oldRcoa,
                        //need a floor round down func
                        static_cast<unsigned int> (mipv6cdsMN->pcoaLifetime()),
                        ifIndex, mob);

    //create tunnel from lcoa to old map

*/
    //Destroy tunnel from old lcoa to old map except when forwarding from old/bound map to current coa
    if (oldBULE && curMapCopy.v() && oldBULE->careOfAddr() != rcoa)
      tunMod->destroyTunnel(oldBULE->careOfAddr(), curMapCopy.addr());
    //Destroy tunnel to HA from oldRcoa
    tunMod->destroyTunnel(oldRcoa, mipv6cdsMN->primaryHA()->prefix().prefix);

  }

}
/*
   - "Eager" to perform new bindings
      - "Lazy" in releasing existing bindings

   The above means that the MN should register with any "new" MAP
   advertised by the AR (Eager).
   The method by which the MN determines whether the MAP is a "new" MAP
   is described in chapter 5 above. The MN should not release existing
   bindings until it no longer receives the MAP option or the lifetime
   of its existing binding expires (Lazy).
   This Eager-Lazy approach described above will assist in providing a
   fallback mechanism in case one of the MAP routers crash as it would
   reduce the time it takes for a MN to inform its CNs and HA about its
   new COA.

*/
///Forms a remote coa from prefix of MAP me at the interface ifIndex
ipv6_addr HMIPv6NDStateHost::formRemoteCOA(const HMIPv6MAPEntry& me,
                                           unsigned int ifIndex)
{
  assert(me.addr() != IPv6_ADDR_UNSPECIFIED);

  InterfaceEntry *ie = ift->interfaceByPortNo(ifIndex);
  ipv6_addr rcoa = ipv6_addr_fromInterfaceToken(me.addr(), ie->interfaceToken());
  return rcoa;
}

/*

///Override base handover because we are trying to use HMIP always. Only when no
///map options exist do we use MIPv6. However HMIPv6NDStateHost::discoverMAP
///only hands over when map options have changed thus moving between ARs
///connected to same MAP will not trigger handover and requires
///MIPv6NDStateHost::movementDetectedCallback's RtrAdvMissedTmr to do so. That
///will calls us but how do we know it called us so we can forward it back to
///MIPv6NDStateHost::handover
HMIPv6NDStateHost::handover(boost::shared_ptr<MIPv6RouterEntry> newRtr)
{
  //If we don't have a mip6 router then we definitely have not received a MAP
  //option because if we did currentRouter would be set already.
  if (!mipv6cdsMN->currentRouter())
    MIPv6NDStateHost::handover(newRtr);


  //Set some flag to see if map options exist and if they do then do not forward
  //to MIPv6::Handover as we'll handover to map first if/when we are in
  //processRtrAd()
}

*/

/*
  @brief sends BU to map when handoff between ARs

  Called from sendBU of MIPv6NDStateHost base class. Forwards from PAR to NAR if
  ARs have HA func.
  Assumes lcoa is assigned already
  @callgraph
 */
bool HMIPv6NDStateHost::arhandover(const ipv6_addr& lcoa)
{

//instead of lcoa should really use this to get ifIndex too
//boost::shared_ptr<MobileIPv6::MIPv6RouterEntry> newRtr)

  //until we get the newRtr arg assume single iface
  unsigned int ifIndex = 0;
  
  //assert(newRtr.get());

  //  if (hmipv6cdsMN.isMAPValid() && newRtr)
  if (!rt->hmipSupport() || hmipv6cdsMN.remoteCareOfAddr() == IPv6_ADDR_UNSPECIFIED)
  {
    //Do base ipv6 handover
    return false;
  }

  //As long as currentMap exists we will assume handover between ARs in MAP
  //domain.  Only during reception of RtrAdv's MAP options can we tell if we
  //have changed map domain by missing current MAP option.  or no MAP options
  //advertised at all

  //AR to AR handover BU to currentMAP
  assert(mipv6cdsMN->primaryHA());

  //Make sure coa is already assigned i.e. we've seen the rtrAdv from newRtr
  //and processed it in Superclass::processRtrAd.
  InterfaceEntry *ie = ift->interfaceByPortNo(ifIndex);

  if (rt->odad())
    assert(ie->ipv6()->addrAssigned(lcoa)||ie->ipv6()->tentativeAddrAssigned(lcoa));
  else
    assert(ie->ipv6()->addrAssigned(lcoa));

  ipv6_addr oldcoa = hmipv6cdsMN.localCareOfAddr();
  assert(oldcoa != mipv6cdsMN->homeAddr());

    //There's no need to resend BU containing exactly the same information
  if (oldcoa != lcoa)
  {
      //Iterate through every BU_entry and send updates only to currentMAP and
      //TODO optionally CNs that are on same link as lcoa (same prefix)
//       for_each(mipv6cdsMN->bul.begin(), mipv6cdsMN->bul.end(),
//                sendMAPBUs(coa, rt->minValidLifetime(), mob, mipv6cdsMN));

    Dout(dc::hmip, rt->nodeName()<<" "<<nd->simTime()<<" arhandover register with MAP "
         <<hmipv6cdsMN.currentMap().addr()<<" lcoa="<<lcoa<<" rcoa="
         <<hmipv6cdsMN.remoteCareOfAddr()<<" lifetime="
         <<hmipv6cdsMN.currentMap().lifetime()<<" ifIndex="
         <<ifIndex);
//           <<newRtr->re.lock()->ifIndex());

    //lifetime should really be min(map.lifetime(), lifetime of lcoa)
    mstateMN->sendMapBU(hmipv6cdsMN.currentMap().addr(), lcoa,
                        hmipv6cdsMN.remoteCareOfAddr(),
                        //This may not be exact lifetime as DAD delay will have reduced this
                        hmipv6cdsMN.currentMap().lifetime(),
                        ifIndex,
                        //newRtr->re.lock()->ifIndex(),
                        mob);
    mob->lbuVector->record(nd->simTime());
/*
#if EDGEHANDOVER
    if (mob->edgeHandover()) //and previous map distance > 1 then do pcoaf otherwise we would be in mapHandover anyway.
    {
#endif // EDGEHANDOVER
//make pcoaf an xml option
    mstateMN->previousCoaForward(lcoa, oldcoa, mob);
#if EDGEHANDOVER
    }
#endif // EDGEHANDOVER

*/

    if (hmipv6cdsMN.currentMap().v())
    {
      //Set up reverse tunnelled link from LCOA to map here as we update lcoa and remove old one
      IPv6Encapsulation* tunMod = check_and_cast<IPv6Encapsulation*>
        (OPP_Global::findModuleByType(rt, "IPv6Encapsulation"));
      assert(tunMod != 0);

      tunMod->destroyTunnel(oldcoa, hmipv6cdsMN.currentMap().addr());

      size_t vIfIndex = tunMod->findTunnel(lcoa, hmipv6cdsMN.currentMap().addr());
      //assert(!vIfIndex);
      if (!vIfIndex)
      {
        //assuming single mobile interface at 0
        vIfIndex = tunMod->createTunnel(lcoa, hmipv6cdsMN.currentMap().addr(), 0, mipv6cdsMN->primaryHA()->prefix().prefix);

          Dout(dc::hmip|dc::encapsulation|dc::debug|flush_cf, rt->nodeName()<<" (arhandover) reverse tunnel created entry lcoa="
             <<lcoa<<" exit map="<< hmipv6cdsMN.currentMap().addr()<<" vIfIndex="<<hex<<vIfIndex<<dec
             <<" V flag set so triggering on HA="<<mipv6cdsMN->primaryHA()->prefix().prefix);
      }
      else
      {
        Dout(dc::hmip|dc::encapsulation, "(arhandover) reverse tunnel exists already lcoa="
             <<lcoa<<" exit map="<< hmipv6cdsMN.currentMap().addr()<<" vIfIndex="<<hex<<vIfIndex<<dec);
        Dout(dc::hmip|dc::encapsulation, *tunMod);
      }
    }
  }
  return true;
}

}  //namespace HierarchicalMIPv6
