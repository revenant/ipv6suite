//
// Copyright (C) 2001, 2003, 2004 CTIE, Monash University
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
   @file NDStateHost.cc
   @brief Implementation of NDStateHost class. Implements Neighbour Discovery
   mechanisms defined in RFC2461 and AutoConf in RFC2462 for a host.

   @author Johnny Lai
   @date 24.9.01
*/

#include "sys.h"
#include "debug.h"


#include <iomanip> //setprecision
#include <climits> //UINT_MAX
#include <memory>  //auto_ptr
#include <cassert>
#include <iostream>
#include <boost/cast.hpp>
#include <boost/bind.hpp>
#include <sstream>

#include "cSignalMessage.h"

#include "NDStateHost.h"
#include "NDTimers.h"
#include "NeighbourDiscovery.h"
#include "ICMPv6Message_m.h"
#include "ICMPv6NDMessage.h"
#include "RoutingTable6.h"
#include "ExpiryEntryListSignal.h" //rt->pel/rel
#include "IPv6InterfaceData.h"
#include "InterfaceTableAccess.h"
#include "IPv6Datagram.h"
#include "ipv6_addr.h"
#include "cTimerMessage.h"
#include "opp_utils.h"
#include "IPv6CDS.h"

#ifdef USE_MOBILITY
//for awayfromhome check for fastrs
#include "MIPv6CDSMobileNode.h" 
#endif //USE_MOBILITY

#include "NotificationBoard.h" //for rtp handover l2 down signal
#include "NotifierConsts.h"

const int Tmr_L2Trigger = 8009;


namespace
{
  const size_t ADDR_CONF_TIME = 2*3600; //2 hours 5.5.3 e-1
  typedef cSignalMessage HostTmrMsg;
  typedef cSignalMessage RtrSolRetry;
}

namespace IPv6NeighbourDiscovery
{

const short MAX_RTR_SOLICITATIONS = 3;
const double RTR_SOLICITATION_INTERVAL = 4.0;
const double MAX_RTR_SOLICITATION_DELAY = 1.0;


size_t NDStateHost::addrResGate = UINT_MAX;

/**
   Use RoutingTable6 object to obtain administratively assigned
   information.

   Routers: Adv? Managed? What prefixes to advertise?

   Hosts: Use any assigned global IP addresses
*/
NDStateHost::NDStateHost(NeighbourDiscovery* mod)
  :NDState(mod), rt(mod->rt), stateEntered(false), addrResln(0),
   rtrSolicited(0), managedFlag(false), otherFlag(false)
{
  //Node is already initialised
  if (nextState != 0)
    return;

  ift = InterfaceTableAccess().get();

  addrResln = OPP_Global::findModuleByName(nd, "addrResln"); // XXX try to get rid of pointers to other modules --AV
  assert(addrResln != 0);
  if (!addrResln)
    DoutFatal(dc::core|error_cf, "Cannot find Address Resolution module");

  // Initialise this magic number once only as its constant during runtime
  if (addrResln != 0 && addrResGate == UINT_MAX)
  {
    addrResGate = addrResln->findGate("ICMPIn");
  }

  // join all-node multicast group
  IPv6Address node_multicast(ALL_NODES_NODE_ADDRESS);
  IPv6Address link_multicast(ALL_NODES_LINK_ADDRESS);
  IPv6Address site_multicast(ALL_NODES_SITE_ADDRESS);
  rt->joinMulticastGroup(node_multicast);
  rt->joinMulticastGroup(link_multicast);
  rt->joinMulticastGroup(site_multicast);

  ifStats.resize(ift->numInterfaceGates());

  rtrSolicited = new bool[ift->numInterfaceGates()];
  std::fill(rtrSolicited, rtrSolicited + ift->numInterfaceGates(), false);

  cSignalMessage* init = new cSignalMessage("InitialiseNode", Ctrl_NodeInitialise);
  init->connect(boost::bind(&NDStateHost::nodeInitialise, this));
  timerMsgs.push_back(init);
  init->rescheduleDelay((simtime_t) nd->par("startTime"));
/*
  cerr<<" sizeof cSignalMessage "<<sizeof(cSignalMessage)
      <<" sizeof cCallbackMessage="<<sizeof(cCallbackMessage)
      <<" sizeof callback="<<sizeof(boost::function<void (void)>)
      <<" cTTimerMessage"<<sizeof(cTTimerMessageA<void, IPv6NeighbourDiscovery::NDStateHost, IPv6NeighbourDiscovery::NDTimer>)<<" cTimerMessage"<<sizeof(cTimerMessage)<<" cMessage"<<sizeof(cMessage)<<" cObject"<<sizeof(cObject)<<endl;;
*/
}

NDStateHost::~NDStateHost()
{
  delete [] rtrSolicited;

  //Shouldn't need to cancel messages as we are quiting --> YES you need (Andras)
  for (TMI it = timerMsgs.begin(); it != timerMsgs.end(); it++)
    {
      nd->cancelEvent(*it);
      delete *it;
    }

  removeAllCallbacks();
}

std::auto_ptr<ICMPv6Message> NDStateHost::processMessage(std::auto_ptr<ICMPv6Message> msg)
{

  switch(msg->type() )
  {
      case ICMPv6_ROUTER_SOL:
        ///silently discard packet
        cout <<rt->nodeName()<<"- host received a router sol"<<endl;
        msg.reset();
        nd->ctrIcmp6InRtrSol++;

        break;
      case ICMPv6_ROUTER_AD:
        msg.reset((processRtrAd(OPP_Global::auto_downcast<IPv6NeighbourDiscovery::ICMPv6NDMRtrAd> (msg))).release());
        nd->ctrIcmp6InRtrAdv++;

        break;
      case ICMPv6_NEIGHBOUR_SOL:

        processNgbrSol(OPP_Global::auto_downcast<IPv6NeighbourDiscovery::ICMPv6NDMNgbrSol>(msg));
        nd->ctrIcmp6InNgbrSol++;

        break;

      case ICMPv6_NEIGHBOUR_AD:
        processNgbrAd(OPP_Global::auto_downcast<IPv6NeighbourDiscovery::ICMPv6NDMNgbrAd>(msg));
        nd->ctrIcmp6InNgbrAdv++;

        break;
      case ICMPv6_REDIRECT:
        processRedirect(OPP_Global::auto_downcast<IPv6NeighbourDiscovery::Redirect> (msg));
        nd->ctrIcmp6InRedirect++;

        break;
      default:
        cerr << "Unknown ND message type received " << msg->type() <<endl;
        Dout(dc::neighbour_disc|dc::notice|flush_cf, rt->nodeName()<<" "<<nd->simTime()
             <<" Unknown ND message type received"<< msg->type());
        msg.reset();
        break;
  }

  return msg;

}


// Protected Functions

void NDStateHost::enterState()
{
  assert(!stateEntered);
  stateEntered = true;
}

void NDStateHost::leaveState()
{
  ///Cancel all timers
}

///Called after link local address has been assigned.
void NDStateHost::initialiseInterface(size_t ifIndex)
{
  //For other tentative addr for interface that has assigned
  //link local addr.
  dupAddrDetOtherAddr(ifIndex);

  //Routers already have prefixes set by manual conf.
  if (!rt->isRouter())
  {
    //According to Sec. 6.3.7 host should still send Rtr Sol after receive Unsol
    //Rtr Ad as those may be incomplete. But in this sim it is always
    //complete. Change if sim becomes more realistic.
    if (!rtrSolicited[ifIndex]) //rt->getPrefixesByIndex(ifIndex).empty())
      sendRtrSol(0, ifIndex);
  }
  //Unnecessary to do prefix assignments at this time because processing of
  //router adv. will do that anyway
}

void NDStateHost::disableInterface(size_t ifIndex)
{}


///Return the link local address for this interface
IPv6Address NDStateHost::linkLocalAddr(size_t ifIndex)
{
  IPv6Address linkLocal;

  InterfaceEntry *ie = ift->interfaceByPortNo(ifIndex);

  // Search for LinkLocal address first as without a unique one can forget about
  // assigning other interfaces
  for (size_t k = 0; k < ie->ipv6()->tentativeAddrs.size(); k++)
  {
    ///Search for presence of manually assigned link local prefix
    if (IPv6Address(LINK_LOCAL_PREFIX).isNetwork(ie->ipv6()->tentativeAddrs[k]))
    {
      linkLocal = ie->ipv6()->tentativeAddrs[k];
      ifIndex = ie->outputPort();
      assert(ifIndex != (unsigned int) -1);
      ifStats[ifIndex].manualLinkLocal = true;
      break;
    }
  }

  //No link local addr manually assigned then auto conf one
  if (linkLocal == IPv6_ADDR_UNSPECIFIED)
  {
    IPv6Address lhs;
    lhs.setAddress(LINK_LOCAL_PREFIX);
    linkLocal = ipv6_addr_fromInterfaceToken(lhs, ie->interfaceToken());

    //Determine scope
    linkLocal.scope();

    ie->ipv6()->tentativeAddrs.push_back(linkLocal);
    Dout(dc::xml_addresses, "LLAddr "<<rt->nodeName()<<":"<<ifIndex<<" "
         <<linkLocal);
  }
  return linkLocal;
}

inline bool NDStateHost::linkLocalAddrAssigned(size_t ifIndex) const
{
  //The very first addr to be assigned is always the link local one
  return (ift->interfaceByPortNo(ifIndex)->ipv6()->inetAddrs.size() > 0);
}

bool  NDStateHost::globalAddrAssigned(size_t ifIndex) const
{
  InterfaceEntry *ie = ift->interfaceByPortNo(ifIndex);
  for ( unsigned int i = 0; i < ie->ipv6()->inetAddrs.size(); i++)
    if ( ie->ipv6()->inetAddrs[i].scope() == ipv6_addr::Scope_Global )
      return true;
  return false;
}

/**
   AutoConfiguration: test tentative link local address
 */
void NDStateHost::nodeInitialise()
{
  //fails to compile on some compilers !!!
  //TMI it = std::find_if(timerMsgs.begin(), timerMsgs.end(),
  //		boost::bind(&cTimerMessage::kind, _1) == Ctrl_NodeInitialise);

  for (TMI it = timerMsgs.begin(); it != timerMsgs.end();++it )
  {
    if ((*it)->kind() == Ctrl_NodeInitialise)
    {
      delete *it;
      timerMsgs.erase(it);      
      break;
    }
  }

  for(size_t ifIndex = 0; ifIndex < ift->numInterfaceGates(); ifIndex++)
  {
    NDTimer* tmr = new NDTimer;
    tmr->ifIndex = ifIndex;
    tmr->tentativeAddr = linkLocalAddr(ifIndex);
    dupAddrDetection(tmr);
  }
}

/**
   Prepare for DAD of tentative addr
*/
void NDStateHost::detectDupAddress(size_t ifIndex, const IPv6Address& tentativeAddr)
{
  ift->interfaceByPortNo(ifIndex)->ipv6()->tentativeAddrs.push_back(tentativeAddr);
  //Call func to send ns
  NDTimer* tmr = new NDTimer;
  tmr->ifIndex = ifIndex;
  tmr->tentativeAddr = tentativeAddr;
  dupAddrDetection(tmr);
}

///Do dupAddrDet on manually assigned addresses of interface except the link
///local addr which must have been assigned to reach this point
void NDStateHost::dupAddrDetOtherAddr(size_t ifIndex)
{
  //When this is done then do for every other addr when there's outstanding
  //tentativeAddr
  InterfaceEntry *ie = ift->interfaceByPortNo(ifIndex);

  IPv6Address tentativeAddr;

  //Dup Addr Det for each addr
  for (size_t j = 0; j < ie->ipv6()->tentativeAddrs.size(); j++)
  {
    tentativeAddr = ie->ipv6()->tentativeAddrs[j];

    //Call func to send ns
    NDTimer* tmr = new NDTimer;
    tmr->ifIndex = ifIndex;
    tmr->tentativeAddr = tentativeAddr;
    dupAddrDetection(tmr);
  }
}

/**
   Initiate DupAddrDet
   Resends the NgbrSol during DupAddrDet.
   When timeout expires will assign address.
 */
void NDStateHost::dupAddrDetection(NDTimer* tmr)
{
  InterfaceEntry *ie = ift->interfaceByPortNo(tmr->ifIndex);

  double delay = 0;

  if (tmr->msg == 0)
  {

    //Don't forget to do this otherwise ND will not work on tentative/assigned
    //address.  Can't delete solimcast_addr as joinMulticastGroup will take
    //ownership of pointer
    IPv6Address solimcast_addr(tmr->tentativeAddr.solNodeAddr());
    rt->joinMulticastGroup(solimcast_addr);

    if (ie->ipv6()->dupAddrDetectTrans < 1)
    {
      Dout(dc::ipv6|dc::neighbour_disc|dc::notice|flush_cf, rt->nodeName()<<":"<<tmr->ifIndex
           <<" skipped DupAddrDet "<< tmr->tentativeAddr << " assigned on "<<nd->simTime());

      //Don't do dup addr detect so just assign addr
      dupAddrDetSuccess(tmr);
      delete tmr;
      return;
    }

    NS* ns = new NS;

    tmr->dgram = new IPv6Datagram(IPv6Address(IPv6_ADDR_UNSPECIFIED), solimcast_addr);
    tmr->dgram->setHopLimit(NDHOPLIMIT);

    tmr->dgram->encapsulate(ns);
    tmr->dgram->setName(ns->name());
    tmr->dgram->setTransportProtocol(IPv6_PROT_ICMP);
    ns->setTargetAddr(static_cast<ipv6_addr>(tmr->tentativeAddr));

    if (rt->odad())
    {
      assert(!ie->ipv6()->tentativeAddrAssigned(tmr->dgram->srcAddress()));
      if (ie->ipv6()->tentativeAddrAssigned(tmr->dgram->srcAddress()))
      {
        Dout(dc::warning, rt->nodeName()<<":"<<tmr->ifIndex<<" cannot use a tentative address as source of NS (ODAD)");
	delete tmr;
        return;
      }
    }

    //last paragraph of 11.5.2 of mipv6 revision 24  says should not delay
    if (rt->odad() || rt->mobilitySupport() && rt->isMobileNode() && rt->mipv6cds->mipv6cdsMN->awayFromHome()
	//delay only for first addr assigned on iface as that's usually interface initialisation
	||  ie->ipv6()->inetAddrs.size() > 0)
      delay = 0;
    else
    {
#if FASTRS
      delay = uniform(0, ie->ipv6()->maxRtrSolDelay);
#else
      delay = uniform(0, MAX_RTR_SOLICITATION_DELAY);
#endif //FASTRS
    }

    tmr->dgram->setOutputPort(tmr->ifIndex);

    tmr->max_sends = ie->ipv6()->dupAddrDetectTrans; //Max Dup addr retries
    tmr->timeout = ie->ipv6()->retransTimer/1000.0; //Dup addr det timeout

    tmr->msg = new cSignalMessage("DupAddrDet", Tmr_DupAddrSolTimeout);
    ((cSignalMessage*)(tmr->msg))->connect(boost::bind(&NDStateHost::dupAddrDetection, this, tmr));
    tmr->msg->setContextPointer(tmr);
    timerMsgs.push_back(tmr->msg);
  }

  //Duplicate packet assuming dest will delete packet
  if (tmr->counter < tmr->max_sends)
  {

    Dout(dc::neighbour_disc|continued_cf, rt->nodeName()<<":"<<tmr->ifIndex
         <<" Tentative addr:"<<tmr->tentativeAddr);
    Dout(dc::finish, nd->simTime() <<" DupAddrDet sends: "<<  tmr->counter + 1
         <<" max:"<<tmr->max_sends<<" timeout:"<< setprecision(4) << tmr->timeout
         <<" initial delay:"<<delay);

    std::stringstream name;
    NS* ns = static_cast<NS*>(tmr->dgram->encapsulatedMsg());
    if (tmr->counter == 0)
      name<<ns->name()<<"(DAD) "<<tmr->counter+1<<"/"<<tmr->max_sends;
    else
    {
      std::string modify = ns->name();
      name<<tmr->counter + 1;
      modify[modify.size()-3] = name.str()[0];
      name.str(modify);      
    }
    ns->setName(name.str().c_str());
    tmr->dgram->setName(ns->name());

    //Send directly to IPv6Output as we need to send to a particular interface
    //which the routingCore can't determine from dest addr.
    nd->sendDelayed(tmr->dgram->dup(), delay, "outputOut", tmr->ifIndex);

    //would include address from which sent if IPv6Address was a cPolymorphic*
    nd->nb->fireChangeNotificationAt(nd->simTime() + delay, NF_IPv6_NS_SENT);

    tmr->counter++;

    nd->scheduleAt(nd->simTime() + delay + tmr->timeout, tmr->msg);
  }
  else
  {

    Dout(dc::ipv6|dc::neighbour_disc|dc::notice|flush_cf, rt->nodeName()<<":"<<tmr->ifIndex
         <<" tentative addr: "<<tmr->tentativeAddr<<" assigned on "<<nd->simTime());


    dupAddrDetSuccess(tmr);
    timerMsgs.remove(tmr->msg);
    //msg deletes tmr (_arg) internally
    delete tmr->msg;
    delete tmr;
  }
}


//private funcs

/**
   Assigns the address and initialise rest of interface once only

   @todo store the amount of time used for doing DAD and subtract from
   storedLifetime of assigned address for exact lifetimes
*/
void NDStateHost::dupAddrDetSuccess(NDTimer* tmr)
{
  //tmr->tentativeAddr->setStoredLifetime(tmr->tentativeAddr->storedLifetime() - 'DAD time');
  rt->assignAddress(tmr->tentativeAddr, tmr->ifIndex);

  //Need to do this only once for each interface after link local addr is
  //assigned (that is the first address to be assigned)
  if (!ifStats[tmr->ifIndex].initStarted)
  {
    //Do what this node should do after common initialisation is
    //accomplished. (once per node)
    if (!stateEntered)
      enterState();
    //May need to add a delay and call initialiseInterface otherwise
    //the node appears to do processing very quickly
    initialiseInterface(tmr->ifIndex);
    ifStats[tmr->ifIndex].initStarted = true;
  }

  nd->nb->fireChangeNotification(NF_IPv6_ADDR_ASSIGNED);

  if (rt->mobilitySupport() && rt->isMobileNode())
  {   
    InterfaceEntry *ie = ift->interfaceByPortNo(tmr->ifIndex);
    ipv6_addr potentialCoa = ie->ipv6()->inetAddrs[ie->ipv6()->inetAddrs.size()-1];
    invokeCallback(potentialCoa);
  }

}

///Send initial & subsequent retries of router solicitations
void NDStateHost::sendRtrSol(NDTimer* tmr, unsigned int ifIndex)
{
  OPP_Global::ContextSwitcher switchContext(nd);

  double delay = 0;

  InterfaceEntry *ie = ift->interfaceByPortNo(tmr?tmr->ifIndex:ifIndex);

  if (!tmr)
  {
    tmr = new NDTimer;
    tmr->max_sends = MAX_RTR_SOLICITATIONS;
    tmr->counter = 0;
    tmr->ifIndex = ifIndex;

    //Cannot be sure whether DAD was done b4 this step so just always delay. On
    //node startup only after link local addr assigned is this fn
    //called. However when moving to new subnet this gets called immediately and
    //not after DAD as we need to confirm whether this is a new network via RA
#if FASTRS
    delay = uniform(0, ie->ipv6()->maxRtrSolDelay);
#else
    delay = uniform(0, MAX_RTR_SOLICITATION_DELAY);
#endif // FASTRS

    tmr->dgram = new IPv6Datagram(
      ie->ipv6()->inetAddrs.empty()?IPv6Address(IPv6_ADDR_UNSPECIFIED):ie->ipv6()->inetAddrs[0],
      IPv6Address(ALL_ROUTERS_LINK_ADDRESS), 0, "RtrSol");

    tmr->dgram->setHopLimit(NDHOPLIMIT);

    RS* rs = new RS;
    if (!ie->ipv6()->inetAddrs.empty()
        || (rt->odad() && !ie->ipv6()->tentativeAddrAssigned(tmr->dgram->srcAddress())))
      rs->setSrcLLAddr(ie->llAddrStr());

    tmr->dgram->encapsulate(rs);
    tmr->dgram->setTransportProtocol(IPv6_PROT_ICMP);
    tmr->msg = new RtrSolRetry("RtrSolRetry", Tmr_RtrSolTimeout);
    ((RtrSolRetry*)(tmr->msg))->connect(boost::bind(&NDStateHost::sendRtrSol, this, tmr, tmr->ifIndex));
    tmr->msg->setContextPointer(tmr);

    timerMsgs.push_back(tmr->msg);
  }

#if FASTRS
    tmr->timeout = tmr->counter < tmr->max_sends - 1 ? RTR_SOLICITATION_INTERVAL:
      ie->ipv6()->maxRtrSolDelay;
#else
    tmr->timeout = tmr->counter < tmr->max_sends - 1 ? RTR_SOLICITATION_INTERVAL:
      MAX_RTR_SOLICITATION_DELAY;
#endif //FASTRS

  if (tmr->counter < tmr->max_sends)
  {
    Dout(dc::router_disc|flush_cf, rt->nodeName()<<":"<<tmr->ifIndex<<" "<<nd->simTime()
         <<" RtrSol sends: "<<  tmr->counter+1
         <<" max:"<<tmr->max_sends<<" timeout:"<< setprecision(6) << tmr->timeout
         <<" initial delay:"<<delay);

    std::stringstream name;
    RS* rs = static_cast<RS*>(tmr->dgram->encapsulatedMsg());
    if (tmr->counter == 0)
      name<<rs->name()<<" "<<tmr->counter+1<<"/"<<tmr->max_sends;
    else
    {
      std::string modify = rs->name();
      name<<tmr->counter + 1;
      modify[modify.size()-3] = name.str()[0];
      name.str(modify);
    }
    rs->setName(name.str().c_str());
    tmr->dgram->setName(rs->name());

    //Delay is non-zero for first rtrsol i.e. initial rtr sol
    nd->sendDelayed(tmr->dgram->dup(), delay, "outputOut", tmr->ifIndex);
    
    nd->nb->fireChangeNotificationAt(nd->simTime() + delay, NF_IPv6_RS_SENT);

    nd->ctrIcmp6OutRtrSol++;
    rt->ctrIcmp6OutMsgs++;

    ///Schedule a timeout
    tmr->msg->rescheduleDelay(tmr->timeout + delay);
    tmr->counter++;
    return;
  }
  else
  {
    Dout(dc::router_disc|dc::notice|flush_cf, rt->nodeName()<<":"<<tmr->ifIndex<<" "<<nd->simTime()
        <<" - No Routers responded to Solicitations");
    timerMsgs.remove(tmr->msg);
    delete tmr->msg;
    delete tmr;
  }


}

/*
  Unsolicited NA. ifIndex is interface to send on. Src address and target of NA
  will be the from target.
*/
void NDStateHost::sendUnsolNgbrAd(size_t ifIndex, const ipv6_addr& target)
{
  OPP_Global::ContextSwitcher switcher(nd);

  InterfaceEntry *ie = ift->interfaceByPortNo(ifIndex);

  assert(ie->ipv6()->addrAssigned(target));
  //Send NA that is
  ipv6_addr src = target;
  IPv6Datagram* dgram = new IPv6Datagram(src,
                                         c_ipv6_addr(ALL_NODES_LINK_ADDRESS));
  dgram->setHopLimit(NDHOPLIMIT);
  bool solicited = false;
  bool override = true;
  NA* na = new NA(src, ie->llAddrStr(), rt->isRouter(), solicited, override);
  dgram->encapsulate(na);
  dgram->setName(na->name());
  dgram->setTransportProtocol(IPv6_PROT_ICMP);
  nd->send(dgram, "outputOut", ifIndex);
  nd->nb->fireChangeNotification(NF_IPv6_NA_SENT);
}

///disc 6.3.4 and autoconf 5.5.3
std::auto_ptr<RA> NDStateHost::processRtrAd(std::auto_ptr<RA> rtrAdv)
{
  IPv6Datagram* dgram = check_and_cast<IPv6Datagram*>(rtrAdv->encapsulatedMsg());
  ipv6_addr srcAddr = dgram->srcAddress();
  size_t ifIndex = dgram->inputPort();

  RouterEntry* re = 0;

  assert(ifIndex < ift->numInterfaceGates());
  Dout(dc::router_disc|flush_cf, rt->nodeName()<<":"<<ifIndex<<" "<<nd->simTime()
       <<" received rtr adv "<<*rtrAdv);
  nd->nb->fireChangeNotification(NF_IPv6_RA_RECVD);

  if (!rtrSolicited[ifIndex])
  {
    //Don't want to set rtrSolicited when iface not ready and we receive Unsol
    //rtrAdv and we cannot configure the prefixes at that stage yet. Otherwise
    //we will never configure the prefixes until the next UnsolRtrAdv which
    //could be another 50-100 seconds too long for some apps that start at say
    //10 seconds. For MIPv6 it does not matter as much because Unsol are once
    //every 1-1.5 seconds
    if (linkLocalAddrAssigned(ifIndex))
    {
    //Remove RtrSol timeout from queue when RtrAdv received
    rtrSolicited[ifIndex] = true;
    for (TMI it = timerMsgs.begin(); it != timerMsgs.end(); it++)
      if ((*it)->kind() == Tmr_RtrSolTimeout &&
          ((NDTimer*)(check_and_cast<RtrSolRetry*>(*it)->contextPointer()))->ifIndex ==
          ifIndex)
      {
        nd->cancelEvent(*it);
	delete ((NDTimer*)(check_and_cast<RtrSolRetry*>(*it)->contextPointer()));
        delete *it;
        timerMsgs.erase(it);
        break;
      }
    }
  }

  bool reValid = false;

  if ((re = rt->cds->router(srcAddr).lock().get()) == 0 && rtrAdv->routerLifetime() != 0)
  {
    re = new RouterEntry(srcAddr, ifIndex,
                         rtrAdv->hasSrcLLAddr()?rtrAdv->srcLLAddr().c_str():0,
                         rtrAdv->routerLifetime());

    Dout(dc::router_disc|flush_cf, rt->nodeName()<<":"<<ifIndex<<" "<<nd->simTime()
         <<" Inserting new router entry from rtr adv "<<*re);

    rt->insertRouterEntry(re);
    reValid = true;
  }
  else if (re != 0)
  {
      if ( rtrAdv->routerLifetime() != 0)
      {
        OPP_Global::ContextSwitcher switcher(rt);
        //@todo Really should enforce that the two should occur together but
        //how without introducing RoutingTable into Prefix/RouterEntry.
        re->setInvalidTmr(rtrAdv->routerLifetime());
        rt->rel->addOrUpdate(re);
        //update llAddr if necessary
        if (rtrAdv->hasSrcLLAddr() && rtrAdv->srcLLAddr() != re->linkLayerAddr())
        {
          re->setLLAddr(rtrAdv->srcLLAddr());
          //Set state to STALE when LL addr changed
          re->setState(NeighbourEntry::STALE);
        }
        else if (rt->mobilitySupport() && rtrAdv->hasSrcLLAddr() && rtrAdv->srcLLAddr() == re->linkLayerAddr() &&
		 re->state() == NeighbourEntry::INCOMPLETE)
	{
	  //as MIPv6NDStateHost::relinquishRouter sets it to incomplete and now
	  //mn has moved back to prev visited subnet.
	  re->setState(NeighbourEntry::REACHABLE);
	}

        reValid = true;
	if (rt->mobilitySupport())
	{
        ///Recreate the DE if it was deleted when moving to different subnets
        if (rt->cds->neighbour(re->addr()).lock().get() == 0)
          (*rt->cds)[re->addr()] = DestinationEntry(rt->cds->router(srcAddr));

	Dout(dc::router_disc|flush_cf, " lhs="<<rt->cds->neighbour(re->addr()).lock()->addr()
	     <<" rhs="<<re->addr()<<" srcAddr="<<srcAddr<<" rt->cds->router(srcAddr)="
	     <<*(rt->cds->router(srcAddr).lock()));

	if (rt->cds->neighbour(re->addr()).lock()->addr() != re->addr())
	{
	  //possible when relinquishRouter not called yet and we receive Rtr
	  //Advertisement and previous router was not none. The neighbour that
	  //it is pointing to would have been the last default router. Fix would
	  //be to remove all link local addresses from dest cache upon moving to
	  //another subnet. but when do you know that happens for certain?
          (*rt->cds)[re->addr()] = DestinationEntry(rt->cds->router(srcAddr));	  
	}
	}
        Dout(dc::router_disc|flush_cf, rt->nodeName()<<":"<<ifIndex<<" "<<nd->simTime()
             <<" Updated router entry from RtrAd "<<*re);
      }
      else
      {
        Dout(dc::router_disc|flush_cf, rt->nodeName()<<":"<<ifIndex<<" "<<nd->simTime()
             <<" Removing router entry as adv lifetime is 0 "<<*re);
        //Follow 6.3.5
        rt->cds->removeRouterEntry(srcAddr);
        return rtrAdv;
      }
  }

  if (reValid)
    re->lastRAReceived = nd->simTime();

  //Check if link layer addr option exists
  if (rtrAdv->hasSrcLLAddr())
  {
    if (rt->mobilitySupport())
    {
    //Required to reinstate the router neighbour entry if we go back to prior subnets
    //as the router entry exists in the list but not in the Dest Cache neighbour pointer
    if ((*rt->cds)[srcAddr].neighbour.lock().get() == 0)
    {
      assert(rt->cds->router(srcAddr).lock().get());
      (*rt->cds)[srcAddr].neighbour =  rt->cds->router(srcAddr);
    }
    }

    //Check that isRouter flag is true in NE

    if (!(*rt->cds)[srcAddr].neighbour.lock().get()->isRouter())
    {

//      DoutFatal(dc::core|error_cf|flush_cf,  FILE_LINE
      Dout(dc::warning|flush_cf,  FILE_LINE
                <<rt->nodeName()<<":"<<ifIndex<<" isRouter Assumption "
                <<"was wrong for router "<<srcAddr<<" ngbr="
                <<(*rt->cds)[srcAddr].neighbour.lock()->addr());
      cerr<<FILE_LINE<<rt->nodeName()<<":"<<ifIndex<<" isRouter Assumption "
          <<"was wrong for router "<<srcAddr<<" ngbr="
          <<(*rt->cds)[srcAddr].neighbour.lock()->addr()<<endl;
      //This can happen if a host becomes a router
      //create router entry for it and remove old neighbour entry
      //Done already in insertRouterEntry

//HACK (FIXME do not know why this occurs since we do not have hosts changing to routers)
      (*rt->cds)[srcAddr].neighbour.lock().get()->setIsRouter(true);
      //boost::polymorphic_downcast<RouterEntry*> ((*rt->cds)[srcAddr].neighbour.lock().get());

    }
  }
  else
  {
    Dout(dc::router_disc|flush_cf, rt->nodeName()<<":"<<ifIndex<<" "<<nd->simTime()
         <<" Unhandled processRtrAdv see "<<FILE_LINE<<" "<<*re);

    //However for case of lladdr not exist then it is possible that router is in
    //router list but neighbour entry is host when host becomes router without
    //appending a ll option
    NeighbourEntry* ne = (*rt->cds)[srcAddr].neighbour.lock().get();
    if (ne && !ne->isRouter())
    {
      //TODO
      //delete old NE and replace with RE.
      //THe new NE is not reachable as we don't know the LL address
    }
  }

  // hop limit, reachabletime, retranstime
  InterfaceEntry *ie = ift->interfaceByPortNo(ifIndex);

  if (rtrAdv->curHopLimit() != 0 && rtrAdv->curHopLimit() != ie->ipv6()->curHopLimit)
    ie->ipv6()->curHopLimit = rtrAdv->curHopLimit();

  if (rtrAdv->reachableTime() != 0 && rtrAdv->reachableTime() != ie->ipv6()->baseReachableTime())
  {
    ie->ipv6()->setBaseReachableTime(rtrAdv->reachableTime());
    //TODO suppose to generate one once every 2 hours even if baseReachable time
    //is the same
  }

  if (rtrAdv->retransTimer() != 0 && rtrAdv->retransTimer() != ie->ipv6()->retransTimer)
    ie->ipv6()->retransTimer = rtrAdv->retransTimer();

  if (rtrAdv->MTU() != 0) //MTU exists
  {
    if (rtrAdv->MTU() >= IPv6_MIN_MTU) //&& < default LinkMTU for specific link type defined in IPv6-ether etc.
      ie->setMtu(rtrAdv->MTU());
  }

  LinkPrefixes prefixes =  rt->cds->getPrefixesByIndex(ifIndex);
  typedef LinkPrefixes::iterator LPI;
  PrefixEntry* prefix = 0;
  bool prefixFound = false;
  IPv6Address addr;
  //Min address conf lifetime
  unsigned int minStoredLifetime = 0;

  //for each prefix option if on-link set then add to prefix list or remove if
  //valid lifetime is 0
  for (size_t i = 0; i < rtrAdv->prefixCount(); i++, prefixFound = false,
         addr = IPv6Address(IPv6_ADDR_UNSPECIFIED))
  {
    if (IPv6Address(LINK_LOCAL_PREFIX).isNetwork(rtrAdv->prefixInfo(i).prefix))
      continue;

    const ICMPv6NDOptPrefix& prefOpt = rtrAdv->prefixInfo(i);

    if (prefOpt.onLink)
    {

      for (LPI it = prefixes.begin(); it != prefixes.end(); it++)
        if ((*it)->prefix() == prefOpt.prefix)
        {
          prefixFound = true;
          prefix = (*it);
          break;
        }

      if (prefixFound)
      {
        if (prefOpt.validLifetime > 0)
        {
          OPP_Global::ContextSwitcher switcher(rt);
          //Updating prefix
          //@todo Really should enforce the two calls should occur together but
          //how without introducing RoutingTable into Prefix/RouterEntry.
          prefix->setAdvValidLifetime(prefOpt.validLifetime);
          if (prefOpt.validLifetime < VALID_LIFETIME_INFINITY)
            rt->pel->addOrUpdate(prefix);
        }
        else
        {
          cerr<<*(rt->pel)<<endl;
          // Timeout lifetime according to 6.3.4 immediately
          rt->removePrefixEntry(prefix);
        }
      }
      else
      {
        if (prefOpt.validLifetime > 0)
        {
          rt->insertPrefixEntry(PrefixEntry(prefOpt), ifIndex);
          Dout(dc::prefix_timer, rt->nodeName()<<":"<<ifIndex<<" "<<nd->simTime()
               <<prefOpt<<" prefix inserted"<<dec);
        }
        else
        {
          Dout(dc::prefix_timer, rt->nodeName()<<":"<<ifIndex<<" "<<nd->simTime()
               <<" ignoring prefix (lifetime=0 and prefix unknown) "<<prefOpt<<dec);
        }
      }
    }


    //Process according to addr conf Sec. 5.5.3
    if (!prefOpt.autoConf)
    {
      Dout(dc::debug, rt->nodeName()<<" rtrAdv prefix no autoconf flag set "
           <<prefOpt.prefix);
      continue;
    }
    if (IPv6Address(LINK_LOCAL_PREFIX).isNetwork(prefOpt.prefix))
      continue;
    if (prefOpt.validLifetime == 0)
      continue;
    if (prefOpt.preferredLifetime > prefOpt.validLifetime)
    {
      cerr << rt->nodeName()<<":"<<ifIndex<<" RtrAdv received from "
           <<dgram->srcAddress()<<"with pref > valid lifetime"<<endl;
      Dout(dc::notice|dc::router_disc|flush_cf, rt->nodeName()<<":"<<ifIndex<<" "
           <<nd->simTime()<<" RtrAdv received from "
           <<dgram->srcAddress()<<"with pref > valid lifetime");
      continue;
    }

    //minStoredLifetime is used to store the shortest Lifetime update received
    //from this router advertisement.  This is used by RoutingTable to determine
    //if its necessary to reschedule the address expiry timer.
    if ((addr = ie->ipv6()->matchPrefix(prefOpt.prefix, prefOpt.prefixLen, true)) == IPv6_ADDR_UNSPECIFIED)
    {
      //Create a new address and add it to tentative Addr for DAD unless link
      //local address was assigned via address conf from unique Interface ID in
      //which case address is assigned directly
      if (prefixAddrConf(ifIndex, prefOpt.prefix, prefOpt.prefixLen,
                         prefOpt.preferredLifetime, prefOpt.validLifetime))
      {
/*
        Dout(dc::ipv6|dc::address_timer|flush_cf, rt->nodeName()<<":"<<ifIndex<<" "
             <<ie->ipv6()->inetAddrs[ie->ipv6()->inetAddrs.size()-1]<<" (maybe tentatively) assigned from prefixOpt="
             <<prefOpt.prefix<<"/"<<dec<<(int)prefOpt.prefixLen<<" at "
             <<nd->simTime());
*/
        if (minStoredLifetime == 0)
          minStoredLifetime = prefOpt.validLifetime;
        else
          minStoredLifetime = prefOpt.validLifetime < minStoredLifetime? prefOpt.validLifetime: minStoredLifetime;
      }
      else //Can log administrative error with Rtr prefix  option
      {
        Dout(dc::notice|dc::router_disc|flush_cf, rt->nodeName()<<":"<<ifIndex
             <<" "<<nd->simTime()<<"RtrAdv received from "<<dgram->srcAddress()
             <<" -  Unable to autoconf addr from prefix: Interface not ready or "
             <<"irreconcilable prefix length"<<prefOpt.prefixLen);
      }
    }
    else //Advertise prefix matches an assigned address
    {
      if (ie->ipv6()->tentativeAddrAssigned(addr))
        continue;

      //5.5.3 - 5.5.4
      if (prefOpt.validLifetime > ADDR_CONF_TIME ||
          prefOpt.validLifetime > addr.storedLifetime())
      {
        Dout(dc::address_timer, rt->nodeName()<<":"<<ifIndex<<"["<<i<<"]"<<"="
             <<addr<<" "<<rt->simTime()<<" matches prefix "<<prefOpt.prefix<<"/"
             <<prefOpt.prefixLen<<" from RA.  Updating lifetimes from rtr.pref="
             <<prefOpt.preferredLifetime<<" rtr.valid="<<prefOpt.validLifetime);

        //Specs don't say anything about updating preferredLifetime
        //so I guess this is a good guess.
        addr.setPreferredLifetime(prefOpt.preferredLifetime);
        addr.setStoredLifetimeAndUpdate(prefOpt.validLifetime);
      }
      else if (addr.storedLifetime() <= ADDR_CONF_TIME &&
               prefOpt.validLifetime <= addr.storedLifetime())
      {
        Dout(dc::address_timer, rt->nodeName()<<":"<<ifIndex<<"["<<i<<"]"<<"="<<addr<<" "<<rt->simTime()
             <<": In clause dictated by storedLifetime < ADDR_CONF_TIME="<<ADDR_CONF_TIME
             <<" and prefOpt.validLifetime < addr->storedLifetime. Supposed to set to "
             <<" storedLifetime if authenticated");
        continue;
      }

      else
      {
        Dout(dc::address_timer, rt->nodeName()<<":"<<ifIndex<<"["<<i<<"]"<<"="<<addr<<" "<<rt->simTime()
             <<" setting storedLifetime to ADDR_CONF_TIME="<<ADDR_CONF_TIME);
        addr.setStoredLifetimeAndUpdate(ADDR_CONF_TIME);
      }

      Dout(dc::address_timer, "addr taken from "<<addr<<" stored="<<addr.storedLifetime());

      if (minStoredLifetime == 0)
        minStoredLifetime = addr.storedLifetime();
      else
        minStoredLifetime = addr.storedLifetime() < minStoredLifetime? addr.storedLifetime(): minStoredLifetime;

    }

  } //end for loop of router adv prefixes

  if (minStoredLifetime > 0 && minStoredLifetime < VALID_LIFETIME_INFINITY)
    rt->rescheduleAddrConfTimer(minStoredLifetime);

  ///5.5.3

  //Not running stateful (is this a host thing or per interface?)
  if (rtrAdv->managed() && !managedFlag)
  {
    managedFlag = true;
    //Start stateful address conf
  }

  if (rtrAdv->other() && !otherFlag)
  {
    otherFlag = true;
    if (!managedFlag)
    {
      //Just invoke stateful for other configuration excluding addresses
    }
  }
  return rtrAdv;
}

/**
   @brief    auto configure an address from a multicast rtr adv prefix

   @note    The address will go through DAD always according to RFC4862

   @return true if the prefix is consistent with this interface i.e. prefix
   length and interfaceID length equals IPv6_ADDR_BITLENGTH.
   false if it doesn't or no link local addr assigned yet

*/
bool  NDStateHost::prefixAddrConf(size_t ifIndex, const ipv6_addr& prefix,
                                  size_t prefix_len, unsigned int preferredLifetime,
                                  unsigned int storedLifetime)
{
  InterfaceEntry *ie = ift->interfaceByPortNo(ifIndex);
  assert(storedLifetime != 0);
  //Create a new address and add it to tentative if link local address was
  //an addr conf addr from unique Interface ID
  if (prefix_len + ie->interfaceToken().length() == IPv6_ADDR_BITLENGTH )
  {
    //Can't autoconf other addresses till link local done (implementation
    //requires that link local be first address assigned not part of spec).
    //Just wait till link local addr assigned as host will solicit router
    if (!linkLocalAddrAssigned(ifIndex))
      //received an unsolicated rtr adv or other nodes solicited this adv
      return false;

    ipv6_addr newAddr = ipv6_addr_fromInterfaceToken(prefix, ie->interfaceToken());

    IPv6Address addrObj(newAddr);
    addrObj.setPrefixLength(prefix_len);
    addrObj.setStoredLifetimeAndUpdate(storedLifetime);
    addrObj.setPreferredLifetime(preferredLifetime);

      //Do not do DAD on outstanding DAD which is indicated by been in
      //tentativeAddrs. Only when interface initialises is there already
      //stuff in tentativeAddrs
      if (ie->ipv6()->tentativeAddrAssigned(addrObj))
      {
        return true;
      }

      if (rt->mobilitySupport() && rt->isMobileNode())// && rt->awayFromHome())
      {
	Dout(dc::ipv6|dc::address_timer|flush_cf, rt->nodeName()<<":"<<ifIndex<<" "
             <<addrObj<<" undergoing DAD prefix="
             <<prefix<<"/"<<dec<<(int)prefix_len<<" at "
             <<nd->simTime());    
      }

      detectDupAddress(ifIndex, addrObj);
      return true;
  }
  return false;
}

bool NDStateHost::removeTentativeAddress(InterfaceEntry* ie, ipv6_addr addr)
{
  
  for(TMI it = timerMsgs.begin(); it != timerMsgs.end(); it++)
  {
    //cout << rt->nodeName()<<":"<<recDgram->inputPort()<<" "<<(*it)->name()<<endl;
    if((*it)->kind() == Tmr_DupAddrSolTimeout && ie->outputPort() ==
       static_cast<int> ((static_cast<NDTimer*>((*it)->contextPointer()))->ifIndex) &&
       (static_cast<NDTimer*>((*it)->contextPointer()))->tentativeAddr == addr)
    {
      // cancel duplicate detect timer message

      //Should be scheduled if it isn't scheduled then message shouldn't
      //be in queue (forgot to remove) or we are matching against wrong
      //timeout msg
      assert((*it)->isScheduled());

      IPv6Address tentativeAddr = 
	(static_cast<NDTimer*>((*it)->contextPointer()))->tentativeAddr;

      Dout(dc::ipv6|address_timer|flush_cf, rt->nodeName()<<":"<<ie->outputPort()
	   <<" CANCEL "<<(*it)->name()<<" timer message for tentative addr "
	   <<tentativeAddr);

      ie->ipv6()->removeTentativeAddress(tentativeAddr);

      delete nd->cancelEvent(*it);
      delete (NDTimer*) (*it)->contextPointer();
      timerMsgs.erase(it);
      
      //Found a duplicate tentative address and treated accordingly so
      //stop further processing
      return true;
    }
  }
  return false;
}

/**
 * @brief test received Neighbour solicitations' and advertisements'
 * target address is a tentative address of this node and process them
 * accordingly Sec. 5.4.3-5 inclusive of RFC 2462.
 *
 *  @param targetAddr is the target address from the NS or NA.
 *  @param recDgram is the datagram that contained the NS or NA
 */
bool NDStateHost::checkDupAddrDetected(const ipv6_addr& targetAddr, IPv6Datagram* recDgram)
{
  InterfaceEntry *ie = ift->interfaceByPortNo(recDgram->inputPort());

  if (ie->ipv6()->tentativeAddrAssigned(targetAddr))
  {
    //Check if NgbrSol/NgbrAd signifies a dup address is detected
    //remove addr from tentative addr as detected duplicate
    //disable interface if necessary
    //Remove the scheduled tmr message from list and cancel further sends

      //Disable interface if no link local address has been assigned
      if (ie->ipv6()->inetAddrs.empty())
        disableInterface(recDgram->inputPort());

      rt->leaveMulticastGroup(
        IPv6Address(IPv6Address::solNodeAddr(targetAddr)));

      if (rt->odad() && !ifStats[recDgram->inputPort()].manualLinkLocal)
        Dout(dc::custom, rt->nodeName()<<":"<<recDgram->inputPort()<<" "<<nd->simTime()
             <<" ODAD @todo Should attmmpt address generation when failed dad");

      bool removed = removeTentativeAddress(ie, targetAddr);
      assert(removed);
      if (removed)
	Dout(dc::warning|flush_cf, rt->nodeName()<<":"<<recDgram->inputPort()
	     <<" duplicate address detected) "<<targetAddr);

  }

  return false;

}

void NDStateHost::processNgbrSol(std::auto_ptr<NS> msg)
{

  if (!valNgbrSol(msg.get()))
  {

    Dout(dc::neighbour_disc|dc::warning|flush_cf, rt->nodeName()
         <<" Discarded invalid NS");
    //Silently Discard sol Sec. 7.1.1
    return;
  }

  IPv6Datagram* recDgram = check_and_cast<IPv6Datagram*>(msg->encapsulatedMsg());

  InterfaceEntry *ie = ift->interfaceByPortNo(recDgram->inputPort());

  //check if dupAddrDetection failed (another node is doing dupAddrDetection on
  //a tentative addr)
  if(recDgram->srcAddress() == IPv6_ADDR_UNSPECIFIED)
  {
    if (checkDupAddrDetected(msg->targetAddr(), recDgram))
      return;
  }
  else  //unicast src address
  {
    //Another node performs addr resln on a tentative addr so we ignore
    for(size_t j = 0; j < ie->ipv6()->tentativeAddrs.size(); j++)
    {
      if((ipv6_addr)(ie->ipv6()->tentativeAddrs[j]) == msg->targetAddr())
      {
	if (!rt->odad())
	{
	  Dout(dc::neighbour_disc|dc::notice, rt->nodeName()
	       <<"Ignoring Ngbr Sol with target of "<<msg->targetAddr()
	       <<" as target is a tentative address.");
	  return;
	}
	else
	  Dout(dc::debug, rt->nodeName()<<":"<<recDgram->inputPort()<<" "<<nd->simTime()
	       <<" ODAD processNgbrSol responding to NS from unicast addr");

      }
    }
  }


  // TODO Check for NUD solicitation

  //Otherwise respond with NA should be done by forwarding orig message to Addr
  //Resln Module
  assert(addrResln != 0);
  nd->sendDirect(msg.release(), 0, addrResln, addrResGate);
}

///Rename to processDupAddrResp
void NDStateHost::processNgbrAd(std::auto_ptr<NA> ngbrAdv)
{

  if (!valNgbrAd(ngbrAdv.get()))
    return;

  IPv6Datagram* recDgram = check_and_cast<IPv6Datagram*>(ngbrAdv->encapsulatedMsg());

  //Check if NA comes from a node that has assigned our tentative addr
  //Determine if this was a dupAddrDet adv, or one response to an addr res req.
  if(!ngbrAdv->solicited())
    if (checkDupAddrDetected(ngbrAdv->targetAddr(), recDgram))
      return;


  // TODO Check for NUD solicitation

  //Otherwise proc of addr res responses should be done by forwarding orig
  //message to Addr Resln Module
  nd->sendDirect(ngbrAdv.release(), 0, addrResln, addrResGate);

}

bool NDStateHost::valNgbrAd(NA* ad)
{
  return true;
}

/**
   Validation routine for a recieved Neighbour Solicitation.
   Original datagram is assumed to be encapsulated inside this
 */
bool NDStateHost::valNgbrSol(NS* sol)
{
  IPv6Datagram* dgram = check_and_cast<IPv6Datagram*>(sol->encapsulatedMsg());

  assert(dgram->hopLimit() == NDHOPLIMIT);

  if (dgram->hopLimit() != NDHOPLIMIT)
    return false;

  //Authenticate header (if exists)

  //Checksum

  assert(sol->code() == 0);

  if (sol->code() != 0)
    return false;

  //ICMP length is 24 or more octets

  if (sol->targetAddr().isMulticast())
    return false;

  //All options have length > 0

  if (dgram->srcAddress() == IPv6_ADDR_UNSPECIFIED)
    if (IPv6Address(SOLICITED_NODE_PREFIX).isNetwork(dgram->destAddress()) &&
        !sol->hasSrcLLAddr())
      return true;
    else
      return false;

  return true;
}

bool NDStateHost::valRtrAd(RA* ad)
{
  return true;
}

bool NDStateHost::valRedirect(Redirect* re)
{
  return true;
}

/**
 * Check
 * @todo Ngbr->Rtr Transition required for proper functioning.  Test!!!
 *
 */
void  NDStateHost::processRedirect(std::auto_ptr<Redirect> msg)
{

  if (!valRedirect(msg.get()))
    return;

  NeighbourEntry* ne = rt->cds->neighbour(msg->targetAddr()).lock().get();

  if (ne != 0)
    ne->update(msg.get());
  else
  {
    //TODO create a static virtual ctor to create either a RE or NE
    //based on msg
    if (msg->targetAddr() != msg->destAddr())
    {
      ne = new RouterEntry(msg.get());
      rt->insertRouterEntry(static_cast<RouterEntry*> (ne));
      (*rt->cds)[msg->destAddr()].neighbour = rt->cds->router(ne->addr());
    }
    else
    {
      rt->cds->insertNeighbourEntry(new NeighbourEntry(msg.get()));
    }
    return;
  }

  if (msg->targetAddr() != msg->destAddr())
    //TODO Ngbr->Rtr Transition
    ne->setIsRouter(true);
}

#if OPTIMISTIC_DAD
/*
  opt dad Sec. 3.3

  @todo does not do the exp backoff if dad failed and attempting generateAddr
  again
  @note currently unused
 */
ipv6_addr NDStateHost::generateAddress(const ipv6_prefix& prefix) const
{
  ipv6_addr ret;
  ret.extreme=prefix.prefix.extreme;
  ret.high=prefix.prefix.high;
  ret.normal = OPP_Global::generateInterfaceId();
  ret.low = OPP_Global::generateInterfaceId();

  //set u and g bits to 0
  ret.normal &= 0xFFFcFFFF;
  return ret;
}
#endif //OPTIMISTIC_DAD

void NDStateHost::addCallbackToAddress(const ipv6_addr& tentativeAddr, cTimerMessage* cb)
{
  Dout(dc::mipv6|flush_cf, rt->nodeName()<<" addcb "<<cb->name()<<" tentative="<<tentativeAddr
       <<" size="<<addressCallbacks.size());
  //If we really need multiple callbacks then change value of map to a list
  assert(addressCallbacks.count(tentativeAddr) == 0);
  addressCallbacks[tentativeAddr] = cb;
}

bool NDStateHost::callbackAdded(const ipv6_addr& tentativeAddr, int message_id) 
{
  if (addressCallbacks.count(tentativeAddr))
  {
    cTimerMessage* cb = addressCallbacks[tentativeAddr];
    return (cb->kind() == message_id);
  }
  return false;
}

void NDStateHost::invokeCallback(const ipv6_addr& tentativeAddr)
{
  if (addressCallbacks.count(tentativeAddr))
  {
    //assert(addressCallbacks.count(tentativeAddr));
    Dout(dc::debug, rt->nodeName()<<" Invoking callback for address "<< tentativeAddr);
    cTimerMessage* cb = addressCallbacks[tentativeAddr];
    cb->callFunc();
    delete cb;
    addressCallbacks.erase(tentativeAddr);
  }
}

cTimerMessage*  NDStateHost::addressCallback(const ipv6_addr& tentativeAddr)
{
  if (!addressCallbacks.count(tentativeAddr))
    return 0;
  return addressCallbacks[tentativeAddr];
}

int NDStateHost::removeAllCallbacks()
{
  int ret = addressCallbacks.size();
  typedef std::map<ipv6_addr, cTimerMessage*>::iterator AIT;
  for (AIT it = addressCallbacks.begin(); it != addressCallbacks.end(); it++)
  {
    if (it->second->isScheduled())
      it->second->cancel();
    delete it->second;
  }
  addressCallbacks.clear();
  return ret;
}

int NDStateHost::removeCallbacks(const ipv6_addr& addr)
{
  int ret = 0;
  typedef std::map<ipv6_addr, cTimerMessage*>::iterator AIT;
  for (AIT it = addressCallbacks.begin(); it != addressCallbacks.end(); it++)
  {
    if (it->first != addr)
      continue;
    if (it->second->isScheduled())
      it->second->cancel();
    delete it->second;
    ++ret;
  }
  if (ret > 0)
    addressCallbacks.erase(addr);
  return ret;
}

} //namespace IPv6NeighbourDiscovery

//  LocalWords:  multicast sim addr
