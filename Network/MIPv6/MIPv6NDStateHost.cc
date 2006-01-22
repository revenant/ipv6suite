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
#include "cTTimerMessageCB.h"
#include "RoutingTable6.h"
#include "NeighbourDiscovery.h"
#include "IPv6CDS.h"

#include "NDTimers.h"
#include "InterfaceTable.h"
#include "IPv6InterfaceData.h"
#include "MIPv6Entry.h"
#include "MIPv6ICMPv6NDMessage.h"
#include "MIPv6CDSMobileNode.h"
#include "cTimerMessageCB.h" //schedSendUnsolNgbrAd
#include "MIPv6MNEntry.h"
#include "MIPv6DestOptMessages.h"

//extern for movement detection messages types
#include "NeighbourDiscovery.h"
#include "IPv6Encapsulation.h" //callback registration
#include "opp_utils.h"

#include "IPv6Mobility.h"
#include "MIPv6MStateMobileNode.h"
#include "WirelessEtherModule.h" //l2 trigger set

#include <iomanip> //setprecision

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

#include <boost/cast.hpp>


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

class RtrAdvMissedTmr: public cTTimerMessageCBA<void, void>
{
public:

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
   * @param mod this node's NeighbourDiscovery module to send the timer self
   * message on
   *
   * @param nodeName use to distinguish names of diff timers on per host basis
   */

  RtrAdvMissedTmr(const double& interval, const unsigned int& allowCount,
                  TFunctorBaseA<cTimerMessage>* missedConsecCB,
                  NeighbourDiscovery* mod, const string& nodeName)
    :cTTimerMessageCBA<void, void> (Tmr_RtrAdvMissed, mod,
                                    makeCallback(this,
                                                 &RtrAdvMissedTmr::missedRtrAdv),
                                    string(nodeName+"RtrAdvMissedTmr").c_str()),
     consecCountMissed(0), allowCount(allowCount), interval(interval),
     cb(missedConsecCB)
    {}

  ~RtrAdvMissedTmr()
  {
    delete cb;
  }

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
  TFunctorBaseA<cTimerMessage>* cb;
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
      (*cb)(this);
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
  : NDStateHost(mod), mipv6cdsMN(0), awayCheckDone(false), missedTmr(0), mob(0),
    mstateMN(0), schedSendRtrSolCB(
      new cTTimerMessageCBA<NDTimer, void>
      //(Sched_SendRtrSol, nd, makeCallback(this, &MIPv6NDStateHost::sendRtrSol),
      (Sched_SendRtrSol, nd, makeCallback((NDStateHost*) this, &NDStateHost::sendRtrSol),
       0, false, "schedSendRtrSol", true)), schedSendUnsolNgbrAd(0)
    ,potentialNewInterval(0), ignoreInitiall2trigger(true)

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

  tunMod->registerMIPv6TunnelCallback(makeCallback(this, &MIPv6NDStateHost::checkDecapsulation));

  mob = check_and_cast<IPv6Mobility*>
    (OPP_Global::findModuleByType(rt, "IPv6Mobility"));
  assert(mob != 0);

  //Todo change all instance() to return the state pointer to themself instead
  //of superclass since its not a virtual function anyway (even if was they can
  //differ by return type) and we referencing a static func by the correct
  //class itself.
  mstateMN = boost::polymorphic_downcast<MIPv6MStateMobileNode*>
    (MIPv6MStateMobileNode::instance());
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

      if ( wlanMod->linkUpTrigger() )
        wlanMod->setLayer2Trigger(new cTTimerMessageCBA<cTimerMessage, void>
                                  (Tmr_L2Trigger, mod, makeCallback
                                   (this, &MIPv6NDStateHost::movementDetectedCallback),
                                   ///Should pass this tmr as the argument
                                   //to callback as default arg of 0
                                   "Tmr_L2Trigger"));
    }
  }

  assert(rt->mipv6cds != 0);

  mipv6cdsMN = boost::polymorphic_downcast<MIPv6CDSMobileNode*> (rt->mipv6cds);

#endif //USE_MOBILITY

}

MIPv6NDStateHost::~MIPv6NDStateHost(void)
{
  if (missedTmr && !missedTmr->isScheduled())
    delete missedTmr;
  delete schedSendRtrSolCB;
}

void MIPv6NDStateHost::scheduleSendRtrSol(NDTimer* tmr)
{
//   if (schedSendRtrSolCB == 0)
//   {
//     schedSendRtrSolCB = new cTTimerMessageCBA<NDTimer, void>
//       (Sched_SendRtrSol, nd, makeCallback(this, &MIPv6NDStateHost::sendRtrSol),
//        0, false, "schedSendRtrSol", true);
//   }
  schedSendRtrSolCB->setArg(tmr);
  nd->scheduleAt(nd->simTime() + SELF_SCHEDULE_DELAY, schedSendRtrSolCB);
}

/**
 * Sec. 6.6
 *
 * Binary exponential backoff (double interval between consecutive rtr sol)
 * after MAX_RTR_SOLICITATIONS.  Increase rate after obtain new care of
 * address. Don't send rtr sol when node has valid care of address.

 * Stop sending solicitations when interval is > maximum (configurable)

 * Configurable minimum RTR_SOL_INTERVAL (default is 1 sec)

 * Movement Detection algorithm should reinitiate this again.
 *
 * @deprecated Draft 24 has no changes to sending Router Solicitations only Rtr
 * Advertisements
 * @todo remove this func and see if using plain ND sendRtrSol is alright or not
 */
/*
void MIPv6NDStateHost::sendRtrSol(NDTimer* tmr)
{
  bool away = awayFromHome();

  if (!away )
  {
    //Use old algorithm if at home
    return NDStateHost::sendRtrSol(tmr);
  }
  else
  {
    //Use new exponential backoff if not at home
    InterfaceEntry *ie = ift->interfaceByPortNo(tmr->ifIndex);

    if (tmr->counter < tmr->max_sends - 1)
      tmr->timeout = ie->ipv6()->mipv6Var.minRtrSolInterval;
    else
      tmr->timeout = tmr->timeout*2;

    ///The logic in this block below is necessary when movement detection
    ///returns positive and the home router is unreachable.  It is also
    ///applicable when MN starts up in a foreign network
    ///when it has a configured home subnet.  It

    if (tmr->dgram == 0)
    {
      //Don't do initial delay
      InterfaceEntry *ie = ift->interfaceByPortNo(tmr->ifIndex);
      tmr->dgram = new IPv6Datagram(
        ie->ipv6()->inetAddrs.empty()?IPv6Address(IPv6_ADDR_UNSPECIFIED):ie->ipv6()->inetAddrs[0],
        IPv6Address(ALL_ROUTERS_LINK_ADDRESS));

      tmr->dgram->setHopLimit(NDHOPLIMIT);

      RS* rs = new RS;

      if (!ie->ipv6()->inetAddrs.empty())
        rs->setSrcLLAddr(ie->llAddrStr());

      tmr->dgram->encapsulate(rs);

      tmr->msg = new RtrSolRetryMsg(Tmr_RtrSolTimeout, nd, cbRetrySol, tmr, true, "RtrSol");

      timerMsgs.push_back(tmr->msg);

    }

    if (tmr->timeout > ie->ipv6()->mipv6Var.maxInterval)
    {
      //Give up sending of Rtr solicitations
      Dout(dc::mipv6|dc::router_disc, rt->nodeName()<<":"<<tmr->ifIndex<<" "<<nd->simTime()
           <<" - exp. backoff excceded no router responded to solicitations");

      timerMsgs.remove(tmr->msg);
      delete tmr->msg;
      return;
    }

    Dout(dc::ipv6|dc::router_disc, rt->nodeName()<<":"<<tmr->ifIndex<<" "<<nd->simTime()
         <<" MIPv6 RtrSol "<<" timeout:"<< setprecision(4)
         << tmr->timeout<<" max:"<<ie->ipv6()->mipv6Var.maxInterval);
    nd->send(tmr->dgram->dup(), "outputOut", tmr->ifIndex);

    ///Schedule a timeout
    nd->scheduleAt(nd->simTime() + tmr->timeout, tmr->msg);

    tmr->counter++;
  }

}
*/

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
    mipv6cdsMN->insertHomeAgent(ha);
    bha = mipv6cdsMN->findRouter(ll_addr);
    Dout(dc::router_disc|flush_cf,  nd->rt->nodeName()<<":"<<ifIndex<<" "
         <<setprecision(6)<<nd->simTime()<<" Mipv6 router "<<ha->prefix()
         <<" added to list ");
  }
  else
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
    mipv6cdsMN->_homeAddr = mipv6cdsMN->primaryHA()->prefix();
    mipv6cdsMN->_homeAddr.prefix =
      mipv6cdsMN->formCareOfAddress(mipv6cdsMN->primaryHA(),
                                    ift->interfaceByPortNo(ifIndex));

    Dout(dc::mipv6|dc::mobile_move, rt->nodeName()<<" Primary HA registered. Addr="<<bha->addr());
  }


  if (!mipv6cdsMN->currentRouter())
  {
    if (rt->cds->defaultRouter().lock()->state() == NeighbourEntry::INCOMPLETE
        && rt->cds->routerCount() == 2)
    {
        Dout(dc::mipv6, " hack to get starting away from home working");
        rt->cds->setDefaultRouter(bha->re);
    }
    if ((mipv6cdsMN->movementDetected() || mipv6cdsMN->eagerHandover()) &&
        mipv6cdsMN->primaryHA())
    {

#ifdef USE_HMIP
      if (rt->hmipSupport() && rtrAdv->hasMapOptions())
        Dout(dc::hmip, rt->nodeName()<<" (no current router) Detected map options in MIPv6NDStateHost::processRtrAdv deferring handover");
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
      Dout(dc::debug|flush_cf, rt->nodeName()
           <<"Archaic movement detection triggered handover?");
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
        new RtrAdvMissedTmr(newInterval, ie->ipv6()->mipv6Var.maxConsecutiveMissedRtrAdv,
                            makeCallback(this,
                                         &MIPv6NDStateHost::movementDetectedCallback),
                            nd, rt->nodeName());
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
        check_and_cast<RtrSolRetryMsg*>(*it)->arg()->ifIndex == ifIndex)
    {
      return;
    }

  //reset this back to unsolicited as we believe we may be on new subnet
  rtrSolicited[ifIndex] = false;
  NDTimer* tmr = new NDTimer;
  tmr->max_sends = MAX_RTR_SOLICITATIONS;
  tmr->counter = 0;
  tmr->ifIndex = ifIndex;
  scheduleSendRtrSol(tmr);
  Dout(dc::mipv6|dc::mobile_move|dc::router_disc, rt->nodeName()<<" rtr Sol sent on ifIndex="<<tmr->ifIndex
       <<" in response to movement detected");
}

void MIPv6NDStateHost::movementDetectedCallback(cTimerMessage* tmr)
{
#ifdef USE_HMIP
  ///Invoked directly by discoverMAP now
  if (!rt->hmipSupport())
#endif //USE_HMIP
    assert(tmr != 0);

  if (mipv6cdsMN->currentRouter())
    mipv6cdsMN->previousDefaultRouter() = mipv6cdsMN->currentRouter();

  if (ift->numInterfaceGates() == 0)
    return;

  ///check if interface 0 is at least initialised totally
  if (!ifStats[0].initStarted || !globalAddrAssigned(0))
    return;

  //check what type of message i.e. what mechanism triggered a movement detected
  //invocation
  if (!tmr)
  {
    ///@note Assuming direct invocation from discoverMAP if tmr == 0
    Dout(dc::mobile_move|dc::hmip, rt->nodeName()<<" "<<setprecision(6)
         <<nd->simTime()<<" MAP algorithm detected movement");

    mipv6cdsMN->setAwayFromHome(true);

    //hmip knows movement is detected based on changing map options. Exception
    //is detecting first map which may be either at home or foreign
    //subnet(Always assuming the latter case for now).

  }
  else if (tmr->kind() == Tmr_RtrAdvMissed)
  {
    mipv6cdsMN->setMovementDetected(true);
    mipv6cdsMN->setAwayFromHome(true);
    Dout(dc::mipv6|dc::mip_missed_adv|dc::mobile_move, rt->nodeName()<<" "<<setprecision(6)
         <<nd->simTime()<<" exceeded MaxConsecMissedRtrAdv of "
        <<(mipv6cdsMN->currentRouter()?
           ift->interfaceByPortNo(mipv6cdsMN->currentRouter()->re.lock()->ifIndex())->ipv6()->mipv6Var.maxConsecutiveMissedRtrAdv
           :check_and_cast<RtrAdvMissedTmr*>(tmr)->maxMissed()));

    //Can also do NUD to current Rtr to confirm unreachability (once that's
    //implemented (although handover() to a null router will also do that Not
    //done though according to MIPL kernel and
    //\cite{nakajima03:handof_delay_analy_measur_sip_ipv6}

  }
  else if (tmr->kind() == Tmr_L2Trigger)
  {
    if (ignoreInitiall2trigger)
    {
      ignoreInitiall2trigger = false;
      return;
    }
    else
      mipv6cdsMN->setAwayFromHome(true);

    mipv6cdsMN->setMovementDetected(true);

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
  }

  if (tmr && tmr->kind() != Tmr_RtrAdvMissed || !tmr)
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
    if (mipv6cdsMN->findRouter(mipv6cdsMN->currentRouter()->re.lock()->addr(), it) &&
        (++it != mipv6cdsMN->mrl.end()) &&
        //Do a simple check to make sure this next router is not another router
        //also on previous subnet by using onlink global prefix
        (mipv6cdsMN->currentRouter()->prefix() != (*mipv6cdsMN->mrl.rbegin())->prefix()))
    {
      Dout(dc::mipv6|dc::mobile_move|flush_cf, rt->nodeName()
           <<" Detected movement and received rtrAdv from new router already");

#ifdef USE_HMIP
      ///Invoked directly by discoverMAP now
      if (rt->hmipSupport())
        ///@note Assuming direct invocation from discoverMAP if tmr == 0
        if (!tmr)
        {
          relinquishRouter(mipv6cdsMN->currentRouter(), (*mipv6cdsMN->mrl.rbegin()));
          return;
        }
#endif //USE_HMIP

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
      ///Because of the staged manner in which macro to local handover occurs
      ///through 2 stages i.e. macro depends on L2/missed tmr to elapse and uses
      ///cached info in mrl for macro handover we may have to wait for second
      ///RtrAdv before we actually switch to map since it was not cached the
      ///first time it was received because we did not think we were away from
      ///home. If we don't return now will kill off our current router. Todo
      ///remove this limitation and have it store map options always even when
      ///at home and practice the eager/lazy policy in hmip spec.
#ifdef USE_HMIP
      ///Invoked directly by discoverMAP now
      if (rt->hmipSupport())
        if (!tmr)
          return;
#endif //USE_HMIP

      Dout(dc::mobile_move|dc::mipv6|flush_cf, rt->nodeName()
           <<" Moved to foreign subnet and  have not received rtrAds yet. curRtr.prefix="
           <<mipv6cdsMN->currentRouter()->prefix()<<" mrl.prefix="<<(*mipv6cdsMN->mrl.rbegin())->prefix());

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
#ifdef USE_HMIP
      //because hmip defers everything to itself no one sets the currentRouter
      //when moving to new subnet after moving to a no coverage zone. As a
      //result the defaultRouter still points to the previous one and nothing
      //works. So we need to relinquish to new router if invoked from hmip
      //directly.
      if (rt->hmipSupport())
      {
        if (!tmr && mipv6cdsMN->previousDefaultRouter()->prefix() != (*mipv6cdsMN->mrl.rbegin())->prefix())
        {
          relinquishRouter(mipv6cdsMN->currentRouter(), (*mipv6cdsMN->mrl.rbegin()));
          Dout(dc::hmip, " hmip invoked relinquishRouter from null to "<<(*mipv6cdsMN->mrl.rbegin())->prefix());
        }
      }
      else
#endif //USE_HMIP
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
        Dout(dc::notice|dc::mipv6|dc::mobile_move,  rt->nodeName()<<" "<<nd->simTime()
             <<" Returning home case detected");
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
      mipv6cdsMN->setFutureCoa(coa);

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

        if ( mob->signalingEnhance() == CellResidency )
        {
          if ( mob->linkDownTime )
          {
            mob->handoverDelay = nd->simTime() - mob->linkDownTime;
            mob->linkDownTime = 0;
          }
        }

        if ( mob->isEwuOutVectorHODelays() )
          mob->recordHODelay( nd->simTime() );
      }
      else
      {
        //Not assigned at all yet unless DAD has finished (it would be in
        //tentativeAddr in this case)
        Dout(dc::mipv6|dc::neighbour_disc|flush_cf, rt->nodeName()<<" "<<nd->simTime()
             <<ifIndex<<" waiting for dad completion before sending BU for coa "
             <<coa);
      }

    } //if primary HA exists
    else
    {
      //we'll remove oldRtr  at end of func if no primaryHA.
    }

    mipv6cdsMN->setMovementDetected(false);
  } //if newRtr exists

  relinquishRouter(oldRtr, newRtr);

  if ( mob->isEwuOutVectorHODelays() )
    mob->setLinkUpTime(0); // clear the link up time
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
    //Remove the addr too that do not have global scope i.e. link local or
    //site local except for the autoconfigured address at position 0
    if (!home_addr_scope_check((*it).prefix))
    {
      IPv6Address addr = oie->ipv6()->matchPrefix((*it).prefix, (*it).length);
      if (addr != IPv6_ADDR_UNSPECIFIED)
      {
        Dout(dc::mipv6, rt->nodeName()<<__FUNCTION__
             <<":  onlink local addresss "<<addr<<" removed");
        rt->removeAddress(addr, oldIfIndex);
      }
      else if (oie->ipv6()->matchPrefix((*it).prefix, (*it).length), true)
      {
        //Must be tentative addrs
        DoutFatal(dc::core|error_cf, "Unimplemented func for removing tentative addr when node moves "
                  <<" too fast as dad timer has to be removed cleanly see "
                  <<" checkDupAddrDetected and separate the bits there into separate func");
      }
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
}

/**
 * Send BU to src of tunneled/encapsulated packet (CN) if outer header src addr
 * is a registered HA to which we have sent BU to before.  Sec. 10.9 has details
 * on exact checks
 *
 * Invoked at IPv6Encapsulation module.
 *
 * @param dgram is the outer tunnel packet including the inner dgram that has
 * not been modified
 *
 */

void MIPv6NDStateHost::checkDecapsulation(IPv6Datagram* dgram)
{
  if (!mipv6cdsMN->primaryHA())
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
        <<dgram->destAddress()<<" does not qualify as a care of address");
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
         <<tunDest<<" does not qualify as a home address in BU");
    return;
  }

  ipv6_addr cna = check_and_cast<IPv6Datagram* >
    (dgram->encapsulatedMsg())->srcAddress();

  if (dgram->srcAddress() == cna)
    valid = false;

  if (!valid)
  {
    Dout(dc::mipv6|dc::notice, rt->nodeName()
         <<" outer and inner src addresses are equal not valid CN correspondence");
    return;
  }

  ipv6_addr coa = mipv6cdsMN->careOfAddr();

#ifdef USE_HMIP
  if (rt->hmipSupport())
  {
// Unnecessary now with tunDest==coa check
//   if (cna == mipv6cdsMN->primaryHA()->addr())
//   {
//     Dout(dc::hmip, __FUNCTION__<<": Ignoring CN with srcAddress of HA "
//          <<cna<<" to prevent BA from HA triggering Route Optimisation "
//          <<"causing off by one sequence no. when BU is sent to HA in CN mode");
//     //TODO better fix would be to stop sending BU to nodes that have outstanding
//     //unacknowledged BU i.e. store all BURetransTmr in a list and search in
//     //there
//     return;
//   }

  HMIPv6CDSMobileNode* hmipv6cds =
    boost::polymorphic_downcast<HMIPv6CDSMobileNode*>(mipv6cdsMN);

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


  // the mobile node only initiates the route optimization process
  // when the received packet is a data packet. The mobile node may
  // receive a mobility message in tunnel when the other communicating
  // mobile node also moves to a foreign network

  IPv6Datagram* tunPacket =
    check_and_cast<IPv6Datagram*>(dgram->encapsulatedMsg());


  // could be an ICMPv6 message sent to MN's HoA
  if (tunPacket->transportProtocol() == IP_PROT_IPv6_ICMP )
    return;


  // check if the BU has already been created

  for (MIPv6CDSMobileNode::BULI it = mipv6cdsMN->bul.begin();
       it != mipv6cdsMN->bul.end(); it++)
  {
    if ( tunPacket->srcAddress() == (*it)->addr())
    {
      mipv6cdsMN->removeBU(tunPacket->srcAddress());
      break;

    }
  }

  // The peer node may also be a mn. When the peer node moves to a
  // foreign link, it may send the packets in tunnel. The source
  // address of the packet may be the coa of the peer node. We need to
  // check our BU and BCE to see if the packet comes from the peer
  // node. If it does, we don't need to do correspondent registration

  boost::weak_ptr<bc_entry> bce =
    mipv6cdsMN->findBindingByCoA(tunPacket->srcAddress());

  if (bce.lock())
  {
    for (MIPv6CDSMobileNode::BULI it = mipv6cdsMN->bul.begin();
         it != mipv6cdsMN->bul.end(); it++)
    {
      if ( bce.lock()->home_addr == (*it)->addr())
        return;
    }
  }

  MIPv6MobilityHeaderBase* ms = 0;
  if (tunPacket->transportProtocol() == IP_PROT_IPv6_MOBILITY)
    ms = check_and_cast<MIPv6MobilityHeaderBase*>(
      tunPacket->encapsulatedMsg());

  // It could be HoTI that is reverse tunneled by the sender's HA
  // and gets tunneled again by the receiver's HA

  if (mob->routeOptimise() && ms == 0)
  {
    // In simultaneous movement, if the correspondent node completes
    // route optimization with the mobile node before the mobile node
    // does, the mobile node would use the care-of address of the
    // correspondent node as destionation for HoTI and CoTI to
    // initiate the route optimization process.

    // A suggested solution to overcome such issue is to check if the
    // received data packet has a home address option. This implies
    // that the correspondent node has a BUL entry for the mobile node
    // if it does.  The mobile node MAY do a futher check to determine
    // if the home address in the home address option appears in the
    // home address field of the BC entry for the correspodnent
    // node. Hence, the mobile node would use home address of the
    // correspondent node as the destination for HoTI and CoTI to
    // initiate the route optimization process. If not, it would
    // simply use the source address of the packet as destination
    // address for the HoTI and CoTI as described in the Mobile IPv6
    // specification.

/*
    HdrExtProc* proc = check_and_cast<IPv6Datagram* >(dgram->encapsulatedMsg())->findHeader(EXTHDR_DEST);
    if (proc)
    {
      IPv6TLVOptionBase* opt =
        check_and_cast<HdrExtDestProc*>(proc)->
        getOption(IPv6TLVOptionBase::MIPv6_HOME_ADDRESS_OPT);

      if (opt)
      {
        MIPv6TLVOptHomeAddress* haOpt = check_and_cast<MIPv6TLVOptHomeAddress*>(opt);
        assert(haOpt);
        ipv6_addr hoa = haOpt->homeAddr();

        IPv6NeighbourDiscovery::IPv6CDS::DCI it;
        bool findDest = rt->cds->findDestEntry(hoa, it);

        boost::weak_ptr<bc_entry> bce = it->second.bce;
        if (bce.lock().get() != 0 && !mob->directSignaling())
            const_cast<ipv6_addr&>(cna) = bce.lock().get()->home_addr;
      }
    }
*/
    // OR

    // to look up the destination cache entry
    boost::weak_ptr<bc_entry> bce =
      mipv6cdsMN->findBindingByCoA(tunPacket->srcAddress());
    if (bce.lock())
      cna = bce.lock()->home_addr;

    Dout(dc::mipv6|flush_cf, "At " << rt->simTime() << "," << rt->nodeName()<<" receiving packet in tunnel, " << *tunPacket);

    ///////

    if (mob->returnRoutability())
        mstateMN->sendInits(cna, coa, mob);
    else
    {
      //What happens if home_addr is really our previous coa? so use the one
      //registered with primaryHA. What if there are multiple home addresses?
      mstateMN->sendBU(cna, coa, mipv6cdsMN->homeAddr(),
                     rt->minValidLifetime(), false, 0, mob);
      Dout(dc::debug|flush_cf, rt->nodeName()<<" sending BU to CN (Route Optimisation) "
           <<cna);
    }
  }
}

/**
 * Does the actual procedure of returning home.
 *
 */

void MIPv6NDStateHost::returnHome()
{
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

  mstateMN->sendBUToAll(mipv6cdsMN->homeAddr(), mipv6cdsMN->homeAddr(), 0,
                        mob);

  //unsolicited NA to advertise our Link layer address for each on-link
  //prefix (just home addr for now) after BU sent.

  //delay by 2*SELF_SCHEDULE_DELAY to ensure after BU sent (1*SELF_SCHEDULE_DELAY)
  //sendUnsolNgbrAd(mipv6cdsMN->primaryHA()->re->ifIndex(), mipv6cdsMN->homeAddr());

//typedef required to disambiguate overloaded send func
//     typedef int (IPv6Mobility::*sendPtr)(cMessage*, const char*, int);

  Loki::cTimerMessageCB
    <void, TYPELIST_2(unsigned int, ipv6_addr)>* schedUNA = 0;
  if (!schedSendUnsolNgbrAd)
  {
    schedUNA = new Loki::cTimerMessageCB<void, TYPELIST_2(unsigned int, ipv6_addr)>
       (Sched_SendUnsolNgbrAd, nd, this, &MobileIPv6::MIPv6NDStateHost::sendUnsolNgbrAd,
        "Sched_SendUnsolNgbrAd");
  }
  else
    schedUNA = static_cast<Loki::cTimerMessageCB<void, TYPELIST_2(unsigned int, ipv6_addr)>* >
      (schedSendUnsolNgbrAd);
  Loki::Field<0> (schedUNA->args) = mipv6cdsMN->primaryHA()->re.lock()->ifIndex();
  Loki::Field<1> (schedUNA->args) = mipv6cdsMN->homeAddr();
  schedSendUnsolNgbrAd = schedUNA;
  nd->scheduleAt(nd->simTime() +  2*SELF_SCHEDULE_DELAY, schedSendUnsolNgbrAd);

#ifdef USE_HMIP
  if (rt->hmipSupport())
  {
    HMIPv6CDSMobileNode* hmipv6cds =
      boost::polymorphic_downcast<HMIPv6CDSMobileNode*>(rt->mipv6cds);
    assert(hmipv6cds);
    hmipv6cds->setNoCurrentMap();

#if EDGEHANDOVER
    if (mob->edgeHandover())
    {
      (boost::polymorphic_downcast<EdgeHandover::EHCDSMobileNode*>(hmipv6cds))
        ->setNoBoundMap();
    }
  }
#endif //EDGEHANDOVER
#endif //USE_HMIP
}

///Calls sendBU of MStateMN eventually
void MIPv6NDStateHost::sendBU(const ipv6_addr& ncoa)
{
  assert(ncoa != IPv6_ADDR_UNSPECIFIED);

  bool pcoa = false;
  //this returns the coa from setFutureCoa
  if (mipv6cdsMN->careOfAddr(pcoa) != ncoa || ncoa == IPv6_ADDR_UNSPECIFIED)
    return;

  ipv6_addr ocoa = mipv6cdsMN->careOfAddr();

#ifdef USE_HMIP
  bool handoverDone = false;
  if (rt->hmipSupport())
  {
    HMIPv6NDStateHost* derived = boost::polymorphic_downcast<HMIPv6NDStateHost*>(this);
    assert(derived);
    //Used only for local handovers i.e. update MAPs binding for lcoa
    handoverDone = derived->arhandover(ncoa);
  }
  if (!handoverDone)
  {
#endif //USE_HMIP

  //We use minimum valid lifetime since that's guaranteed to be <=
  //both home addr and coa
  mstateMN->sendBUToAll(ncoa, mipv6cdsMN->homeAddr(), rt->minValidLifetime(),
                        mob);

#ifdef USE_HMIP
  }
#endif //USE_HMIP
  mipv6cdsMN->setFutureCoa(IPv6_ADDR_UNSPECIFIED);

#ifdef USE_HMIP
  if (handoverDone)
    return;
#endif //USE_HMIP

  //Cannot put this after sendBUToAll as careOfAddr asserts
  //Set up reverse tunnelled link from MN to HA here and tear down old tunnel
  IPv6Encapsulation* tunMod = check_and_cast<IPv6Encapsulation*>
    (OPP_Global::findModuleByType(rt, "IPv6Encapsulation"));
  assert(tunMod != 0);
  if (ocoa != IPv6_ADDR_UNSPECIFIED && ocoa != ncoa)
    tunMod->destroyTunnel(ocoa, mipv6cdsMN->primaryHA()->prefix().prefix);

  Dout(dc::debug|flush_cf, "finding tunnel entry="<<mipv6cdsMN->careOfAddr(pcoa)
       <<" exit="<<mipv6cdsMN->primaryHA()->prefix().prefix);
  size_t vIfIndex = tunMod->findTunnel(mipv6cdsMN->careOfAddr(pcoa),
                                     mipv6cdsMN->primaryHA()->prefix().prefix);

  //assert(!vIfIndex);
  //Sometimes old tunnel is not removed so it may have been created already when
  //we revisit past ARs
  if (!vIfIndex)
  {
  //assuming single mobile interface at 0
  vIfIndex = tunMod->createTunnel(ncoa, mipv6cdsMN->primaryHA()->prefix().prefix, 0);

  Dout(dc::mipv6|dc::encapsulation|dc::debug|flush_cf, " reverse tunnel created entry="
         <<ncoa<<" exit="<<mipv6cdsMN->primaryHA()->prefix()<<" vIfIndex="<<hex<<vIfIndex<<dec);
  }
  else
  {
    Dout(dc::mipv6|dc::encapsulation, " reverse tunnel exists already "<<ncoa<<" exit="<<mipv6cdsMN->primaryHA()->prefix()<<" vIfIndex="<<hex<<vIfIndex<<dec);
    Dout(dc::mipv6|dc::encapsulation, *tunMod);
  }

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

