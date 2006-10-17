//
// Copyright (C) 2000 Institut fuer Telematik, Universitaet Karlsruhe
// Copyright (C) 2001, 2003 CTIE, Monash University
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
    @file IPv6Send.cc
    @brief Implementation for IPSendCore
    ------
    Responsibilities:
    receive IPInterfacePacket from Transport layer or ICMP
    or Tunneling (IP tunneled datagram)
    encapsulate in IP datagram
    set version
    set hoplimit
    set Protocol to received value
    set destination address to received value
    send datagram to Routing

    if IPInterfacePacket is invalid (e.g. invalid source address),
     it is thrown away without feedback

    @author Johnny Lai
    based on IPSendCore by Jochen Reber
*/
#include "sys.h"
#include "debug.h"

#include <omnetpp.h>
#include <cstdlib>
#include <cstring>
#include <boost/cast.hpp>


#include "IPv6Send.h"
#include "IPv6Datagram.h"
#include "IPv6ControlInfo_m.h"
#include "ipv6addrconv.h"
#include "HdrExtRteProc.h"
#include "InterfaceTableAccess.h"
#include "IPv6InterfaceData.h"
#include "RoutingTable6Access.h"
#include "IPv6Mobility.h"
#include "MIPv6MStateMobileNode.h"
#include "AddrResInfo_m.h"
#include "IPv6CDS.h"
#include "MIPv6CDSMobileNode.h"

Define_Module( IPv6Send );

void IPv6Send::initialize()
{
  QueueBase::initialize();
  ift = InterfaceTableAccess().get();
  rt = RoutingTable6Access().get();

  ctrIP6OutNoRoutes = 0;

  defaultMCTimeToLive = par("multicastTimeToLive");
  ctrIP6OutRequests = 0;
  mob = check_and_cast<IPv6Mobility*>
    (OPP_Global::findModuleByType(rt, "IPv6Mobility"));
}

void IPv6Send::endService(cMessage* msg)
{
  IPv6Datagram *datagram = encapsulatePacket(msg);
  if (!datagram)
    return;
  auto_ptr<IPv6Datagram> cleanup(datagram);

// {{{ Process outgoing packet for MIP6 
//binding exists for dest then swap dest==hoa to coa

  if (rt->mobilitySupport())
  {
    if (rt->isMobileNode())
    {
      MobileIPv6::MIPv6MStateMobileNode* mstateMN = 
	boost::polymorphic_downcast<MobileIPv6::MIPv6MStateMobileNode*>(mob->role);
      mstateMN->mnSrcAddrDetermination(datagram);

      bool tunnel = false;
      if (!mstateMN->mnSendPacketCheck(*datagram, tunnel))
      {
	ctrIP6OutNoRoutes++;
	return;
      }
      if (tunnel)
      {
	cleanup.release();
	send(datagram, "tunnelEntry");
	return;
      }
    }

    (boost::polymorphic_downcast<MobileIPv6::MIPv6MStateCorrespondentNode*>(
      mob->role))->cnSendPacketCheck(*datagram);
  }

// }}}

  AddrResInfo* info = new AddrResInfo;
  rt->conceptualSending(datagram, info);
  datagram->setControlInfo(info);

  //Set src address here if upper layer protocols left it up
  //to network layer
  if (datagram->srcAddress() == IPv6_ADDR_UNSPECIFIED)
  {
    datagram->setSrcAddress(rt->determineSrcAddress(
                              datagram->destAddress(), info->ifIndex()));
  }

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
    if (!rt->mobilitySupport())
      Dout(dc::warning, rt->nodeName()<<" "<<className()<<" No suitable src Address for destination "
	   <<datagram->destAddress());
    ctrIP6OutNoRoutes++;
    //TODO probably send error message about -EADDRNOTAVAIL

    return;
  }

// }}}

  cleanup.release();
  send(datagram, "routingOut");

}

IPv6Datagram *IPv6Send::encapsulatePacket(cMessage *msg)
{
  // if no interface exists, do not send datagram
  if (ift->numInterfaceGates() == 0 ||
      ift->interfaceByPortNo(0)->ipv6()->inetAddrs.size() == 0)
  {
    cerr<<rt->nodeId()<<" 1st Interface is not ready yet"<<endl;
    Dout(dc::warning, rt->nodeName()<<" 1st Interface is not ready yet");
    delete msg;
    return NULL;
  }

  IPv6Datagram *datagram = new IPv6Datagram();
  IPv6ControlInfo *ctrl = check_and_cast<IPv6ControlInfo*>(msg->removeControlInfo());

  datagram->encapsulate(msg);
  datagram->setName(msg->name());

  datagram->setTransportProtocol(ctrl->protocol());

  // set source and destination address
  assert(mkIpv6_addr(ctrl->destAddr()) != IPv6_ADDR_UNSPECIFIED);
  datagram->setDestAddress(mkIpv6_addr(ctrl->destAddr()));

  // when source address given in Interface Message, use it
  if (mkIpv6_addr(ctrl->srcAddr()) != IPv6_ADDR_UNSPECIFIED)
  {
    //Test if source address actually exists
    bool found = false;
    for (size_t ifIndex = 0; ifIndex < ift->numInterfaceGates(); ifIndex++)
    {
      InterfaceEntry *ie = ift->interfaceByPortNo(ifIndex);
      if (ie->ipv6()->addrAssigned(mkIpv6_addr(ctrl->srcAddr())) ||
          (rt->odad() && ie->ipv6()->tentativeAddrAssigned(mkIpv6_addr(ctrl->srcAddr()))))
      {
        found = true;
        break;
      }
    }

    if (!found)
    {
      Dout(dc::warning|flush_cf, rt->nodeName()<<" src addr not assigned in ifaces "<<mkIpv6_addr(ctrl->srcAddr()));
      //opp_error(rt->nodeName()<<" src addr not found in ifaces "<<mkIpv6_addr(ctrl->srcAddr()));
      assert(false);
      delete ctrl;
      delete datagram;
      return 0;
    }

    datagram->setSrcAddress(mkIpv6_addr(ctrl->srcAddr()));
  }
  else
  {
    //Let IPv6Routing determine src addr
  }

  datagram->setInputPort(-1);

  if (ctrl->timeToLive() > 0)
    datagram->setHopLimit(ctrl->timeToLive());

  datagram->setEncapLimit(ctrl->encapLimit());

  //TODO check dest Path MTU here
  ctrIP6OutRequests++;

  delete ctrl;
  return datagram;
}

void IPv6Send::finish()
{
  recordScalar("IP6OutRequests", ctrIP6OutRequests);
  recordScalar("IP6OutNoRoutes", ctrIP6OutNoRoutes);
}

