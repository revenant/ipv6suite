//
// Copyright (C) 2001, 2004 CTIE, Monash University
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

/**
   @file NDStateRouter.cc
   @brief Implementation of NDStateRouter class.
   Implements Neighbour Discovery mehanisms defined in RFC2461 and AutofConf in
   RFC2462 for a router.

   @author Johnny Lai
   @date 24.9.01
*/

#include "sys.h"
#include "debug.h"

#include <iomanip> //setprecision
#include <memory> //auto_ptr
#include <sstream>
#include <iostream>

#include "NDStateRouter.h"
#include "NDTimers.h"
#include "NeighbourDiscovery.h"
#include "ICMPv6Message.h"
#include "ICMPv6NDMessage.h"
#include "RoutingTable6.h"
#include "IPv6Datagram.h"
#include "ipv6_addr.h"
#include "cTimerMessage.h"
#include "IPv6CDS.h"
#include "LL6ControlInfo_m.h"

#ifdef USE_HMIP
#include "HMIPv6ICMPv6NDMessage.h"
#include "HMIPv6Entry.h"
#endif // USE_HMIP

#include "opp_utils.h"
#include "opp_akaroa.h"

#include "IPv6OutputCore.h"



namespace IPv6NeighbourDiscovery
{


size_t NDStateRouter::outputUnicastGate = UINT_MAX;


/**
   NDStateRouter
 */
NDStateRouter::NDStateRouter(NeighbourDiscovery* mod):NDStateHost(mod)
{
  //RFC 2461 Sec 6.2.2
  IPv6Address mr_node_addr(ALL_ROUTERS_NODE_ADDRESS);
  IPv6Address mr_link_addr(ALL_ROUTERS_LINK_ADDRESS);
  IPv6Address mr_site_addr(ALL_ROUTERS_SITE_ADDRESS);

  mr_node_addr.scope();
  Dout(dc::debug,  rt->nodeName()<<" IPv6Address obj to be added to multicast group for all nodes,"
       <<" all links and all sites"<<mr_node_addr <<" "<<mr_link_addr <<" "
       <<mr_site_addr);

  assert(mr_node_addr.scope() == ipv6_addr::Scope_Node);
  mr_link_addr.scope();
  assert(mr_link_addr.scope() == ipv6_addr::Scope_Link);
  mr_site_addr.scope();
  assert(mr_site_addr.scope() == ipv6_addr::Scope_Site);

  rt->joinMulticastGroup(mr_node_addr);
  rt->joinMulticastGroup(mr_link_addr);
  rt->joinMulticastGroup(mr_site_addr);

  MAX_INITIAL_RTR_ADVERT_INTERVAL = 16.0;
  MAX_INITIAL_RTR_ADVERTISEMENTS = 3;
  MAX_FINAL_RTR_ADVERTISEMENTS = 3;
  MAX_RA_DELAY_TIME = 0.5;

#ifdef USE_MOBILITY
  if(rt->mobilitySupport())
  {
    MIN_DELAY_BETWEEN_RAS = 0.05;
  }
  else
  {
    MIN_DELAY_BETWEEN_RAS = 3.0;
  }
#else
  MIN_DELAY_BETWEEN_RAS = 3.0;
#endif //USE_MOBILITY

  ///Stuff for unicast sending
  if (rt->interfaceCount()==0)
  {
    outputMod = NULL; //XXX is that enough? --AV
    outputUnicastGate = -1;
    return;
  }
  outputMod = new cModule*[rt->interfaceCount()];
  //nd->icmp->proc
  cModule* procMod = nd->parentModule()->parentModule();
  assert(procMod);
  //ned gen sources don't have header files for incl.
  //assert(check_and_cast<IPv6Processing*>(procMod));
  for (unsigned int i = 0; i < rt->interfaceCount(); i++)
  {
    outputMod[i] = procMod->submodule("output", i);
    outputMod[i] = outputMod[i]->submodule("core");
    assert(outputMod[i]);
    assert(check_and_cast<IPv6OutputCore*>(outputMod[i]));
    if (!outputMod[i])
      DoutFatal(dc::core|error_cf, "Cannot find outputCore module");
  }

  // Initialise this magic number once only as its constant during runtime and
  // should be same modules of same type
  if (outputMod[0] != 0 && outputUnicastGate == UINT_MAX)
  {
    outputUnicastGate = outputMod[0]->findGate("neighbourDiscoveryDirectIn");
    assert(outputUnicastGate != UINT_MAX);
    if (UINT_MAX == outputUnicastGate)
      DoutFatal(dc::core|error_cf, "Cannot find gate");
  }

}

NDStateRouter::~NDStateRouter()
{
  for (size_t i = 0; i < advTmrs.size();i++)
  {
    //Need to call drop from a cObject derived class e.g. cTimerMessage?
    //simulation.msgQueue.drop(advTmrs[i]->msg);
    //delete advTmrs[i];
  }
#if FASTRA
  delete [] outputMod;
#endif //FASTRA
}

std::auto_ptr<ICMPv6Message>  NDStateRouter::processMessage(std::auto_ptr<ICMPv6Message> msg)
{
  switch(msg->type() )
  {
      case ICMPv6_ROUTER_SOL:
        processRtrSol((OPP_Global::auto_downcast<ICMPv6NDMRtrSol> (msg)).release());
        nd->ctrIcmp6InRtrSol++;

        break;
      case ICMPv6_ROUTER_AD:
//        if (!valRtrAd((OPP_Global::auto_downcast<IPv6NeighbourDiscovery::ICMPv6NDMRtrAd> (msg)).release()))
//          ev <<"Inconsistent message detected"<<endl;
#ifndef USE_HMIP
        msg.reset();
#else
        processRtrAd(OPP_Global::auto_downcast<ICMPv6NDMRtrAd>(msg));
#endif //USE_HMIP

        nd->ctrIcmp6InRtrAdv++;

        break;
      case ICMPv6_NEIGHBOUR_SOL:
        processNgbrSol(OPP_Global::auto_downcast<ICMPv6NDMNgbrSol>(msg));
        nd->ctrIcmp6InNgbrSol++;

        break;
      case ICMPv6_NEIGHBOUR_AD:
        processNgbrAd(OPP_Global::auto_downcast<ICMPv6NDMNgbrAd>(msg));
        nd->ctrIcmp6InNgbrAdv++;

        break;
      case ICMPv6_REDIRECT:
        //8.2
        //Router does not update routing tables when this is received.  But
        //what happens when a host acting as a router has servers running on it?
        msg.reset();
        nd->ctrIcmp6InRedirect++;

        break;
      default:
        Dout(dc::warning, "Unknown ND message received " << msg->type());
        msg.reset();
        break;
  }

  return msg;

}

// Protected Functions

void NDStateRouter::enterState()
{
  assert(!stateEntered);
  stateEntered = true;

  //By this stage a link local addr should be assigned on one interface

  assert(rt->interfaceCount() > 0);
}

void NDStateRouter::leaveState()
{
  //Sec 6.2.5
  //Send  up to MAX_FINAL_RTR_ADV ads. with Router Lifetime of 0.
  //Leave the all routers multicast group
  //Subsequent Neigbhour ads. have router flag 0
  Dout(dc::warning, "Unimplemented behaviour "<<FILE_LINE);
}

///Do this only for interfaces that have link local addr assigned.
void NDStateRouter::initialiseInterface(size_t ifIndex)
{
  //Do dupaddrDet on other addrs, create addresses from preconfigured prefixes
  //and start unsol rtr ad
  NDStateHost::initialiseInterface(ifIndex);

  Interface6Entry& ie = rt->getInterfaceByIndex(ifIndex);

/*
    //Assume that XML config will assign address (will go through DAD rules).
    //If you want to use this code then do not specify the same address in XML conf
    //otherwise same address assigned twice (assertion)
    //Of course I could add a check in prefixAddrConf to ensure not
    //assigned before

  typedef LinkPrefixes::iterator LPI;
  LinkPrefixes prefixes(rt->cds->getPrefixesByIndex(ifIndex));
  for (LPI it = prefixes.begin(); it != prefixes.end(); it++)
  {
    if ((*it)->advAutoFlag())
      prefixAddrConf(ifIndex, (*it)->prefix(), (*it)->prefix().prefixLength(),
                     (*it)->advPrefLifetime(), (*it)->advValidLifetime());
  }
*/

  if (ie.rtrVar.advSendAds)
  {

    Dout(dc::notice, rt->nodeName()<<":"<<ifIndex<<" is advertising");

    for(size_t i = 0; i < ie.rtrVar.advPrefixList.size(); i++)
      if (ie.rtrVar.advPrefixList[i].advOnLink())
        rt->insertPrefixEntry(ie.rtrVar.advPrefixList[i], ifIndex);

    RtrTimer* tmr = new RtrTimer;
    tmr->ifIndex = ifIndex;
    tmr->noInitAds = 0;
    ///Guess these two variables below are constants so don't need to store
    //separate ones for each interface.  (Can't really make a generic template
    //timer class since other messages don't have this behaviour)
    tmr->maxInitAds = MAX_INITIAL_RTR_ADVERTISEMENTS;
    tmr->maxInitRtrInterval = MAX_INITIAL_RTR_ADVERT_INTERVAL;

    sendUnsolRtrAd(tmr);
  }
}

/**
   RFC 2461 Sec 6.2.4
   Send an unsolicated multicast Router Adv. when delay has expired.

   6.2.3 -4
   A router can send Router Adv. without becoming a default router e.g. to
   notify nodes of prefixes for auto conf by setting Router Lifetime field in
   outgoing advs to 0.

   @todo detect changing system information at runtime and reset maxInitAds as
   specified in last paragraph of 6.2.4
*/
void  NDStateRouter::sendUnsolRtrAd( RtrTimer* tmr)
{
  IPv6Datagram* dgram = createDatagram(tmr->ifIndex, IPv6Address(ALL_NODES_LINK_ADDRESS));

  const Interface6Entry& ie = rt->getInterfaceByIndex(tmr->ifIndex);
  const Interface6Entry::RouterVariables& rtrVar = ie.rtrVar;

  //Calculate new delay
  double delay = OPP_UNIFORM(rtrVar.minRtrAdvInt, rtrVar.maxRtrAdvInt);
  assert(delay <= rtrVar.maxRtrAdvInt);

  if (tmr->noInitAds < tmr-> maxInitAds && delay > tmr->maxInitRtrInterval)
    delay = tmr->maxInitRtrInterval;
  //@todo
  //Check that the resulting pdu is not > link MTU if so then split the prefix
  //list into many router ads.
  nd->send(dgram, "outputOut", tmr->ifIndex);

  nd->ctrIcmp6OutRtrAdv++;
  rt->ctrIcmp6OutMsgs++;

#if FASTRA
  if (ie.rtrVar.fastRA && ie.rtrVar.fastRACounter)
  {
    ie.rtrVar.fastRACounter = 0;
    Dout(dc::debug|flush_cf, rt->nodeName()<<":"<<tmr->ifIndex
         <<" "<<nd->simTime()<<" fastRACounter reset to 0");
  }
#endif //FASTRA

  //Only need to increment when maxInitAds has not been exceeded yet otherwise
  //will get roll back and delays happening again

  if (tmr->noInitAds <= tmr->maxInitAds)
  {
    Dout(dc::debug, rt->nodeName()<<":"<<tmr->ifIndex<<" "<<nd->simTime()
         <<" RtrAdv initSends: "<<tmr->noInitAds
         <<" maxInitSends:"<<tmr->maxInitAds
         <<" nextAdvDelay:"<<setprecision(6)<<delay);
    tmr->noInitAds++;
  }

  cTimerMessage*& msg = tmr->msg;

  //Can reuse timer message as we are sending to self
  if (msg == 0)
  {
    stringstream msgName;
    msgName <<"UnsolRA "<<rt->nodeName()<<":"<<tmr->ifIndex;

    msg = ::createTmrMsg(Tmr_NextUnsolRtrAd, nd, this,
                         &NDStateRouter::sendUnsolRtrAd, tmr,
                         nd->simTime() + delay, false, msgName.str().c_str());
    advTmrs.push_back(tmr);
    return;
  }

  tmr->msg->reschedule(nd->simTime() + delay);

  Dout(dc::router_disc, rt->nodeName()<<":"<<tmr->ifIndex<<" "<<nd->simTime()
       <<" UnsolRtrAdv sendingTime: "<<setprecision(6)<<msg->sendingTime()
       <<" arivalTime: "<<setprecision(4)<<msg->arrivalTime()
       <<" nextAdvDelay: "<<setprecision(4)<<delay);
#ifdef USE_HMIP
//  Debug(
//    ICMPv6NDMRtrAd* rtrAd = check_and_cast<ICMPv6NDMRtrAd*> (dgram->encapsulatedMsg());
//    Dout(dc::debug,  rt->nodeName()<<":"<<tmr->ifIndex<<" "<<nd->simTime()
//         <<" INCLUDE MAP options? "<<(rtrAd->hasMapOptions()? "YES":"NO")
//         <<" "<<&transitMapOpts);
//    );
  //Cleared so that we only maintain an active list of MAP options from other
  //Rtrs' advertisements. Race conditions can arise where the advertising router
  //that supplies us with map options has less frequent multicast advs. than
  //this router. Really should remove options that are no longer advertised by
  //the upstream source of the map option.
//  transitMapOpts.clear();
#endif //USE_HMIP
}

void NDStateRouter::disableInterface(size_t ifIndex)
{}

///Private Functions
void NDStateRouter::processRtrSol(RS* rs)
{
  if (valRtrSol(rs))
    sendRtrAd(rs);
  delete rs;
}


bool NDStateRouter::valRtrSol(RS* msg)
{

  IPv6Datagram* dgram = check_and_cast<IPv6Datagram*> (msg->encapsulatedMsg());
  if (dgram->hopLimit() == NDHOPLIMIT &&
      msg->calculateChecksum() == msg->checksum() &&
      msg->length() >= 8)
  {

    //No source LL address option when unspecified IP address or it should be
    //specified
    if ((dgram->srcAddress() == IPv6_ADDR_UNSPECIFIED && !msg->hasOptions()) ||
        msg->hasOptions())
      return true;
    else
    {
      Dout(dc::router_disc, rt->nodeName()<<":"<<dgram->inputPort()<<" "<<rt->simTime()
           <<" ValRtrSol - src link layer address not found");
    }
  }

  Dout(dc::router_disc, rt->nodeName()<<":"<<dgram->inputPort()<<" "<<rt->simTime()
       <<" ValRtrSol invalid - dropped silently!\n"
       <<" TTL="<<dgram->hopLimit()<<" length="<<msg->length());
  return false;
}

///Sec. 6.2.7
bool NDStateRouter::valRtrAd(RA* ad)
{
  //TODO - sounds rather ambiguous in spec
  return true;
}

/**
   Send a solicited RtrAdv delayed by at most MAX_RA_DELAY_TIME and rate limited
   to one per MIN_DELAY_BETWEEN_RAS seconds (with reference to unsolicited
   rtrAdv) refer to RFC 2461 Sec. 6.2.6.  It is always a multicast adv never
   unicast unless Fast RA is turned on.
   @param rtrSol is the Router Solicitation that was received
*/
void  NDStateRouter::sendRtrAd(RS* rtrSol)
{
  //Assume (Assumptions turn out to be true during debug)
  //msg->sendingTime() is time when timer message was sent or scheduled at
  //Equivalent to prev scheduled Adv.
  //msg->arrivalTime() is time when timer message will arrive at or time arrived
  //already.
  //Equivalent to next scheduled Adv.

  IPv6Datagram* dgram = check_and_cast<IPv6Datagram*>(rtrSol->encapsulatedMsg());
  bool found = false;
  RtrTimer* tmr = 0;
  double delay = 0;
  cTimerMessage* msg = 0;

  for (size_t i = 0; i < advTmrs.size(); i++)
    if (static_cast<int> (advTmrs[i]->ifIndex) == dgram->inputPort())
    {
      found = true;

      tmr = advTmrs[i];
      msg = tmr->msg;

      //really want random not uniform
      delay = OPP_UNIFORM(0, MAX_RA_DELAY_TIME);

#if FASTRA
      Interface6Entry& ie = rt->getInterfaceByIndex(tmr->ifIndex);
      if (ie.rtrVar.fastRA)
      {
        if (ie.rtrVar.fastRACounter <= ie.rtrVar.maxFastRAS)
        {
          if (dgram->srcAddress() != IPv6_ADDR_UNSPECIFIED)
          {

            nd->cancelEvent(msg);
            //restart multicast timer
            nd->scheduleAt(nd->simTime() + MIN_DELAY_BETWEEN_RAS + delay, msg);

            if (rtrSol->hasSrcLLAddr())
            {
              //TODO send to addrResln instead so they can resolve address if
              //source did not include source link-layer addr opt.
/* XXX code changed to use LL6ControlInfo --AV
              IPv6Datagram* advDgram = createDatagram(tmr->ifIndex, dgram->srcAddress());
              LLInterfaceInfo info = {advDgram, rtrSol->srcLLAddr()};
              //cGate *inputgate instead of mod and gatename/id
              nd->sendDirect( new LLInterfacePkt(info), 0,
                              outputMod[tmr->ifIndex], outputUnicastGate);
*/
              IPv6Datagram* advDgram = createDatagram(tmr->ifIndex, dgram->srcAddress());
              LL6ControlInfo *ctrlInfo = new LL6ControlInfo();
              ctrlInfo->setDestLLAddr(rtrSol->srcLLAddr());
              advDgram->setControlInfo(ctrlInfo);
              nd->sendDirect(advDgram, 0, outputMod[tmr->ifIndex], outputUnicastGate); // XXX why sendDirect? -AV

              ie.rtrVar.fastRACounter++;
              Dout(dc::router_disc|dc::debug, rt->nodeName()<<":"<<tmr->ifIndex
                   <<" "<<nd->simTime()<<" fast ra sent counter="<<ie.rtrVar.fastRACounter
                   <<" max="<<ie.rtrVar.maxFastRAS);

              nd->ctrIcmp6OutRtrAdv++;
              rt->ctrIcmp6OutMsgs++;
            }
            else
              Dout(dc::router_disc|dc::debug, rt->nodeName()<<":"<<tmr->ifIndex
                   <<" "<<nd->simTime()<<" Fast RA not sent as no link-layer"
                   <<" addr opt in rtrSol from "<<dgram->srcAddress());

            break;
          }
        }
        else
        {
          Dout(dc::router_disc|dc::debug, rt->nodeName()<<":"<<tmr->ifIndex
               <<" "<<nd->simTime()<<" maxFastRAS="<<ie.rtrVar.maxFastRAS
               <<" exceeded. Waiting for unsolicited RA to be sent before "
               <<" resetting fastRACounter at sendUnsolRtrAd");
        }
      }
#endif //FASTRA

      ///According to algorithm spec in Sec. 6.2.6
      if ( msg->arrivalTime() <= nd->simTime() + delay)
        break;
      //previous multicast adv. (un/sol) within last MIN_DELAY_BETWEEN_RAS
      if (msg->sendingTime() >= nd->simTime() - MIN_DELAY_BETWEEN_RAS)
      {
        delay = delay + MIN_DELAY_BETWEEN_RAS;
        //subtract the elapsed time
        delay = delay - (nd->simTime() - msg->sendingTime());
        nd->cancelEvent(msg);
        nd->scheduleAt(nd->simTime() + delay, msg);
        break;
      }
      else
      {
        nd->cancelEvent(msg);
        nd->scheduleAt(nd->simTime() + delay, msg);
        break;
      }
/*   //Previous version
      if (msg->arrivalTime() < msg->sendingTime() + delay)
        //The next scheduled time is fine so just wait till then to send a rtrAdv
        break;
        //last adv was sent within MIN_DELAY_BETWEEN_RAS so rate limiting to one
        //adv for every MIN_DELAY_BETWEEN_RAS seconds
      else if (nd->simTime() < msg->sendingTime() + MIN_DELAY_BETWEEN_RAS )
      {
        delay = delay + MIN_DELAY_BETWEEN_RAS;
         nd->cancelEvent(msg);
         nd->scheduleAt(nd->simTime() + delay, msg);
         break;
      }
      //No need to rate limit just send it slightly earlier than the
      //previous scheduled unsolicited adv
      else
      {
        nd->cancelEvent(msg);
        nd->scheduleAt(nd->simTime() + delay, msg);
        break;
      }
*/
    }

  //Update neighbour cache if cond is met
  if (found && dgram->srcAddress() != IPv6_ADDR_UNSPECIFIED)
  {
#if defined TESTIPv6 || defined DEBUG_RTRADV
    cout <<rt->nodeName()<<":"<<tmr->ifIndex<<" "<<nd->simTime()
         <<" SolRtrAdv sendingTime: "<<setprecision(4)<<msg->sendingTime()
         <<" arivalTime: "<<setprecision(6)<<msg->arrivalTime()
         <<" nextAdvDelay:"<<setprecision(6)<<delay<<"\n";
#endif //#if defined TESTIPv6 || defined DEBUG_RTRADV
    Dout(dc::debug, rt->nodeName()<<":"<<tmr->ifIndex<<" "<<nd->simTime()
         <<" SolRtrAdv sendingTime: "<<setprecision(4)<<msg->sendingTime()
         <<" arivalTime: "<<setprecision(6)<<msg->arrivalTime()
         <<" nextAdvDelay:"<<setprecision(6)<<delay);

    NeighbourEntry* ngbr = rt->cds->neighbour(dgram->srcAddress()).lock().get();
    if (ngbr != 0)
    {
      if (rtrSol->hasSrcLLAddr() && rtrSol->srcLLAddr() != ngbr->linkLayerAddr())
      {
        ngbr->setLLAddr(rtrSol->srcLLAddr());
        ngbr->setState(NeighbourEntry::STALE);
        Dout(dc::neighbour_disc|dc::debug, rt->nodeName()<<":"<<dgram->inputPort()
             <<" "<<nd->simTime()<<" updated ngbr="<<dgram->srcAddress()
             <<" state=STALE & srcLLAddr="<<rtrSol->srcLLAddr());
      }
    }
    else
    {
      //Create new Neighbour Entry
      ngbr = new NeighbourEntry(dgram->srcAddress(), dgram->inputPort(),
                                rtrSol->hasSrcLLAddr()?
                                rtrSol->srcLLAddr().c_str():0,
                                NeighbourEntry::STALE);
      rt->cds->insertNeighbourEntry(ngbr);
      Dout(dc::neighbour_disc|dc::debug, rt->nodeName()<<":"<<dgram->inputPort()
           <<" "<<nd->simTime()<<" created ngbr="<<dgram->srcAddress()
           <<" state=STALE & srcLLAddr="<<(rtrSol->hasSrcLLAddr()?
                                           rtrSol->srcLLAddr():""));
    }

    //If this condition is violated then need common function to change the
    //Router Entry to a neighbour entry.  As Routers changing to hosts has not
    //implemented can leave this here to verify this assumption.
    assert(!ngbr->isRouter());
  }

  if (!found)
  {
    Interface6Entry& ie = rt->getInterfaceByIndex(dgram->inputPort());
    if (!ie.inetAddrs.empty() && ie.rtrVar.advSendAds)
    {
      DoutFatal(dc::core, "Shouldn't be here ("<<FILE_LINE<<"), at all");
      ///Not possible to reach here ?
    }
    else
    {
      Dout(dc::debug, rt->nodeName()<<":"<<dgram->inputPort()<<" "<<setprecision(4)
           <<nd->simTime()<<(ie.rtrVar.advSendAds?
                             " Not ready for RtrSol yet":
                             " Iface not advertising"));
      //Just ignore sol as we're not ready for it yet or we are not an
      //advertising interface ( but another interface is)
    }
  }
  //rtrSol Deleted by caller processRtrSol
}

/**
   RFC 2461 8.2

   A redirect message is sent to src of dgram to notify it to send packets
   destined for dest of dgram to use nextHop as the nextHop addr.

   @param theDgram was the original packet about to be forwarded through this
   router.

   @param nextHop is the new forwarding address to send to in future and will
   most likely come from routing protocols or a static routing table

   @param redirected out parameter is true if redirected successfully
*/

void  NDStateRouter::sendRedirect(IPv6Datagram* theDgram, const ipv6_addr& nextHop, bool& redirected)
{
  OPP_Global::ContextSwitcher switcher(nd);

  //std::auto_ptr<IPv6Datagram> dgram(theDgram);
  IPv6Datagram* dgram = theDgram;

  //Check packet is not src routed through router? If called from
  //forwardcore only should be safe because source routing is done before
  //reaching this step

  //@todo Should be rate limited too

  NeighbourEntry* ne, *redirNE = 0;
  size_t ifIndex = dgram->inputPort();
  const Interface6Entry& ie = rt->getInterfaceByIndex(ifIndex);

  if ((ne = rt->cds->neighbour(dgram->srcAddress()).lock().get()) != 0 &&
      ne->ifIndex() == ifIndex)
    if (!dgram->destAddress().isMulticast())
      if ((nextHop & IPv6_ADDR_LINK_LOCAL_PREFIX) == IPv6_ADDR_LINK_LOCAL_PREFIX ||
          dgram->destAddress() == nextHop)
      {
        //Check to see if the nextHop's link layer address is available and
        //include it in redirect message if it is
        redirNE = rt->cds->neighbour(nextHop).lock().get();
        if (redirNE !=0 && redirNE->linkLayerAddr() == "")
          redirNE = 0;

        IPv6Datagram* redirect = new IPv6Datagram(ie.inetAddrs[0],
                                                  dgram->srcAddress());

        redirect->setHopLimit(NDHOPLIMIT);

        if (redirNE && redirNE->isRouter())
          assert( (nextHop & IPv6_ADDR_LINK_LOCAL_PREFIX) ==
                  IPv6_ADDR_LINK_LOCAL_PREFIX);
        else if (redirNE) //Not a router
          assert( nextHop == dgram->destAddress());

        redirect->encapsulate(
          //new ICMPv6NDMRedirect(dgram->destAddress(), nextHop, dgram->dup(),
          new ICMPv6NDMRedirect(dgram->destAddress(), nextHop, dgram,
                                redirNE != 0 ? redirNE->linkLayerAddr():0));
        redirect->setTransportProtocol(IP_PROT_IPv6_ICMP);
        //TODO Unicast send required with addr res ability

        if (ne->linkLayerAddr() == "")
        {
          Dout(dc::warning|flush_cf,
               " Trying to send redirect to peer without their link layer addr "
               <<"(looks like incorrect use of redirect function)");
          delete redirect;
          redirected = false;
          return;
        }

/* XXX code change to use control info --AV
        LLInterfaceInfo info = {redirect, ne->linkLayerAddr()};
        nd->sendDirect( new LLInterfacePkt(info), 0,
                        outputMod[ifIndex], outputUnicastGate);
*/
        // XXX TBD factor out next 4 lines into separate sendXXXX() function
        LL6ControlInfo *ctrlInfo = new LL6ControlInfo();
        ctrlInfo->setDestLLAddr(ne->linkLayerAddr().c_str());
        redirect->setControlInfo(ctrlInfo);
        nd->sendDirect(redirect, 0, outputMod[ifIndex], outputUnicastGate); // XXX why sendDirect? -AV

        Dout(dc::forwarding|dc::debug|flush_cf, rt->nodeName()<<":"<<ifIndex
             <<" "<<nd->simTime()<<" Redirect sent to "<<dgram->srcAddress()
             <<" for "<<dgram->destAddress());

        nd->ctrIcmp6OutRedirect++;
        rt->ctrIcmp6OutMsgs++;
        redirected = true;
        return;
      }
  redirected = false;
}

ICMPv6NDMRtrAd* NDStateRouter
::createRA(const Interface6Entry::RouterVariables& rtrVar, size_t ifidx)
{
  ICMPv6NDMRtrAd* rtrAd = new ICMPv6NDMRtrAd(static_cast<unsigned int>(rtrVar.advDefaultLifetime),
                                             rtrVar.advCurHopLimit,
                                             rtrVar.advReachableTime,
                                             rtrVar.advRetransTmr,
                                             rtrVar.advPrefixList,
                                             rtrVar.advManaged,
                                             rtrVar.advOther);
  return rtrAd;
}

IPv6Datagram* NDStateRouter
::createDatagram(unsigned int ifIndex, const ipv6_addr& destAddr)
{

  Interface6Entry& ie = rt->getInterfaceByIndex(ifIndex);

  const Interface6Entry::RouterVariables& rtrVar = ie.rtrVar;

  ICMPv6NDMRtrAd* rtrAd = createRA(rtrVar, ifIndex);

  rtrAd->setSrcLLAddr(ie.LLAddr());

  //Source is link local addr (It is always the first address to be assigned)
  IPv6Datagram* dgram = new IPv6Datagram(ie.inetAddrs[0], destAddr);

  dgram->setHopLimit(NDHOPLIMIT);

  dgram->encapsulate(rtrAd);
  //XXX TBD dgram->setTransportProtocol(???);
  dgram->setName(rtrAd->name());

#ifdef USE_HMIP
  Debug(
    if (std::string(rt->nodeName()).find("ar") != std::string::npos)
    Dout(dc::debug, rt->nodeName()<<":"<<ifIndex<<" is this called for all ifaces ? "
         <<transitMapOpts.size());
  );

  for ( MAPIt mapIt = transitMapOpts.begin(); mapIt != transitMapOpts.end(); mapIt++ )
  {
      //Finally found the bit about propagation. Like propagate MAP only on
      //configured interfaces 7.1.2. This condition below was from previous
      //impl is totally wrong. In fact ifaceIdx should not be recorded because
      //we are not interested in what iface the map option was sent from
      //original MAP.
//    if ((*mapIt).ifaceIdx() == ifIndex)
      rtrAd->addOption(*mapIt);
      Dout(dc::debug, rt->nodeName()<<":"<<ifIndex<<" added map option to RA "
           <<(*mapIt).addr());
  }
#endif // USE_HMIP

  return dgram;
}


#ifdef USE_HMIP
/*
  Delete only the ones that are no longer advertised by other routers instead of
  deleting every time we send a router adv and hoping other parties send more
  frequent RA to us.
*/
std::auto_ptr<RA> NDStateRouter::processRtrAd(std::auto_ptr<RA> old_rtrAdv)
{
  if ( !old_rtrAdv.get() || !old_rtrAdv->hasMapOptions())
    return old_rtrAdv;

  HierarchicalMIPv6::MAPOptions mapOpts = old_rtrAdv->mapOptions();

  Debug(
    IPv6Datagram* dgram = old_rtrAdv->encapsulatedMsg();
    assert(dgram);
    unsigned int ifIndex = dgram->inputPort();
    // Dout(dc::debug, rt->nodeName()<<":"<<ifIndex<<" "<<nd->simTime()
    //      <<" Map options found size="<<mapOpts.size());
  );

  for ( MAPIt mapIt = mapOpts.begin(); mapIt != mapOpts.end(); mapIt++ )
  {

    bool found = false;
    if ( transitMapOpts.size() )
    {
      for ( MAPIt transMapIt = transitMapOpts.begin();
              transMapIt != transitMapOpts.end(); transMapIt++)
      {
        if ((*mapIt).addr() == (*transMapIt).addr())
        {
          found = true;
          break;
        }
      }
    }
    if ( !found )
    {
      (*mapIt).setDist((*mapIt).dist()+1);
      transitMapOpts.push_back(*mapIt);
      Dout(dc::debug, rt->nodeName()<<" "<<nd->simTime()
           <<" map options added "<<HierarchicalMIPv6::HMIPv6MAPEntry(*mapIt));
    }
  }
  return old_rtrAdv;
}
#endif //USE_HMIP

}//end namespace IPv6NeighbourDiscovery
