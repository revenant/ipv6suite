//
// Copyright (C) 2002, 2004, 2005 CTIE, Monash University
// Copyright (C) 2006 by Johnny Lai
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
#include <boost/cast.hpp>

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
#include "IPv6InterfaceData.h"
#include "IPv6CDS.h"


#ifdef USE_MOBILITY
#include "MIPv6CDSMobileNode.h"
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

    ctrIP6InAddrErrors = 0;

    routingInfoDisplay = par("routingInfoDisplay");

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


// {{{ tunnel via dest addr using CDS::findDestEntry

#if !defined NOINGRESSFILTERING
  //Cannot use the result of conceptual sending i.e. ifIndex because at that
  //stage dgrams with link local addresses are dropped.  They are dropped at
  //processReceived.  Thus this disables a form of "IP Masq" via tunneling at
  //the gateways.  Otherwise this check is not required here at all and a test
  //for vIfIndex from result of conceptual sending is sufficient to trigger a
  //tunnel. This code required for TunnelNet -r 1 but not the prefixed matches
  //-r 2 because only run 1 uses link local addr

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

  AddrResInfo *info = boost::polymorphic_downcast<AddrResInfo*>(datagram->removeControlInfo());
  int status = 0;
  if (!info)
  {
    info = new AddrResInfo;
    status = rt->conceptualSending(datagram.get(), info);
  }
  else
    status = info->status();
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
                  rt->mipv6cds->mipv6cdsMN->careOfAddr()))
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

// {{{ tunnel via dest address using conceptualSending (for prefixed dest addresses)

  //Required to enable triggering of prefix addresses instead of just a host
  //match as done via rt->cds->findDestEntry earlier in this function
  if (info->ifIndex() > ift->numInterfaceGates())
  {
    Dout(dc::encapsulation|dc::forwarding|flush_cf, rt->nodeName()<<":"<<datagram->inputPort()
         <<" vIfindex="<<hex<<status<<dec<<" dgram="<<*datagram);
    datagram->setOutputPort(info->ifIndex());
    delete datagram->removeControlInfo();
    send(datagram.release(), "tunnelEntry");
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
