//
// Copyright (C) 2001, 2002, 2004 CTIE, Monash University
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
 * @file MIPv6NDStateHost.cc
 * @author Johnny Lai
 * @date   14 Apr 2002
 *
 * @brief  Implementation of handover and ICMPv6 message processing
 *
 */


#include "sys.h"
#include "debug.h"


#include "MIPv6NDStateHost.h"
#include <boost/cast.hpp>
#include <iomanip> //setprecision
#include <boost/bind.hpp>
#include "cSignalMessage.h"
#include "cCallbackMessage.h"
#include "TimerConstants.h"
#include "RoutingTable6.h"
#include "NeighbourDiscovery.h"
#include "IPv6CDS.h"
#include "NDTimers.h"
#include "InterfaceTable.h"
#include "IPv6InterfaceData.h"
#include "MIPv6Entry.h"
#include "MIPv6ICMPv6NDMessage.h"
#include "MIPv6CDSMobileNode.h"
#include "MIPv6MNEntry.h"
#include "MIPv6DestOptMessages.h"
#include "MobilityHeaderBase.h"

//extern for movement detection messages types
#include "NeighbourDiscovery.h"
#include "IPv6Encapsulation.h" //callback registration
#include "opp_utils.h"

#include "IPv6Mobility.h"
#include "MIPv6MStateMobileNode.h"
#include "WirelessEtherModule.h" //l2 trigger set


#ifdef USE_HMIP
#include "HMIPv6CDSMobileNode.h"
#include "HMIPv6NDStateHost.h"
#include <string>
#include <memory>
#include <iostream>
using HierarchicalMIPv6::HMIPv6CDSMobileNode;
using HierarchicalMIPv6::HMIPv6NDStateHost;
#endif //USE_HMIP

#if EDGEHANDOVER
#include "EHCDSMobileNode.h"
#endif //EDGEHANDOVER

#include "IPv6Utils.h"

#if defined __GNUC__ && __GNUC__ < 3
template <typename T>
bool operator!=(const boost::weak_ptr<T>& lhs, const boost::weak_ptr<T>& rhs)
{
  return boost::operator!=(lhs, rhs);
}
#endif


namespace MobileIPv6
{

/**
 * @class RtrAdvMissedTmr
 *
 * @brief Timer message for tracking Missed RtrAdv for the current router
 * (forming coa from this)
 */

class RtrAdvMissedTmr: public cSignalMessage
{
public:
  typedef boost::function<void (cTimerMessage*)> MissedConsecCB;

  /**
   * @param allowCount is a reference so that when interface has changed its
   * parameters at runtime (it doesn't allow that now) this will obtain those
   * changes too.
   *
   * @param interval amount of time that elapses before the no. of consecutively
   * missed Rtr Adv is incremented
   *
   * @param missedConsecCB the callback to invoke when no. of consecutively
   * missed Rtr Adv has exceeded allowCount
   *
   * @param nodeName use to distinguish names of diff timers on per host basis
   */

  RtrAdvMissedTmr(const double& interval, const unsigned int& allowCount,
		  MissedConsecCB missedConsecCB,
                  const string& nodeName)
    :cSignalMessage(string(nodeName+"RtrAdvMissedTmr").c_str(), Tmr_RtrAdvMissed),
     consecCountMissed(0), allowCount(allowCount), interval(interval),
     cb(missedConsecCB)
    {
      connect(boost::bind(&RtrAdvMissedTmr::missedRtrAdv, this));
    }

  ~RtrAdvMissedTmr()
  {}

  /**
   * callback function exercised when interval seconds have passed without
   * receiving a rtrAdv from the currentRtr
   *
   */

  void missedRtrAdv();

  /**
   * called by processRtrAd whenever it receives a rtrAdv from the currentRtr
   * @param newInterval is the new interval although most likely there was no
   * change
   */

  void reset(double newInterval)
    {
      interval = newInterval;
      consecCountMissed = 0;
      //Cannot always assume that it is scheduled as we can enter a subnet where
      //no default routers are known yet and so missedRtrAdvTmr is obviously not
      //set
      if (isScheduled())
        cancel();
      rescheduleDelay(interval);
    }

  void resetCount()
    {
      consecCountMissed = 0;
    }

  unsigned int  maxMissed() const
    {
      return allowCount;
    }

private:
  unsigned int consecCountMissed;
  const unsigned int& allowCount;
  double interval;
  MissedConsecCB cb;
};

  /**
   * callback function exercised when interval seconds have passed without
   * receiving a rtrAdv from the currentRtr
   *
   */

  void RtrAdvMissedTmr::missedRtrAdv()
  {
    consecCountMissed++;

    Dout(dc::mip_missed_adv|flush_cf, name()<<" "<<module()->simTime()<<" consecutive misses="<<consecCountMissed
         <<" allowed misses="<<allowCount);

    //Reference does not work if maxConsecMissedRtrAdv is not exactly unsigned
    //int in Interface6Entry.mipv6Var!

    if (consecCountMissed > allowCount)
    {
      cb(this);
    }
    else
      rescheduleDelay(interval);
  }

/**
 * @todo provide a virtual set function for the cbrtrSolRetry so we don't
 * initialise it twice.
 *
 */

MIPv6NDStateHost::MIPv6NDStateHost(NeighbourDiscovery* mod)
  : NDStateHost(mod), mnRole(0), mipv6cdsMN(0), awayCheckDone(false), missedTmr(0), mob(0),
    mstateMN(0), schedSendUnsolNgbrAd(0),
    potentialNewInterval(0), ignoreInitiall2trigger(true)

{
#ifdef USE_MOBILITY
  //Yes very kludgy but we'll just stick with this for now
  //delete cbRetrySol;
  //cbRetrySol = makeCallback(this, &MIPv6NDStateHost::sendRtrSol);

  IPv6Encapsulation* tunMod = check_and_cast<IPv6Encapsulation*>
    (OPP_Global::findModuleByType(rt, "IPv6Encapsulation"));
  //Even though downcast detects incorrect downcasts it still allows casting 0
  //down to anything
  assert(tunMod != 0);

  tunMod->registerCB(boost::bind(&MIPv6NDStateHost::checkDecapsulation,this, _1));

  mob = check_and_cast<IPv6Mobility*>
    (OPP_Global::findModuleByType(rt, "IPv6Mobility"));
  assert(mob != 0);

  mstateMN = boost::polymorphic_downcast<MIPv6MStateMobileNode*>
    (mob->role);
  assert(mstateMN != 0);

  for (unsigned int i = 0; i < ift->numInterfaceGates(); i++)
  {
    InterfaceEntry *ie = ift->interfaceByPortNo(i);

    if (ie->_linkMod->className() == std::string("WirelessEtherModule"))
    {
      WirelessEtherModule* wlanMod =
        static_cast<WirelessEtherModule*>(ie->_linkMod);

      assert(wlanMod != 0);

      //We can make a subclass fo this type and add ifIndex as part of the
      //message so movementDetected knows which iface has changed.  Although
      //this is more useful for multihomed hosts for now just assume first iface
      //is mobile

      cCallbackMessage* cb = new cCallbackMessage("Tmr_L2Trigger", Tmr_L2Trigger);
      (*cb) = boost::bind(&MIPv6NDStateHost::movementDetectedCallback, this, cb);
      if ( wlanMod->linkUpTrigger() )
	wlanMod->setLayer2Trigger(cb);
    }
  }

  assert(rt->mipv6cds != 0);
  mnRole = boost::polymorphic_downcast<MIPv6MStateMobileNode*>(mob->role);
  mipv6cdsMN = rt->mipv6cds->mipv6cdsMN;

#endif //USE_MOBILITY

}

MIPv6NDStateHost::~MIPv6NDStateHost(void)
{
  if (missedTmr && !missedTmr->isScheduled())
    delete missedTmr;
}

/**
 *  Sec 10.5 handling of Router Advertisement from Home Agent
 *
 *
 * Rescheduling/creation of missedConsecutive RtrAdv movement det algorithm
 *
 * @warning TODO It may be possible that two routers on different subnets have
 * the same link local address (Our test scenarios avoid this) so this is not
 * addressed. Fix is to use global addr always.
 */

std::auto_ptr<RA> MIPv6NDStateHost::processRtrAd(std::auto_ptr<RA> rtrAdv)
{
  rtrAdv = IPv6NeighbourDiscovery::NDStateHost::processRtrAd(rtrAdv);

  if (rtrAdv.get() == 0)
    return rtrAdv;

  IPv6Datagram* dgram = check_and_cast<IPv6Datagram*>(rtrAdv->encapsulatedMsg());

  size_t ifIndex = dgram->inputPort();

  const ipv6_addr& ll_addr = dgram->srcAddress();

  boost::shared_ptr<MIPv6RouterEntry> bha = mipv6cdsMN->findRouter(ll_addr);
  MIPv6RouterEntry* ha = bha.get();

  bool isHA = rtrAdv->isHomeAgent();

  //After rereading 11.3.1 of draft 18 it appears that no HA flag is totally
  //ignored. (All subsequent bullet points refer to HA)
  //However for now will leave original assumption about existence of
  //MIPv6Router role also can't really remove isHA from here and
  //MIPv6RouterEntry and just make it a list of HA because we'd still need to
  //maintain a copy of currentRouter for move detect(missedRtrAdv)/handover purposes
//   if (!isHA)
//   {
//     if (bha && bha->isHomeAgent())
//       mipv6cdsMN->removeHomeAgent(bha);
//     return rtrAdv;
//   }
  if (!isHA && bha && bha->isHomeAgent())
  {
    mipv6cdsMN->removeHomeAgent(bha);
    return rtrAdv;
  }

  int preference = 0;
  unsigned long lifetime = rtrAdv->routerLifetime();

  MIPv6ICMPv6NDMRtrAd* mipv6rtrAdv = 0;

  if (isHA)
    mipv6rtrAdv = check_and_cast<MIPv6ICMPv6NDMRtrAd*> (rtrAdv.get());

  if (mipv6rtrAdv != 0 && mipv6rtrAdv->hasHomeAgentInfo())
  {
    preference = mipv6rtrAdv->homeAgentInfo().haPref;
    lifetime = mipv6rtrAdv->homeAgentInfo().haLifetime;
  }

  if (lifetime == 0 && ha != 0)
  {
    //Regardless of whether it is HA or not if it has lifetime (ha or rtr
    //lifetime of 0 we'll remove)
    mipv6cdsMN->removeHomeAgent(bha);
    rtrAdv.reset();
    return rtrAdv;
  }

  if (ha != 0)
  {
    assert(ha->isHomeAgent() == rtrAdv->isHomeAgent());
    ha->setPreference(preference);
    ha->setLifetime(lifetime);
  }

  bool haCreated = false;
  if (ha == 0 && lifetime > 0 && rtrAdv->prefixCount() > 0)
  {

    ha = new MIPv6RouterEntry(rt->cds->router(ll_addr), isHA,
                              ipv6_prefix(IPv6_ADDR_UNSPECIFIED, 128), lifetime,
                              preference);
    haCreated = true;
  }
  else if (lifetime == 0 || rtrAdv->prefixCount() == 0)
  {
    return rtrAdv;
  }

  assert(ha != 0);

  LinkPrefixes prefixes =  rt->cds->getPrefixesByIndex(ifIndex);
  bool globalFound = false;
  for (size_t i = 0; i < rtrAdv->prefixCount(); i++)
  {

    const ICMPv6NDOptPrefix& pref = rtrAdv->prefixInfo(i);
    PrefixEntry* pe = 0;
    if ( (pe = rt->cds->getPrefixEntry(IPv6Address(pref.prefix, pref.prefixLen))) == 0)
      continue;

    //Add any prefixes that are in PrefixList that came from this rtr adv. for
    //later removal when moving to different subnet
    ha->addOnLinkPrefix(pe);

    if (pe->advRtrAddr() && !globalFound)
    {
      globalFound = true;

      Dout(dc::mipv6|dc::router_disc, rt->nodeName()<<":"<<ifIndex<<" "<<setprecision(6)
           <<nd->simTime()<<" global router address found "<<pref.prefix<<"/"
           <<(int)pref.prefixLen);
      ha->setPrefix(ipv6_prefix(pref.prefix, pref.prefixLen));
      (*rt->cds)[IPv6Address(ha->prefix().prefix, 128)].neighbour = ha->re;
    }
    else if (pe->advRtrAddr() && globalFound)
    {
      cerr<<"Don't think router should have two global addresses advertised as "
	  <<"we do not assign both to the same entry"<<endl;
      assert(false);
    }
  }

  if (!globalFound)
  {
    Dout(dc::mipv6|dc::router_disc|flush_cf,  nd->rt->nodeName()<<":"<<ifIndex<<" "
         <<setprecision(4)<<nd->simTime()<< " rtrAdv from "<<dgram->srcAddress()
         <<" but no prefix adv. with R bit");
    delete ha;

    //Todo what do we do if we've moved to a new subnet without MIPv6 Router
    return rtrAdv;
  }

  assert((haCreated && ha != 0) || (!haCreated && ha != 0));
  if (haCreated)
  {
    //possible when it is the static HA since statc HA is given a ll_addr of
    //global HA address
    boost::shared_ptr<MIPv6RouterEntry> sha = mipv6cdsMN->findRouter(ha->addr());
    if (sha.get() != 0)
    {
      if (sha == mipv6cdsMN->primaryHA())
      {
	//mipv6cdsMN->removeHomeAgent(mipv6cdsMN->primaryHA());
	rt->cds->removeRouterEntry(sha->re.lock().get()->addr());
	sha->re = rt->cds->router(ll_addr);
	delete ha;
	ha = sha.get();
	Dout(dc::notice|flush_cf, nd->rt->nodeName()<<":"<<ifIndex<<" "
	     <<setprecision(6)<<nd->simTime()<<" hack to fix static HA CDS entry ");
	//Need line below as sending packet to HA at global addr when at home
	//will cause unnecessary addr res and creation of NE as host. This is
	//due to prefix causing a match to on link host which needs to be
	//resolved.
	(*rt->cds)[IPv6Address(sha->prefix().prefix, 128)].neighbour = sha->re;
	haCreated = false;
      }
      else
	assert(false);
    }
    if (haCreated)
      mipv6cdsMN->insertHomeAgent(ha);
    bha = mipv6cdsMN->findRouter(ll_addr);
    if (haCreated)
      Dout(dc::router_disc|flush_cf,  nd->rt->nodeName()<<":"<<ifIndex<<" "
	   <<setprecision(6)<<nd->simTime()<<" Mipv6 router "<<ha->prefix()
	   <<" added to list ");
  }
  if (!haCreated)
  {

    ///@warning This is a real hack. Currently I detect new routers by looking
    ///into the MRL in mipv6cdsMN. However only new routers/HAs get inserted
    ///into there so if we ever return home or go back to a previous router
    ///that we've visited these are not added to the list. When movement is
    ///detected the MN thinks that there have been no router advs yet from the
    ///default router visited subnet.


    //My "fix" is to make sure that routers are added to this list even if
    //they are in the list already as long as it is not another router on the
    //current subnet(well we do not know if we are on new subnet now).  This
    //is done by removing the entry and adding it back. Perhaps we should
    //have a separate previous routers list and mrl so we do not need these
    //botches. Nevertheless the previous routers list would still need some
    //pruning otherwise the movement of MN is tracked.
    if (mipv6cdsMN->currentRouter().get() != ha &&
        (*mipv6cdsMN->mrl.rbegin())->prefix() != ha->prefix())
    {
      Dout(dc::mipv6, rt->nodeName()<<" Fixing mrl order for revisited router "<<ha->addr());
      mipv6cdsMN->mrl.remove(bha);
      mipv6cdsMN->mrl.push_back(bha);
    }
  }

  assert(ha == bha.get());

  if (!mipv6cdsMN->primaryHA() && !awayFromHome() && isHA)
  {
    mipv6cdsMN->primaryHA() = bha;
    bool primaryHoa = true;
    mipv6cdsMN->formHomeAddress(mipv6cdsMN->primaryHA(), 
				ift->interfaceByPortNo(ifIndex), primaryHoa);

    Dout(dc::mipv6|dc::mobile_move, rt->nodeName()<<" Primary HA registered. Addr="<<bha->addr());
  }


  if (!mipv6cdsMN->currentRouter())
  {
    if (rt->cds->defaultRouter().lock()->state() == NeighbourEntry::INCOMPLETE
        && mipv6cdsMN->primaryHA()->re.lock() == rt->cds->defaultRouter().lock()
        && rt->cds->routerCount() == 2 && bha->re.lock() != rt->cds->defaultRouter().lock())
    {
      //should check on separate variable i.e. mobility's par(homeAgent) != IPv6_ADDR_UNSPECIFIED
      Dout(dc::mipv6, " hack to get starting away from home working");
      rt->cds->setDefaultRouter(bha->re);
    }
    if ((mipv6cdsMN->movementDetected() || mipv6cdsMN->eagerHandover()) &&
        mipv6cdsMN->primaryHA())
    {
#ifdef USE_HMIP
      if (rt->hmipSupport() && rtrAdv->hasMapOptions())
        Dout(dc::hmip, rt->nodeName()<<" Detected map options in MIPv6NDStateHost::processRtrAdv deferring handover(archaic branch) ");
      else
#endif //USE_HMIP	
	handover(bha);
    }
    else
      mipv6cdsMN->currentRouter() = bha;
  }
  else
  {
  //Archaic movement detection check (until we add movement detection hooks like
  //missedRtrAdv, L2 trigger, addrRes hook/NUD) i.e. if current router not
  //reachable and this router is a new one.
    if (mipv6cdsMN->primaryHA() &&
#if defined __GNUC__ && __GNUC__ < 3
      //gcc 2.96 requires this for certain build configurations otherwise get
      //ambigous operator (no way out really)
      boost::operator!=(mipv6cdsMN->currentRouter(), bha) &&
#else
      mipv6cdsMN->currentRouter()!= bha &&
#endif //defined __GNUC__ && __GNUC__ < 3

        //NUD required for these conditions to activate
        ((mipv6cdsMN->currentRouter()->re.lock()->state() != NeighbourEntry::STALE &&
          mipv6cdsMN->currentRouter()->re.lock()->state() != NeighbourEntry::REACHABLE) ||
        //addr res failure means router is removed from DC (This doesn't work in
        //practice because if there is any traffic from upper layers they will
        //force routing6core's conceptualSending to reuse previous default
        //router even if its unreachable in the hopes of doing addr res on it
        //again. (Would this slow down handover to new default router?)
        rt->cds->neighbour(mipv6cdsMN->currentRouter()->re.lock()->addr()).lock().get() == 0||
         mipv6cdsMN->movementDetected() ||
         mipv6cdsMN->eagerHandover()))
    {
      if (rt->cds->neighbour(mipv6cdsMN->currentRouter()->re.lock()->addr()).lock().get() == 0)
	Dout(dc::debug|flush_cf, rt->nodeName()
           <<"Archaic movement detection triggered by currentRouter not reachable in AR");
      else if (mipv6cdsMN->movementDetected())
	Dout(dc::debug|flush_cf, rt->nodeName()
           <<"Archaic movement detection triggered by movement Detected");
      else if (mipv6cdsMN->eagerHandover())
	Dout(dc::debug|flush_cf, rt->nodeName()
           <<"Archaic movement detection triggered by eagerHandover");
      else
	Dout(dc::debug|flush_cf, rt->nodeName()
	     <<"Archaic movement detection prob from NUD triggered handover?");

#ifdef USE_HMIP
      if (rt->hmipSupport() && rtrAdv->hasMapOptions())
        Dout(dc::hmip, rt->nodeName()<<" Detected map options in MIPv6NDStateHost::processRtrAdv deferring handover(archaic branch) ");
      else
#endif //USE_HMIP
	handover(bha);
    }
  }

  ///Missed Rtr Advertisement timer is rescheduled or created only if rtrAdv
  ///comes from the currentRouter
  unsigned int interval = rtrAdv->advInterval();
  if (interval != 0) //option exists
  {
    double newInterval = interval/1000.0;
    if (missedTmr == 0 && mipv6cdsMN->currentRouter() == bha)
    {
      InterfaceEntry *ie = ift->interfaceByPortNo(ifIndex);
      missedTmr = 
	new RtrAdvMissedTmr(newInterval,
			    ie->ipv6()->mipv6Var.maxConsecutiveMissedRtrAdv,
			    boost::bind(&MIPv6NDStateHost::movementDetectedCallback, this, _1),
			    rt->nodeName());

      newInterval = nd->simTime() + newInterval;
      nd->scheduleAt(newInterval , missedTmr);
      Dout(dc::mip_missed_adv|flush_cf, rt->nodeName()<<":"<<ifIndex
           <<" timer created at "<<setprecision(4)<<nd->simTime()
           <<" interval="<<newInterval<<" advfrom="<<dgram->srcAddress()
           <<" curRtr="<<mipv6cdsMN->currentRouter()->addr());

    }
    else if (mipv6cdsMN->currentRouter() == bha)
    {
      missedTmr->reset(newInterval);
      Dout(dc::mip_missed_adv|flush_cf, rt->nodeName()<<":"<<ifIndex
           <<" timer reset at "<<setprecision(4)<<nd->simTime()
           <<" interval="<<newInterval<<" advfrom="<<dgram->srcAddress()
           <<" curRtr="<<mipv6cdsMN->currentRouter()->addr());

    }
    else
    {
      potentialNewInterval = newInterval;
      Dout(dc::mip_missed_adv|flush_cf, rt->nodeName()<<":"<<ifIndex<<" "<<rt->simTime()
           <<" potentialNewInterval="<<newInterval<<" advfrom="
           <<dgram->srcAddress()<<" curRtr="<<
           (mipv6cdsMN->currentRouter()?mipv6cdsMN->currentRouter()->addr():bha->addr()));
    }

  }
  else
  {
    // what happens to movement detection if interval option doesn't exist
    // should we use max-rtrAdv-interval as the timing interval?  What about
    // first rtrAdv contains interval but subsequent ones do not
    Dout(dc::mip_missed_adv|flush_cf, rt->nodeName()<<":"<<ifIndex<<" "<<rt->simTime()
         <<" missing Interval option advfrom="<<dgram->srcAddress()
         <<" curRtr="<<(mipv6cdsMN->currentRouter()?mipv6cdsMN->currentRouter()->addr():bha->addr()));
    potentialNewInterval = 0;
    if (missedTmr->isScheduled())
      missedTmr->cancel();
  }


  //According to spec it looks like the coa and defaultRouter is only changed
  //when movement is detected.  Thus will do handover in
  //movementDetectedCallback

  return rtrAdv;
}

/**
 * Determination of whether we are away from home or at home during
 * system initialisation (when 1st Rtr Adv. received).
 *
 * @note The value is cached subsequently and only when we return home again or
 * handover do we actually set it again.
 */

bool MIPv6NDStateHost::awayFromHome()
{
//Setting the awayFromHome variable was too dodgy should simply just do the
//check of onlink prefix and won't have trouble determing when to set it. Just
//deprecate setAwayfromhome too (not caching the variable either as it can lead
//to problems)

  //primaryHA check needed to stop movementDetected from setting awayFromHome if
  //MN starts from many subnets away from home

  if (awayCheckDone &&  mipv6cdsMN->primaryHA())
     return mipv6cdsMN->awayFromHome();

  awayCheckDone = true;

//  boost::weak_ptr<MIPv6RouterEntry> ha = mipv6cdsMN->primaryHA();

//   if (ha.lock().get() != 0)
//   {
//     ///For cases where we are using HA as the default router which should be
//     ///true most of the time unless there is more than 1 router on the network
//     if (rt->cds->defaultRouter() == ha->re)
//       return false;

//     if (mipv6cdsMN->currentRouter().lock().get() == 0)
//       return false;

// #if 0  //Wait until NUD is done before using this
//     //Default router's global addr not home subnet prefix ||

//     //Home agent not reachable via neighbour soln then we are away from home.
//     //Try to use NUD indication (note NUD is unimplemented at this time so PROBE
//     //and DELAY are never set) Anyway NUD doesn't contain an explicit
//     //unavailable state for probes that fail (should we add one?)
//     if (ha->re->state() == NeighbourEntry::STALE ||
//         ha->re->state() == NeighbourEntry::REACHABLE)
//     {
// #if defined TESTMIPv6 || defined MOVEMENT_DETECTION
//       cout<<rt->nodeName()<<" at home according to NUD states\n";
// #endif //defined TESTMIPv6 || defined MOVEMENT_DETECTION
//       return false;
//     }
//     else
//     {
// #if defined TESTMIPv6 || defined MOVEMENT_DETECTION
//       cout<<rt->nodeName()<<" away from home according to NUD\n";
// #endif //defined TESTMIPv6 || defined MOVEMENT_DETECTION
//       return true;
//     }
// #endif //0
//   }

//   if (mipv6cdsMN->currentRouter().lock().get() != 0
//       //std::rel_ops really is not such a good idea after all.  From now on
//       //define our own operator!= for UDTs
//       && mipv6cdsMN->currentRouter() != mipv6cdsMN->primaryHA())
//     return true;

  //on link prefix list contains home subnet prefix
  const ipv6_prefix& homePref = mipv6cdsMN->homePrefix();

  ///since we don't know what our home network is assume we are at home
  if (homePref.prefix == IPv6_ADDR_UNSPECIFIED)
  {
    Dout(dc::mipv6|dc::mobile_move, rt->nodeName()<<" assuming at home default as no home address configured");
    mipv6cdsMN->setAwayFromHome(false);
    return false;
  }

  //on link prefix list contains home subnet prefix
  //Most effective for case of starting up away from home
  unsigned int ifIndex = 0;
  if (rt->cds->lookupAddress(homePref.prefix, ifIndex))
  {
    Dout(dc::mipv6|dc::mobile_move, rt->nodeName()<<" is at home.  Home Prefix="<<homePref<<" is on link");
    mipv6cdsMN->setAwayFromHome(false);
    return false;
  }

  mipv6cdsMN->setAwayFromHome(true);
  return true;
}

void MIPv6NDStateHost::initiateSendRtrSol(unsigned int ifIndex)
{
  //Don't want to start another rtrSol retransmission unless we're sure there
  //are no outstanding ones.
  for (TMI it = timerMsgs.begin(); it != timerMsgs.end(); it++)
    if ((*it)->kind() == Tmr_RtrSolTimeout &&
        ((NDTimer*)((*it)->contextPointer()))->ifIndex == ifIndex)
    {
      return;
    }

  //reset this back to unsolicited as we believe we may be on new subnet
  rtrSolicited[ifIndex] = false;
  sendRtrSol(0, ifIndex);
}

void MIPv6NDStateHost::movementDetectedCallback(cTimerMessage* tmr)
{
  assert(tmr != 0);

  if (mipv6cdsMN->currentRouter())
    mipv6cdsMN->previousDefaultRouter() = mipv6cdsMN->currentRouter();

  if (ift->numInterfaceGates() == 0)
    return;

  ///check if interface 0 is at least initialised totally
  if (!ifStats[0].initStarted || !globalAddrAssigned(0))
    return;

  if (tmr->kind() == Tmr_RtrAdvMissed)
  {
    mipv6cdsMN->setMovementDetected(true);
    mipv6cdsMN->setAwayFromHome(true);
    Dout(dc::mipv6|dc::mip_missed_adv|dc::mobile_move, rt->nodeName()<<" "<<setprecision(6)
         <<nd->simTime()<<" exceeded MaxConsecMissedRtrAdv of "
        <<(mipv6cdsMN->currentRouter()?
           ift->interfaceByPortNo(mipv6cdsMN->currentRouter()->re.lock()->ifIndex())->ipv6()->mipv6Var.maxConsecutiveMissedRtrAdv
           :check_and_cast<RtrAdvMissedTmr*>(tmr)->maxMissed()));

    std::ostream& os = IPv6Utils::printRoutingInfo(false, 0, "", false);
    os << rt->nodeName()<<" "<<setprecision(6)<<nd->simTime()
       <<" movedet: exceeded MaxConsecMissedRtrAdv of "
       <<(mipv6cdsMN->currentRouter()?
	  ift->interfaceByPortNo(mipv6cdsMN->currentRouter()->re.lock()->ifIndex())->ipv6()->mipv6Var.maxConsecutiveMissedRtrAdv
	  :check_and_cast<RtrAdvMissedTmr*>(tmr)->maxMissed())<<"\n";
    //Can also do NUD to current Rtr to confirm unreachability (once that's
    //implemented (although handover() to a null router will also do that Not
    //done though according to MIPL kernel and
    //\cite{nakajima03:handof_delay_analy_measur_sip_ipv6}

  }
  else if (tmr->kind() == Tmr_L2Trigger)
  {
    /*    if (ignoreInitiall2trigger)
    {
      ignoreInitiall2trigger = false;
      return;
    }
    else
    */
      mipv6cdsMN->setAwayFromHome(true);

    mipv6cdsMN->setMovementDetected(true);

    std::ostream& os = IPv6Utils::printRoutingInfo(false, 0, "", false);
    os <<rt->nodeName() <<" "<<setprecision(6)
       <<nd->simTime() <<" movedet: L2 connect Triggerred \n";
    Dout(dc::mipv6|dc::mobile_move, rt->nodeName()<<" "<<nd->simTime()<<" L2 connect Triggerred");

    //Retrieve ifIndex of triggered interface


  }
  else //default
  {
    DoutFatal(dc::core|flush_cf, rt->nodeName()<<" Unknown type received in movementDetectedCallback"<<tmr->kind());
    mipv6cdsMN->setMovementDetected(false);
    return;
  }
  //Guess we could just do it for the mobile interface i.e. the one with the
  //primaryHA but this should be more correct in theory.  Anyway most mobile
  //nodes have only 1 interface active
  for(size_t ifIndex = 0; ifIndex < ift->numInterfaceGates(); ifIndex++)
  {
    initiateSendRtrSol(ifIndex);
    Dout(dc::mipv6|dc::mobile_move|dc::router_disc, rt->nodeName()<<" Rtr Sol sent on ifIndex="<<ifIndex
	 <<" in response to movement detected");
  }

  if (tmr->kind() != Tmr_RtrAdvMissed)
  {
    //not scheduled when MN moves through no coverage zones
    if (missedTmr && missedTmr->isScheduled())
      missedTmr->cancel();
    else if (!missedTmr)
    {
      //cout<<"Where is missedTmr "<<nd->simTime()<<" tmr "<<tmr<<endl;
    }
  }

  if (mipv6cdsMN->currentRouter())
  {
    MIPv6CDSMobileNode::MRLI it;
    Dout(dc::mobile_move|dc::mipv6|dc::router_disc|flush_cf,  nd->rt->nodeName()<<" "
         <<setprecision(6)<<nd->simTime()<<" "
         <<(mipv6cdsMN->findRouter(mipv6cdsMN->currentRouter()->re.lock()->addr(), it)?
            (++it != mipv6cdsMN->mrl.end()?
             (mipv6cdsMN->currentRouter()->prefix() != (*mipv6cdsMN->mrl.rbegin())->prefix()?
              "next router exists and has different prefix"
              :" next Router exists but same prefix")
             :" no next router after current router"):" Can't find current router!!")
         );

    std::ostream& os = IPv6Utils::printRoutingInfo(false, 0, "", false);
    os <<rt->nodeName() <<" "<<setprecision(6)<<nd->simTime()
       <<(mipv6cdsMN->findRouter(mipv6cdsMN->currentRouter()->re.lock()->addr(), it)?
       (++it != mipv6cdsMN->mrl.end()?
	(mipv6cdsMN->currentRouter()->prefix() != (*mipv6cdsMN->mrl.rbegin())->prefix()?
	 " next router exists and has different prefix"
	 :" next Router exists but same prefix")
	:" no next router after current router"):" Can't find current router!!")
       <<"\n";

    if (mipv6cdsMN->findRouter(mipv6cdsMN->currentRouter()->re.lock()->addr(), it) &&
        (++it != mipv6cdsMN->mrl.end()) &&
        //Do a simple check to make sure this next router is not another router
        //also on previous subnet by using onlink global prefix
        (mipv6cdsMN->currentRouter()->prefix() != (*mipv6cdsMN->mrl.rbegin())->prefix()))
    {
      Dout(dc::mipv6|dc::mobile_move|flush_cf, rt->nodeName()
           <<" Detected movement and received rtrAdv from new router already");

      //We'll let the latest MIPv6 Router be the current Router let handover
      //take care of returning home case too.  processRtrAdv can detect that
      //we are returning home firsthand (although in handover is also
      //possible subsequently when home prefix is in the onlink prefix list)

      //Use freshest router instead of just next one?
      //handover(*it);
      handover((*mipv6cdsMN->mrl.rbegin()));

      //We've already done the handover i.e. we assume that rtr adv. came in as
      //there is a new router with diff global subnet prefix
      mipv6cdsMN->setMovementDetected(false);
    }
    //This case occurs only when we detect movement before a new rtradv received
    else
    {

      Dout(dc::mobile_move|dc::mipv6|flush_cf, rt->nodeName()
           <<" Moved to foreign subnet and  have not received rtrAds yet. curRtr.prefix="
           <<mipv6cdsMN->currentRouter()->prefix()<<" mrl.prefix="<<(*mipv6cdsMN->mrl.rbegin())->prefix());

      std::ostream& os = IPv6Utils::printRoutingInfo(false, 0, "", false);
      os <<rt->nodeName()<<" "<<setprecision(6)<<nd->simTime()
	 <<" movedet: Moved to foreign subnet and  have not received rtrAds yet. curRtr.prefix="
	 <<mipv6cdsMN->currentRouter()->prefix()<<" mrl.prefix="<<(*mipv6cdsMN->mrl.rbegin())->prefix()<<"\n";

      relinquishRouter(mipv6cdsMN->currentRouter(), boost::shared_ptr<MIPv6RouterEntry>());
    }

    if (!mipv6cdsMN->primaryHA())
    {
      //Initiate DHAAD to find primary homeAgent or continue looking
      Dout(dc::mipv6|dc::notice|dc::mobile_move, rt->nodeName()<<" Initiating/Retrying DHAAD as no Home Agent registered (unimplemented)");
    }

  }
  else
  {
    assert(!mipv6cdsMN->currentRouter());
    if (!mipv6cdsMN->primaryHA())
    {
      //This may only be possible if we move through subnets without any global
      //routers or we start from subnet without any global routers.
      Dout(dc::notice|dc::mipv6|dc::mobile_move|flush_cf, rt->nodeName()<<" No global routers initially(unhandled)");

    }
    else
    {
        Dout(dc::notice|dc::mipv6|dc::mobile_move|flush_cf, rt->nodeName()<<" Moved to another "
             <<"subnet and still waiting for new currentRouter");

    }
  }


  //add L2 trigger for current Router MAC addr (10.4 pg 111 para 2 and para 4
  //promiscuous mode).  That way if its received set Router NUD state to
  //Reachable (well its not really forward indication only indication from
  //Router to us).  Alternatively set movement detected to false

  //Add addrRes fails for router as movement detection too
}


/**
 * Tests for validity as a home addr
 *
 * @todo handle multicast addresses too since no thought has gone into
 * MIPv6 multicast handling thus far
 *
 */

bool home_addr_scope_check(const ipv6_addr& addr)
{
  //Remove the addr too that do not have global scope i.e. link local or
  //site local except for the autoconfigured address at position 0
  if (ipv6_addr_scope(addr) == ipv6_addr::Scope_Global ||
      //Site scope is configurable i.e. HA may or may not tunnel to us see 9.4
      //for now we'll assume always tunnelled to us so keep them
      ipv6_addr_scope(addr) == ipv6_addr::Scope_Site)
  {
    return true;
  }
  return false;
}


/**
 * @brief Handover to newRtr
 *
 * returning home case check
 */

void MIPv6NDStateHost::handover(boost::shared_ptr<MIPv6RouterEntry> newRtr)
{
  assert(mipv6cdsMN->currentRouter() != newRtr);

  boost::shared_ptr<MIPv6RouterEntry> oldRtr = mipv6cdsMN->currentRouter();

  mipv6cdsMN->setAwayFromHome(true);

  //normal handover requires sending BU to pHA. (if we don't have pHA) just
  //handover to new subnet and remove old currentRouter)

  if (newRtr)
  {
    Dout(dc::debug|flush_cf, rt->nodeName()<<" handover - new router global is "<<newRtr->prefix()
         <<" link is "<<newRtr->re.lock()->addr());

    unsigned int ifIndex = newRtr->re.lock()->ifIndex();
    //Check for return home case i.e. if pha is now on link or newRtr == pha or
    //pha reachable direct
    if (mipv6cdsMN->primaryHA() == newRtr)
    {
      if (!mipv6cdsMN->bulEmpty())
      {
        returnHome();
      }
      else
        if (!mipv6cdsMN->eagerHandover())
        Dout(dc::mipv6, rt->nodeName()<<" "<<nd->simTime()<<" went back home and"
             <<" no BUL entries so must have never moved i.e. false movement"
             <<" detection from missedRtrAdv not sending BU to HA at all");
        else
        {
          Dout(dc::mipv6, rt->nodeName()<<" "<<nd->simTime()<<" eager handover triggers handover "
               <<" when no router to a router. At initialisation for no static HA case, this"
               "happens to first router we use as HA, so we used to try sending a BU to it with coa =="
               "hoa) fix bug by setting awayFromHome false)");
          mipv6cdsMN->setAwayFromHome(false);
        };
    }
    else if (mipv6cdsMN->primaryHA())
    {
      InterfaceEntry *ie = ift->interfaceByPortNo(ifIndex);
      ///Form current care of addr regardless of what type of handover
      ipv6_addr coa = mipv6cdsMN->formCareOfAddress(newRtr, ie);
      cout << "At " << nd->simTime() << " sec, "<< rt->nodeName() << endl;

      //Make sure coa is already assigned i.e. we've seen the rtrAdv from newRtr
      //and processed it in processRtrAd.
      bool assigned = false;
      if (rt->odad())
      {
        assert(ie->ipv6()->addrAssigned(coa)||ie->ipv6()->tentativeAddrAssigned(coa));
        if (ie->ipv6()->addrAssigned(coa)||ie->ipv6()->tentativeAddrAssigned(coa))
          assigned = true;
      }
      else if (ie->ipv6()->addrAssigned(coa))
        assigned = true;

      if (assigned)
      {
	//newRtr could be passed in as some places may need it but then DAD dup
	//success bit is missing this. Convert this to a callback too?
	sendBU(coa);
      }
      else
      {
	deferSendBU(coa, ifIndex);
      }
    } //if primary HA exists
    else
    {
      //we'll remove oldRtr  at end of func if no primaryHA.
    }

    mipv6cdsMN->setMovementDetected(false);
  } //if newRtr exists

  relinquishRouter(oldRtr, newRtr);

}

void MIPv6NDStateHost::deferSendBU(ipv6_addr& coa, unsigned int ifIndex)
{
    //Not assigned at all yet unless DAD has finished (it would be in
    //tentativeAddr in this case)
    Dout(dc::mipv6|flush_cf, rt->nodeName()<<" "<<nd->simTime()
	 <<ifIndex<<" waiting for dad completion before sending BU for coa "
	 <<coa);
    cCallbackMessage* cb = new cCallbackMessage("MIPv6sendBU", 5490);
    (*cb) = boost::bind(&MIPv6NDStateHost::sendBU, this, coa);
    addCallbackToAddress(coa, cb);
}

/**
 * @brief Remove oldRtr's on link prefixes as we are on newRtr now and update
 * all destEntries that point to oldRtr to newRtr
 *
 * @param oldRtr is the previous link's default (AR) router

 * @param newRtr is the new default router that we have moved towards or can be
 * null if no current router
 * @note sets default router for forwarding purposes too if newRtr exists
 */

void MIPv6NDStateHost::relinquishRouter(boost::shared_ptr<MIPv6RouterEntry> oldRtr,
                                        boost::shared_ptr<MIPv6RouterEntry> newRtr)
{
  ///Set default router for forwarding purposes etc.
  if (newRtr)
    rt->cds->setDefaultRouter(newRtr->re);

  Dout(dc::mipv6|flush_cf, __FUNCTION__<<" oldRtr="<<(oldRtr?ipv6_addr_toString(oldRtr->addr()):"none")
       <<" newRtr="<<(newRtr?ipv6_addr_toString(newRtr->addr()):"none"));

  mipv6cdsMN->currentRouter() = newRtr;

  assert(newRtr != oldRtr);

  //assert((potentialNewInterval != 0 && newRtr.lock().get() != 0) || potentialNewInterval ==0);

  if (missedTmr && missedTmr->isScheduled())
    missedTmr->cancel();

  //We can't really have a missed timer if there are no routers in current
  //subnet
  if (!newRtr)
    potentialNewInterval = 0;

  if (missedTmr)
  {
    if (potentialNewInterval != 0)
      missedTmr->rescheduleDelay(potentialNewInterval);

    missedTmr->resetCount();
  }

  Dout(dc::mip_missed_adv|flush_cf, rt->nodeName()
       <<" missedAdvTimer reset as moved to new subnet potentialNewInterval="
       <<potentialNewInterval);

  removeAllCallbacks();

  if (!oldRtr)
  {
    if (newRtr && mipv6cdsMN->previousDefaultRouter() && newRtr != mipv6cdsMN->previousDefaultRouter())
      rt->cds->updateDestEntryByNeighbour(mipv6cdsMN->previousDefaultRouter()->re.lock()->addr(), newRtr->re.lock()->addr());

    return;
  }

  ///Critical to remove the oldRtr's DE otherwise we update the oldRtr's DE and
  ///say that the next hop is newRtr every time we handover and eventually when
  ///we return to previously visited subnet the default routers have another
  ///router as nexthop!!
  rt->cds->removeDestinationEntry(oldRtr->re.lock()->addr());

  oldRtr->re.lock()->setState(IPv6NeighbourDiscovery::NeighbourEntry::INCOMPLETE);

  unsigned int oldIfIndex = oldRtr->re.lock()->ifIndex();
  InterfaceEntry *oie = ift->interfaceByPortNo(oldIfIndex);
  //Remove old on link prefixes from On Link prefix list
  for (MIPv6RouterEntry::OPLI it = oldRtr->prefixes.begin(); it != oldRtr->prefixes.end(); it++)
  {

    //assuming that rcoas not formed from RA's prefixes so are not removed here
      IPv6Address addr = oie->ipv6()->matchPrefix((*it).prefix, (*it).length);
      if (addr != IPv6_ADDR_UNSPECIFIED && addr != mipv6cdsMN->homeAddr())
      {
        Dout(dc::mipv6, rt->nodeName()<<__FUNCTION__
             <<":  onlink local addresss "<<addr<<" removed");
        rt->removeAddress(addr, oldIfIndex);
	delete addressCallback(addr);
      }
      else
      {
	addr = oie->ipv6()->matchPrefix((*it).prefix, (*it).length, true);
	if (addr != IPv6_ADDR_UNSPECIFIED && addr != mipv6cdsMN->homeAddr())
	  //Must be tentative addrs
	  DoutFatal(dc::core|error_cf, "Unimplemented func for removing tentative addr when node moves "
		    <<" too fast as dad timer has to be removed cleanly see "
		    <<" checkDupAddrDetected and separate the bits there into separate func");
      }

    Dout(dc::prefix_timer|flush_cf, rt->nodeName()<<" removing on link prefix="
         <<*it<<" due to movement");
    //cause of evil?
    //rt->cds->removePrefixEntry((*it));
    PrefixEntry* ppe = rt->cds->getPrefixEntry(IPv6Address(*it));
    assert(ppe);

    rt->removePrefixEntry(ppe);
    it = oldRtr->prefixes.erase(it);

  }


  //Remove old routes going through oldRtr and remove oldRtr destEntry if it
  //still exists (The easiest way would be just to delete the router then all
  //the destEntries would have the neighbour pointers set to zero however we
  //need to keep the router around I think for forwarding etc. so do it the long
  //way)

  //rt->cds->removeDestEntry(oldRtr->re->addr()); would be taken care of by func
  //below anyway
  if (newRtr)
    rt->cds->updateDestEntryByNeighbour(oldRtr->re.lock()->addr(), newRtr->re.lock()->addr());
  else
    rt->cds->removeDestEntryByNeighbour(oldRtr->re.lock()->addr());

  //No need for forwarding from previous care of addr if we don't even have a
  //primary HA
  if (!mipv6cdsMN->primaryHA())
    mipv6cdsMN->removeHomeAgent(oldRtr);

  //remove outstanding bus? let it resend them if necessary
  mstateMN->removeBURetranTmr((BURetranTmr*)0, true);
  
}

/**
 * Route Optimisation (RO) checks. Will send BU to src of tunneled/encapsulated
 * packet (CN) if outer header src addr is a registered HA to which we have sent
 * BU to before.  Sec. 11.7.2 has more details
 *
 * Invoked at IPv6Encapsulation module.
 *
 * @param dgram is the outer tunnel packet including the inner dgram that has
 * not been modified
 *
 * @todo handle multiple home addresses
 */

void MIPv6NDStateHost::checkDecapsulation(IPv6Datagram* dgram)
{
  if (!mob->routeOptimise() || !mipv6cdsMN->primaryHA())
    return;

  //packet must have been encapsulated to end up here (this is a protected
  //function after all)

  bool valid = false;

  //outer header dest addr is equal to one of mobile node's care-of addrs
  if (mipv6cdsMN->careOfAddr() == dgram->destAddress())
    valid = true;
  else
  {
    //We'll just say anything that can be delivered locally is considered a coa
    // if (rt->localDeliver(dgram->destAddress()) &&
//         home_addr_scope_check(dgram->destAddress()))
//       valid = true;

    for (MIPv6CDSMobileNode::BULI it = mipv6cdsMN->bul.begin();
         it != mipv6cdsMN->bul.end(); it++)
    {
      if ((*it)->careOfAddr() == dgram->destAddress())
      {
        valid = true;
        break;
      }
    }
  }

  if (!valid)
  {
    Dout(dc::mipv6, rt->nodeName()<<" encapsulated datagram with outer dest="
        <<dgram->destAddress()<<" does not qualify as a care of address. Not doing RO");
    return;
  }

  valid = false;

  const ipv6_addr& tunDest =
    check_and_cast<IPv6Datagram* > (dgram->encapsulatedMsg())->destAddress();

  if (tunDest == mipv6cdsMN->homeAddr())
    valid = true;
  else
  {
    //check if BUL has a home address matching the tunnelled Destination
    if (home_addr_scope_check(tunDest))
    {
      for (MIPv6CDSMobileNode::BULI it = mipv6cdsMN->bul.begin();
           it != mipv6cdsMN->bul.end(); it++)
      {
        if ((*it)->homeAddr() == tunDest)
        {
          valid = true;
          break;
        }
      }
    }
  }

  if (!valid)
  {
    Dout(dc::mipv6, rt->nodeName()<<" encapsulated datagram with tunDest="
         <<tunDest<<" does not qualify as a home address in BUL. Not doing RO");
    return;
  }

  ipv6_addr cna = check_and_cast<IPv6Datagram* >
    (dgram->encapsulatedMsg())->srcAddress();

  if (dgram->srcAddress() == cna)
    valid = false;

  if (!valid)
  {
    Dout(dc::mipv6|dc::notice, rt->nodeName()
         <<" outer and inner src addresses are equal not valid CN correspondence. Not doing RO");
    return;
  }

  ipv6_addr coa = mipv6cdsMN->careOfAddr();

#ifdef USE_HMIP
  if (rt->hmipSupport())
  {

  HMIPv6CDSMobileNode* hmipv6cds = rt->mipv6cds->hmipv6cdsMN;

  //Also check if CN is on same link as us and send BU to it with our lcoa as
  //coa if MAP option flags allow this
  if (hmipv6cds->isMAPValid() && IPv6Address(cna, EUI64_LENGTH).isNetwork(hmipv6cds->localCareOfAddr()))
   {
     Dout(dc::notice, FILE_LINE<<" Suspicious code coa changed from "<<coa<<" to lcoa="
          <<hmipv6cds->localCareOfAddr());
     coa = hmipv6cds->localCareOfAddr();
     assert(coa != IPv6_ADDR_UNSPECIFIED);
   }

  //Necessary for HMIP as MAP tunnels to us will trigger incorrect route
  //optimisations.  Had we enforced "if (mipv6cdsMN->careOfAddr() !=
  //dgram->destAddress())" properly this would not happen but then we can't do
  //local route optimisation
  if (tunDest == coa)
    return;
  }
#endif //USE_HMIP


  IPv6Datagram* tunPacket =
    check_and_cast<IPv6Datagram*>(dgram->encapsulatedMsg());

  //Check if packet contains HOT/I or /COT/I 

  if (tunPacket->transportProtocol() == IP_PROT_IPv6_MOBILITY && 
      check_and_cast<MobilityHeaderBase*>(tunPacket->encapsulatedMsg()) != 0)
  {
    Dout(dc::mipv6, rt->nodeName()<<" encapsulated datagram contains a RR message. Not doing RO");
    return;
  }

  // Makes sense not to send BU to an ICMP error message since it is not
  // data traffic. But we should allow it for ping payloads TODO

  // could be an ICMPv6 message sent to MN's HoA
//  if (tunPacket->transportProtocol() == IP_PROT_IPv6_ICMP )
//    return;


  // Not in RFC
  // The peer node may also be a mn. When the peer node moves to a
  // foreign link, it may send the packets in tunnel. The source
  // address of the packet may be the coa of the peer node. We need to
  // check our BU and BCE to see if the packet comes from the peer
  // node. If it does, we don't need to do correspondent registration

  boost::weak_ptr<bc_entry> bce =
    rt->mipv6cds->findBindingByCoA(tunPacket->srcAddress());

    // to look up the destination cache entry

  //Use hoa of CN if we have a binding for it
  if (bce.lock())
    cna = bce.lock()->home_addr;

  Dout(dc::mipv6|flush_cf, "At " << rt->simTime() << "," << rt->nodeName()<<" receiving packet in tunnel, " << *tunPacket);


  // check if RO is happening already  
  for (MIPv6CDSMobileNode::BULI it = mipv6cdsMN->bul.begin();
       it != mipv6cdsMN->bul.end(); it++)
  {
    if ( tunPacket->srcAddress() == (*it)->addr())
    {
      //removeBU not implemented! 
      //mipv6cdsMN->removeBU(tunPacket->srcAddress());
      //Dout(dc::mipv6|flush_cf, rt->nodeName()<<" "<<rt->simTime()<<" BU in progress. Not doing RO again");
      return;
    }
  }

    if (mob->returnRoutability())
      mstateMN->sendInits(cna, coa);
    else
    {
      mstateMN->sendBU(cna, coa, mipv6cdsMN->homeAddr(),
                     rt->minValidLifetime(), false, 0);

      assert(mipv6cdsMN->homeAddr() == tunPacket->destAddress());
      Dout(dc::debug|flush_cf, rt->nodeName()<<" sending BU to CN (Route Optimisation) "
           <<cna);
    }
}

void sendNALater(cTimerMessage* tmr)
{
  tmr->callFunc();
  delete static_cast<cCallbackMessage*>(tmr->contextPointer());
  delete tmr;
}

/**
 * Does the actual procedure of returning home.
 *
 */

void MIPv6NDStateHost::returnHome()
{
  Dout(dc::notice|dc::mipv6|dc::mobile_move,  rt->nodeName()<<" "<<nd->simTime()
       <<" Returning home case detected");
  mipv6cdsMN->setAwayFromHome(false);

  //check home prefix is on link (rather silly when you consider that later on
  //it says not to do NS to find HA link layer addr but read earlier from RtrAdv
  //LL option which means that HA is the advertising router anyway so why not
  //just check global address of HA? (this is what we do)

  //Binding not expired yet (probably couldn't handle it if it were)
  if (!mipv6cdsMN->bulEmpty())
  {
    //no DAD how do I get it to do that, it is part of prefixAddrConf of
    //NDStateHost which happens before we get a chance. Well address was already
    //assigned so it won't go through DAD (unless expired of course but then if
    //that happens we don't have a pHA)
  }

  //send BU to HA A and H bit set coa = hoa and no home address option
  mstateMN->sendBUToAll(mipv6cdsMN->homeAddr(), mipv6cdsMN->homeAddr(), 0);

  //unsolicited NA to advertise our Link layer address for each on-link
  //prefix (just home addr for now) after BU sent.
  cCallbackMessage* cbuna = new cCallbackMessage;
  (*cbuna) = boost::bind(&MIPv6NDStateHost::sendUnsolNgbrAd, this, mipv6cdsMN->primaryHA()->re.lock()->ifIndex(),
	      mipv6cdsMN->homeAddr());
  cCallbackMessage* callLater = new cCallbackMessage;
  (*callLater) = boost::bind(sendNALater, cbuna);
  cbuna->setContextPointer(callLater);
  //ensure NA after BU sent
  nd->scheduleAt(nd->simTime() + 100.0 * SELF_SCHEDULE_DELAY, callLater);

#ifdef USE_HMIP
  if (rt->hmipSupport())
  {

    HMIPv6CDSMobileNode* hmipv6cds = rt->mipv6cds->hmipv6cdsMN;
    assert(hmipv6cds);
    hmipv6cds->setNoCurrentMap();

#if EDGEHANDOVER
    if (mob->edgeHandover())
    {
      rt->mipv6cds->ehcds->setNoBoundMap();
    }
  }
#endif //EDGEHANDOVER
#endif //USE_HMIP
}

///Calls sendBU of MStateMN eventually
void MIPv6NDStateHost::sendBU(const ipv6_addr& ncoa)
{
  if (callbackAdded(ncoa, 5490))
  {
    cCallbackMessage* cb = check_and_cast<cCallbackMessage*>(addressCallback(ncoa));
    if (cb)
      ;
    //removeCallback(
  }

  assert(ncoa != IPv6_ADDR_UNSPECIFIED);

  if (ncoa == IPv6_ADDR_UNSPECIFIED)
    return;

  ipv6_addr ocoa = mipv6cdsMN->careOfAddr();

  assert(ncoa != ocoa);

  //We use minimum valid lifetime since that's guaranteed to be <=
  //both home addr and coa
  mstateMN->sendBUToAll(ncoa, mipv6cdsMN->homeAddr(), rt->minValidLifetime());
}
// void MIPv6NDStateHost::enterState(void)
// {}

// void MIPv6NDStateHost::leaveState(void)
// {}

// void MIPv6NDStateHost::initialiseInterface(size_t ifIndex)
// {}

// void MIPv6NDStateHost::disableInterface(size_t ifIndex)
// {}

// bool MIPv6NDStateHost::valRtrAd(RA* ad)
// {
//   return false;
// }


} // end namesapce MobileIPv6

