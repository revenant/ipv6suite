//
// Copyright (C) 2002, 2004, 2005 CTIE, Monash University
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
    @file IPv6Forward.cc
    @brief Forwarding of packets based on next hop prescribed by RoutingTable6

    -Responsibilities:
        -Receive valid IPv6 datagram
        -send datagram with Multicast addr. to Multicast module
        -Forward to IPv6Encapsulation module if next hop is a tunnel entry point
        -Drop datagram away and notify ICMP if dest. addr not in routing table
        -send to local Deliver if dest. addr. = 127.0.0.1
         or dest. addr. = NetworkCardAddr.[]
         otherwise, send to Fragmentation module
        -Dec Hop Limit when forwarding

    @author Johnny Lai
    @date 28/08/01
*/


#include "sys.h"
#include "debug.h"


#include <iomanip> //setprecision
#include <iterator> //ostream_iterator
#include <omnetpp.h>
#include <boost/cast.hpp>
#include <boost/bind.hpp>

#include "IPv6Forward.h"
#include "InterfaceTableAccess.h"
#include "IPv6InterfaceData.h"
#include "RoutingTable6Access.h"
#include "IPv6Datagram.h"
#include "ICMPv6Message_m.h"
#include "ICMPv6MessageUtil.h"
#include "opp_utils.h"  // for int/double <==> string conversions
#include "NDEntry.h"
#include "AddrResInfo_m.h"
#include "HdrExtRteProc.h"
#include "IPv6InterfaceData.h"
#include "IPv6CDS.h"

#ifdef USE_MOBILITY
#include "MIPv6CDSMobileNode.h"
#include "MIPv6Entry.h"
#include "HdrExtDestProc.h"
#include "MIPv6DestOptMessages.h"
#include "MIPv6MNEntry.h" //bu_entry
#include "IPv6Encapsulation.h"
#include "MIPv6MessageBase.h"
#include "MIPv6MStateCorrespondentNode.h"
#include "MIPv6MStateMobileNode.h"
#ifdef USE_HMIP
#include "HMIPv6CDSMobileNode.h"
#ifdef EDGEHANDOVER
#include "EHCDSMobileNode.h"
#include "HMIPv6Entry.h"
#endif //EDGEHANDOVER
#endif //USE_HMIP
#endif //#ifdef USE_MOBILITY

///For redirect. Its overkill but sendDirect would still require many mods too
///in terms of thinking how to pass a dgram in when it expects all to be
///icmp messages
#include "NDStateRouter.h"
#include "NeighbourDiscovery.h"

#ifdef CWDEBUG
#include <cassert>
#include <iostream>
#include <string>
#include <memory>
#endif //CWDEBUG

using boost::weak_ptr;

Define_Module( IPv6Forward );


void IPv6Forward::initialize(int stage)
{
  if (stage == 0)
  {
    QueueBase::initialize();
    ift = InterfaceTableAccess().get();
    rt = RoutingTable6Access().get();

    mob = boost::polymorphic_downcast<IPv6Mobility*>
      (OPP_Global::findModuleByType(rt, "IPv6Mobility"));
    assert(mob != 0);

    ctrIP6InAddrErrors = 0;
    ctrIP6OutNoRoutes = 0;

    routingInfoDisplay = par("routingInfoDisplay");
#ifdef USE_MOBILITY
    tunMod = check_and_cast<IPv6Encapsulation*>
      (OPP_Global::findModuleByName(this, "tunneling")); // XXX try to get rid of pointers to other modules --AV
    assert(tunMod != 0);

#endif //USE_MOBILITY
  }
  //numInitStages needs to be after creation of ND state in NeighbourDiscovery
  //which happens at 3rd stage
  else if (stage == numInitStages() - 1 && rt->isRouter())
  {
    //Get the nd pointer to ndstaterouter/host pointer
    cModule* procMod = parentModule();  // XXX try to get rid of pointers to other modules --AV
    cModule* icmp = procMod->submodule("ICMP");
    assert(icmp);
    cModule* ndMod = icmp->submodule("nd");
    assert(ndMod);
    NeighbourDiscovery* ndm = check_and_cast<NeighbourDiscovery*>(ndMod);
    assert(ndm);
    if (!ndm)
      DoutFatal(dc::core|error_cf, "Cannot find ND module");
    nd = boost::polymorphic_downcast<IPv6NeighbourDiscovery::NDStateRouter*>(ndm->getRouterState());
    assert(nd);
    if (!nd)
      DoutFatal(dc::core|error_cf, "Cannot find NDStateRouter module");
  }
}

/**
   @brief huge function to handle forwarding of datagrams to lower layers

   @todo When src of outgoing dgram is a tentative address, ODAD would forward
   dgram to router for unknown neighbour LL addr.
   @todo Multicast module will have to handle MIPv6 11.3.4 forwarding of multicast packets
*/

void IPv6Forward::endService(cMessage* theMsg)
{
  std::auto_ptr<IPv6Datagram> datagram(check_and_cast<IPv6Datagram*> (theMsg));

  assert(datagram->inputPort() < (int)ift->numInterfaceGates());

// {{{ localdeliver

  if (rt->localDeliver(datagram->destAddress()))
  {
    Dout(dc::forwarding|flush_cf, rt->nodeName()<<":"<<hex<<datagram->inputPort()<<dec<<" "
         <<simTime()<<" received packet from "<<datagram->srcAddress()
         <<" dest="<<datagram->destAddress());

    IPv6Datagram* copy = datagram->dup();

    //This condition can occur when upper layer originates packets without
    //specifying a src address destined for a multicast destination or local
    //destination so the packet is missing the src address. This is a bit
    //dodgy but the only other solution would be to enforce the app layer to
    //choose a src address.
    if (datagram->srcAddress() == IPv6_ADDR_UNSPECIFIED &&
        datagram->inputPort() == -1)
      copy->setSrcAddress(ift->interfaceByPortNo(0)->ipv6()->inetAddrs[0]);

    send(copy, "localOut");
    //Check if it is a multicast packet that we need to forward
    if (!datagram->destAddress().isMulticast())
      return;
  }

// }}}


// {{{ Process outgoing packet for MIP6 e.g. If binding exists for dest then swap dest==hoa to coa

//  Dout(dc::mipv6, rt->nodeName()<<" datagram->inputPort = " << datagram->inputPort());

  if (rt->mobilitySupport() && datagram->inputPort() == IMPL_INPUT_PORT_LOCAL_PACKET)
  {
// {{{  mob->role->mnSrcAddrDetetermination();

    ipv6_addr::SCOPE destScope = ipv6_addr_scope(datagram->destAddress());

    //We don't care which outgoing iface dest is on because it is determined by
    //default router ifIndex anyway (preferably iface on link with Internet
    //connection after translation to care of addr)
    if (rt->mobilitySupport() && rt->isMobileNode() &&
	datagram->srcAddress() == IPv6_ADDR_UNSPECIFIED &&
	rt->mipv6cds->mipv6cdsMN->primaryHA().get() != 0 &&
	destScope == ipv6_addr::Scope_Global)
    {
      Dout(dc::mipv6, rt->nodeName()<<" using homeAddress "
	   <<rt->mipv6cds->mipv6cdsMN
	   ->homeAddr()<<" for destination "<<datagram->destAddress());
      datagram->setSrcAddress(rt->mipv6cds->mipv6cdsMN->homeAddr());
    }

// }}}

    if (rt->isMobileNode())
    {
      if (!(boost::polymorphic_downcast<MobileIPv6::MIPv6MStateMobileNode*>(
	      mob->role))->mnSendPacketCheck(*datagram, this))
	return;
    }

    (boost::polymorphic_downcast<MobileIPv6::MIPv6MStateCorrespondentNode*>(
      mob->role))->cnSendPacketCheck(*datagram);
  }

// }}}

  insertSourceRoute(*datagram);

// {{{ tunnel via dest addr using CDS::findDestEntry

#if !defined NOINGRESSFILTERING
  //Cannot use the result of conceptual sending i.e. ifIndex because at that
  //stage dgrams with link local addresses are dropped.  Thus this disables a
  //form of "IP Masq" via tunneling at the gateways.  Otherwise this check is
  //not required here at all and a test for vIfIndex from result of conceptual
  //sending is sufficient to trigger a tunnel. (Actually is it required for
  //prefixed tunnel matches see TunnelNet example with prefixed dest trigger?)
  IPv6NeighbourDiscovery::IPv6CDS::DCI it;
  //Want to do this for received packets only otherwise the trigger on cn
  //address will prevent hoa option from been attached to packets sent to CN
  if (datagram->inputPort() != IMPL_INPUT_PORT_LOCAL_PACKET &&
      rt->cds->findDestEntry(datagram->destAddress(), it))
  {
    NeighbourEntry* ne = it->second.neighbour.lock().get();
/*
    Dout(dc::debug|dc::encapsulation|dc::forwarding|flush_cf, rt->nodeName()
         <<" possible tunnel index="<<(ne?
                                       OPP_Global::ltostr(ne->ifIndex()):
                                       "No neighbour")
         <<(ne?
            std::string(" state=") + OPP_Global::ltostr(ne->state()):
            "")
         <<" dgram="<<*datagram);
*/
    //-1 is used to indicate promiscuous addr res so INCOMPLETE test
    //required
    if (ne && ne->state() != NeighbourEntry::INCOMPLETE &&
        ne->ifIndex()  > ift->numInterfaceGates() )
    {
      Dout(dc::encapsulation|dc::forwarding|dc::debug|flush_cf, rt->nodeName()
           <<" Found tunnel vifIndex ="<<hex<<it->second.neighbour.lock().get()->ifIndex() <<dec);
      IPv6Datagram* copy = datagram->dup();
      copy->setOutputPort(ne->ifIndex());
      send(copy, "tunnelEntry");

      if (!datagram->destAddress().isMulticast())
        return;
    }
  }
#endif //!NOINGRESSFILTERING

// }}}

// {{{ Packet was received and not for us

  if (datagram->inputPort() != IMPL_INPUT_PORT_LOCAL_PACKET )
  {
    if (!processReceived(*datagram))
      return;

    Dout(dc::debug|dc::debug|flush_cf, rt->nodeName()<<":"<<datagram->inputPort()
         <<" forwarding packet "<<*datagram);
  }//End if packet was received

// }}}

  if (datagram->destAddress().isMulticast())
  {
    send(datagram.release(), "multicastOut");
    return;
  }

  Dout(dc::forwarding|dc::debug|flush_cf, rt->nodeName()<<" "<<dec<<setprecision(4)<<simTime()
       <<" sending/forwarding dgram="<<*datagram);

  AddrResInfo *info = new AddrResInfo;
  int status = conceptualSending(datagram.get(), info); // fills in control info
  datagram->setControlInfo(info);

// {{{ Address res required

  // destination address does not exist in routing table:
  // Can this ever happen unless there is no default router?

  // If no router, Assume dest is onlink according to Sec 5.2 Conceptual
  // sending algorithm. Do address resolution on all ifaces.

  // or
  // packet awaits addr res so put into queue
  if (status == -1 || status == -2)
  {
    Dout(dc::forwarding|flush_cf," "<<rt->nodeName()<<":"<<info->ifIndex()<<" "
         <<simTime()<<" "<<className()<<": addrRes pending packet dest="
         <<datagram->destAddress()<<" nexthop="<<info->nextHop()<<" status="<<status);

    if (rt->odad())
    {
      bool foundTentative = false;
      for (unsigned int i = 0; i < ift->numInterfaceGates(); i++)
      {
        InterfaceEntry *ie = ift->interfaceByPortNo(i);
        if ((datagram->srcAddress() != IPv6_ADDR_UNSPECIFIED &&
             ie->ipv6()->tentativeAddrAssigned(datagram->srcAddress()))
#ifdef USE_MOBILITY
            || (rt->mobilitySupport() && rt->isMobileNode() && 
                rt->mipv6cds->mipv6cdsMN->awayFromHome() &&
                ie->ipv6()->tentativeAddrAssigned(
                  rt->mipv6cds->mipv6cdsMN->careOfAddr(true)))
#endif //USE_MOBILITY
            )
        {
          //TODO forward via router
          Dout (dc::custom|dc::forwarding|flush_cf, rt->nodeName()<<":"<<simTime()
                <<" ODAD would forward to router for unknown neighbour LL addr. (once I get more time)");
          foundTentative = true;
          return;
        }
      }
    }

    send(datagram.release(), "pendingQueueOut");
    return;
  }

// }}}

  //Set src address here if upper layer protocols left it up
  //to network layer
  if (datagram->srcAddress() == IPv6_ADDR_UNSPECIFIED)
  {
    datagram->setSrcAddress(determineSrcAddress(
                              datagram->destAddress(), info->ifIndex()));
  }

// {{{ tunnel via dest address using conceptualSending (for prefixed dest addresses)

  //Required to enable triggering of prefix addresses instead of just a host
  //match as done via rt->cds->findDestEntry earlier in this function
  if ((unsigned int)status > ift->numInterfaceGates())
  {
    Dout(dc::encapsulation|dc::forwarding|flush_cf, rt->nodeName()<<":"<<datagram->inputPort()
         <<" vIfindex="<<hex<<status<<dec<<" dgram="<<*datagram);
    datagram->setOutputPort(info->ifIndex());
    delete datagram->removeControlInfo();
    send(datagram.release(), "tunnelEntry");
    return;
  }

// }}}

// {{{ Drop packets with src addresses that are not ready yet
  if (rt->mobilitySupport())
  {
    MobileIPv6::MIPv6CDSMobileNode* mipv6cdsMN = rt->mipv6cds->mipv6cdsMN;
    if (mipv6cdsMN && mipv6cdsMN->currentRouter().get() != 0 &&
	datagram->srcAddress() != mipv6cdsMN->homeAddr()
	)
    {
      if (
	(ift->interfaceByPortNo(info->ifIndex())->ipv6()->addrAssigned(
	  datagram->srcAddress())
	 ||
	 (rt->odad() &&
	  ift->interfaceByPortNo(info->ifIndex())->ipv6()->tentativeAddrAssigned(
	    datagram->srcAddress())
	  ))
	)
      {
	Dout(dc::forwarding|flush_cf, rt->nodeName()<<" "<<simTime()
	     <<" checking coa "<<datagram->srcAddress()
	     <<" is onlink at correct ifIndex "<<info->ifIndex());
	unsigned int ifIndexTest;
	//outgoing interface (ifIndex) MUST have src addr (care of Addr) as on
	//link prefix
	//assert(rt->cds->lookupAddress(datagram->srcAddress(), ifIndexTest));
	//assert(ifIndexTest == info->ifIndex());
	if (!rt->cds->lookupAddress(datagram->srcAddress(), ifIndexTest))
	{
	  Dout(dc::forwarding|dc::mipv6, rt->nodeName()<<" "<<simTime()
	       <<"No suitable src address available on foreign network as "
	       <<"coa is old one "<<datagram->srcAddress()
	       <<"and BA from HA not received packet dropped");
	  datagram->setSrcAddress(IPv6_ADDR_UNSPECIFIED);
	}
      }
      else
      {
	if (mipv6cdsMN->currentRouter().get() == 0)
	{
	  //FMIP just set to PCoA here and hope for the best right.
	  Dout(dc::forwarding|dc::mipv6, rt->nodeName()<<" "<<simTime()
	       <<" No suitable src address available on foreign network as no "
	       <<"routers recorded so far to form coa so packet dropped");
	}
	else
	{
	  InterfaceEntry *ie = ift->interfaceByPortNo(info->ifIndex());
	  ipv6_addr unready = mipv6cdsMN->careOfAddr(false);
	  if (ie->ipv6()->tentativeAddrs.size())
	    unready = ie->ipv6()->tentativeAddrs[ie->ipv6()->tentativeAddrs.size()-1];
	  Dout(dc::forwarding|dc::mipv6, rt->nodeName()<<" "<<simTime()
	       <<"No suitable src address available on foreign network as "
	       <<"ncoa in not ready from DAD or BA from HA/MAP not received "
	       <<unready<<" packet dropped");
	}
	datagram->setSrcAddress(IPv6_ADDR_UNSPECIFIED);
      }
    }
  }
// }}}

// {{{ unspecified src addr so drop

  if (datagram->srcAddress() == IPv6_ADDR_UNSPECIFIED)
  {
#ifndef USE_MOBILITY
    Dout(dc::warning, rt->nodeName()<<" "<<className()<<" No suitable src Address for destination "
         <<datagram->destAddress());
#endif //USE_MOBILITY
    ctrIP6OutNoRoutes++;
    //TODO probably send error message about -EADDRNOTAVAIL

    return;
  }

// }}}

  //Unicast dest addr with dest LL addr available in DC -> NE
  assert(datagram->srcAddress() != IPv6_ADDR_UNSPECIFIED &&
         !datagram->srcAddress().isMulticast());

  // default: send datagram to fragmentation
  datagram->setOutputPort(info->ifIndex());

// {{{ Router: send redirect if on link neighbour

    ///Check for forwarding to on link neighbour and send back a redirect to host
    if (rt->isRouter() && datagram->inputPort() != IMPL_INPUT_PORT_LOCAL_PACKET &&
	info->ifIndex() == (unsigned int)datagram->inputPort() &&
        datagram->destAddress() == info->nextHop())
    {
      bool redirected = false;
      //Enter_Method does not work here :(
      nd->sendRedirect(datagram.release(), info->nextHop(), redirected);
      Dout(dc::custom|dc::forwarding|flush_cf, rt->nodeName()<<":"<<datagram->inputPort()
           <<" "<<simTime()<<" sent redirect to ODAD node "<<datagram->srcAddress());
      if (redirected)
      {
        return;
      }
    }

// }}}

  send(datagram.release(), "fragmentationOut");
}

void IPv6Forward::finish()
{
  recordScalar("IP6InAddrErrors", ctrIP6InAddrErrors);
  recordScalar("IP6OutNoRoutes", ctrIP6OutNoRoutes);
}


std::ostream& operator<<(std::ostream & os,  IPv6Forward& routeMod)
{
  for (IPv6Forward::SRI it = routeMod.routes.begin();
       it != routeMod.routes.end(); it++)
  {
    os << "Source route to dest "<<it->first<<" via intermediate hop"<<endl;
    copy(it->second->begin(), it->second->end(), ostream_iterator<
         _SrcRoute::value_type>(os, " >>\n"));
//    os << *(it->second.end()-1)<<endl;
  }
  return os;
}

///Route is searched according to final dest (last address in SrcRoute).  The
///src address of packet is used to determine outgoing iface.
void IPv6Forward::addSrcRoute(const SrcRoute& route)
{
  routes[*route->rbegin()] = route;
}

/**
   Based on Conceptual Sending Algorithm described in Sec. 5.2 of RFC2461

   info.outputPort = UINT_MAX;
   Notify address resolution to occur on all interfaces i.e. don't know
   which link this address could be on
*/

int IPv6Forward::conceptualSending(IPv6Datagram *dgram, AddrResInfo *info)
{
  // Conceptual Sending Algorithm

  weak_ptr<NeighbourEntry> ne;

  if (rt->isRouter())
    ne = rt->cds->lookupDestination(dgram->destAddress());
  else
    ne = rt->cds->neighbour(dgram->destAddress());

  if (ne.lock().get() == 0)
  {
    // Next Hop determination
    unsigned int tmpIfIndex = info->ifIndex();
    if (rt->cds->lookupAddress(dgram->destAddress(),tmpIfIndex))
    {
      info->setIfIndex(tmpIfIndex);
      // destination address of the packet is onlink

      //Do Address Resolution from this interface (info->ifIndex())
      info->setNextHop(dgram->destAddress());
      return -2;
    }
    // destination address of the packet is offlink
    else
    {
      info->setIfIndex(tmpIfIndex);
      ne = rt->cds->defaultRouter();

      // check to see if the router entry exists
      if(ne.lock().get() != 0)
      {
        //Save this route to dest in DC
        (*rt->cds)[dgram->destAddress()].neighbour = ne;
        Dout(dc::forwarding, rt->nodeName()<<" Using default router addr="<<ne.lock()->addr()<<" for dest="
             <<dgram->destAddress()<<" ne="<<*(ne.lock().get()));
        if (ne.lock()->addr() == dgram->srcAddress())
        {
          cerr<< rt->nodeName()<<" default router of destination points back to source! "
              <<*(static_cast<IRouterList*>(rt->cds))<<endl;
          Dout(dc::warning, rt->nodeName()<<" default router of destination points back to source! "
               <<*(static_cast<IRouterList*>(rt->cds)));
        }
      }
      else
      {

        if (ift->numInterfaceGates() > 1)
          //Signify to addr res to occur on all interfaces
          info->setIfIndex(UINT_MAX);
        else
          info->setIfIndex(0);

        info->setNextHop(dgram->destAddress());

        Dout(dc::forwarding, "No default router assuming dest="<<dgram->destAddress()
             <<" is on link."<<"Performing "
             <<(info->ifIndex() != 0?
                "promiscuous addr res":
                "plain addr res on single iface=")<<info->ifIndex());

        // no route to dest -1 (promiscuous addr res) or do plain addr res -2
        return info->ifIndex() != 0?-1:-2;
      }

    }
  }
  else if (!rt->isRouter())
    Dout(dc::forwarding, " Found dest "<<dgram->destAddress()<<" in Dest Cache next hop="
         <<ne.lock()->addr());

  //Assume neighbour is reachable and use precached info
  info->setNextHop(ne.lock().get()->addr());
  info->setIfIndex(ne.lock().get()->ifIndex());

  //Neighbour exists check state that neighbour is in
  if (ne.lock().get()->state() == NeighbourEntry::INCOMPLETE)
    //Pass dgram to addr resln to queue pending packet
    return -2;

  //TODO
  if (ne.lock().get()->state() == NeighbourEntry::STALE)
  {
    //Initiate NUD timer to go to DELAY state & subsequently PROBE if
    //no indication of reachability (refer to RFC 2461 Sec. 7.3.2

    //Probably best to return an indication of this so RoutingCore starts the
    //timer or send a message to ND to initiate NUD

    Dout(dc::debug, rt->nodeName()<<":"<<info->ifIndex()<<" -- Reachability to "
         << info->nextHop() <<" STALE");
  }

  info->setLinkLayerAddr(ne.lock().get()->linkLayerAddr().c_str());

  return info->ifIndex();
} //end conceptualSending

/**
 *    Choose an apropriate source address
 *    should do:
 *    i)   get an address with an apropriate scope
 *    ii)  see if there is a specific route for the destination and use
 *         an address of the attached interface
 *    iii) don't use deprecated addresses or Expired Addresses TODO
 */

ipv6_addr IPv6Forward::determineSrcAddress(const ipv6_addr& dest, size_t ifIndex)
{
  ipv6_addr::SCOPE destScope = ipv6_addr_scope(dest);

  assert(dest != IPv6_ADDR_UNSPECIFIED);
  if (dest == IPv6_ADDR_UNSPECIFIED)
    return IPv6_ADDR_UNSPECIFIED;

  //ifIndex == UINT_MAX can only mean No default Router so assume dest is onlink
  //and return any link local address on the default Interface.  Address Res
  //will find the correct iface and the subsequent source Address
  if (ifIndex == UINT_MAX && rt->cds->defaultRouter().lock().get() == 0)
  {
    if (ift->interfaceByPortNo(0)->ipv6()->inetAddrs.size() == 0)
    {
      cerr <<rt->nodeId()<<" "<<ift->interfaceByPortNo(ifIndex)->name()
           <<" is not ready (no addresses assigned"<<endl;
      Dout(dc::mipv6, rt->nodeName()<<" "<<ift->interfaceByPortNo(ifIndex)->name()
           <<" is not ready (no addresses assigned");

      if (rt->odad())
      {
        if (ift->interfaceByPortNo(0)->ipv6()->tentativeAddrs.size())
        {
          Dout(dc::custom, rt->nodeName()<<":"<<ifIndex<<" "<<simTime()
               <<" determineSrcAddress no default rtr case ODAD is on using tentative addr="
               << ift->interfaceByPortNo(0)->ipv6()->tentativeAddrs[0]);
          return ift->interfaceByPortNo(0)->ipv6()->tentativeAddrs[0];
        }
      }

      return IPv6_ADDR_UNSPECIFIED;
    }
    else
      return ift->interfaceByPortNo(0)->ipv6()->inetAddrs[0];
  }

  InterfaceEntry *ie = ift->interfaceByPortNo(ifIndex);

  for (size_t i = 0; i < ie->ipv6()->inetAddrs.size(); i++)
    if (ie->ipv6()->inetAddrs[i].scope() == destScope)
      return ie->ipv6()->inetAddrs[i];

  if (rt->odad())
  {
    for (size_t i = 0; i < ie->ipv6()->tentativeAddrs.size(); i++)
      if (ie->ipv6()->tentativeAddrs[i].scope() == destScope)
      {
        Dout(dc::custom, rt->nodeName()<<":"<<ifIndex<<" "<<simTime()
             <<" determineSrcAddress using ODAD addr="
             << ift->interfaceByPortNo(0)->ipv6()->tentativeAddrs[0]);
        return ie->ipv6()->tentativeAddrs[i];
      }
  }

#if !defined NOIPMASQ
  //Perhaps allow return of src addresses with diff scope to allow for case of
  //"IP MASQ".
  return ie->ipv6()->inetAddrs[ie->ipv6()->inetAddrs.size() - 1];
#else
  return IPv6_ADDR_UNSPECIFIED;
#endif //!NOIPMASQ
} //end determineSrcAddress

/*
  @brief Test for a preconfigured source route to a destination and insert an
  appropriate routing header
 */
bool IPv6Forward::insertSourceRoute(IPv6Datagram& datagram)
{
  SRI srit = routes.find(datagram.destAddress());
  if (srit != routes.end() &&
      rt->localDeliver(datagram.srcAddress()) &&
      !datagram.findHeader(EXTHDR_ROUTING))
  {
    HdrExtRteProc* proc = datagram.acquireRoutingInterface();
    HdrExtRte* rt0 = proc->routingHeader();
    if(!rt0)
    {
      rt0 = new HdrExtRte;
      proc->addRoutingHeader(rt0);
    }

    const SrcRoute& route = srit->second;
    for_each(route->begin()+1, route->end(),
	     boost::bind(&HdrExtRte::addAddress, rt0, _1));
    assert(datagram.destAddress() == *(route->rbegin()));
    datagram.setDestAddress(*(route->begin()));
    return true;
  }
  return false;
}

bool IPv6Forward::processReceived(IPv6Datagram& datagram)
{
  // delete datagram and continue when datagram arrives from Network
  // for another node
  // IP_FORWARD is off (host)
  // All ND messages are 255 i.e. are not forwarded.
  // Link local address on either source or destination address
  // srcAddress is multicast
  if (!rt->isRouter() || datagram.hopLimit() >= 255 ||
      ipv6_addr_scope(datagram.srcAddress()) == ipv6_addr::Scope_Link ||
      ipv6_addr_scope(datagram.destAddress()) == ipv6_addr::Scope_Link||
      ipv6_addr_scope(datagram.srcAddress()) == ipv6_addr::Scope_Node ||
      ipv6_addr_scope(datagram.destAddress()) == ipv6_addr::Scope_Node||
      datagram.srcAddress().isMulticast() ||
      (!rt->routeSitePackets() &&
       (ipv6_addr_scope(datagram.destAddress()) == ipv6_addr::Scope_Site ||
        ipv6_addr_scope(datagram.srcAddress())  == ipv6_addr::Scope_Site)))
  {
    ctrIP6InAddrErrors++;
    if (!rt->isRouter())
    {
      Dout(dc::debug, rt->nodeName()<<":"<<datagram.inputPort()<<" Packet discarded as this is a host");
    }
    else if (datagram.hopLimit() >= 255)
    {
      // TODO: The datagram could be a proxy NS sent by mobile node
      // and the router could be CN's HA. Should we send proxy NA?
      Dout(dc::forwarding|dc::debug|flush_cf, rt->nodeName()<<":"<<datagram.inputPort()<<" Packet discarded as it is a ND packet");
    }
    else if (ipv6_addr_scope(datagram.srcAddress()) == ipv6_addr::Scope_Link ||
             ipv6_addr_scope(datagram.destAddress()) == ipv6_addr::Scope_Link)
    {
      Dout(dc::forwarding|dc::debug|flush_cf, rt->nodeName()<<":"<<datagram.inputPort()
           <<" Packet discarded because the addresses src="<<datagram.srcAddress()<<" dest="
           <<datagram.destAddress()<< " have link local scope "<< dec <<datagram.hopLimit());
    }
    else if (!rt->routeSitePackets() &&
             (ipv6_addr_scope(datagram.destAddress()) == ipv6_addr::Scope_Site ||
              ipv6_addr_scope(datagram.srcAddress())  == ipv6_addr::Scope_Site))
    {
      Dout(dc::forwarding|dc::debug|flush_cf, rt->nodeName()<<":"<<datagram.inputPort()<<" Packet discarded because the addresses have site scope and forward site packets is "<<(rt->routeSitePackets()?"true":"false"));
    }

    return false;
  }

  //decrement hop Limit only when forwarding
  datagram.setHopLimit(datagram.hopLimit() - 1);
  if (datagram.hopLimit() == 0)
  {
    sendErrorMessage(createICMPv6Message("timeExceeded",
                                         ICMPv6_TIME_EXCEEDED,
                                         ND_HOP_LIMIT_EXCEEDED,
                                         datagram.dup()));
    Dout(dc::forwarding|dc::debug|flush_cf, rt->nodeName()<<":"<<datagram.inputPort()
         <<" hop limit exceeded"<<datagram);
    return false;
  }

  return true;
}

/*     ----------------------------------------------------------
        Private Functions
    ----------------------------------------------------------    */

// send error message to ICMP Module
void IPv6Forward::sendErrorMessage(ICMPv6Message* err)
{
    send(err, "errorOut");

}
