// $Header: /home/cvs/IPv6Suite/IPv6SuiteWithINET/Network/IPv6/IPv6Multicast.cc,v 1.1 2005/02/09 06:15:58 andras Exp $
//
// Copyright (C) 2000 Institut fuer Telematik, Universitaet Karlsruhe
// Copyright (C) 2001 CTIE, Monash University
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

/*
    @file IPv6Multicast.cc
    @brief Implementation for IPv6 Multicast

    Responsibilities:
    receive datagram with Multicast address from Routing
    duplicate datagram if it is sent to more than one output port
		and IPForward is true
    map multicast address on output port, use multicast routing table
    Outsend copy to  local deliver, if
    NetworkCardAddr.[] is part of multicast address
    if entry in multicast routing table requires tunneling,
    send to Tunneling module
    otherwise send to Fragmentation module

    receive IGMP message from LocalDeliver
    update multicast routing table

    Based on IPMulticast by Jochen Reber

	@author Johnny Lai


    @todo @see RoutingTable6.h Tunneling and IGMP not implemented

*/

#include <cstdlib>
#include <omnetpp.h>
#include <cstring>
#include <boost/cast.hpp>

#include "IPv6Multicast.h"
#include "IPv6Datagram.h"
#include "RoutingTable6.h"
#include "cTypedMessage.h"
#include "Messages.h"
#include "IPv6ForwardCore.h"
#include "opp_utils.h"

Define_Module( IPv6Multicast );

/*  ----------------------------------------------------------
        Public Functions
    ----------------------------------------------------------  */

void IPv6Multicast::initialize()
{
	RoutingTable6Access::initialize();
	delay = par("procdelay");
    fc = boost::polymorphic_downcast<IPv6ForwardCore*> (
      OPP_Global::findModuleByTypeDepthFirst(this, "IPv6ForwardCore"));
    assert(fc != 0);
    ctrIP6InMcastPkts = 0;
    waitTmr = new cMessage("IPv6MulticastWait");
    msg = 0;
}

void IPv6Multicast::handleMessage(cMessage* theMsg)
{
  if (!theMsg->isSelfMessage())
  {
    if (waitTmr->isScheduled())
    {
      cerr<<fullPath()<<" "<<simTime()<<" received new packet "
          <<(*boost::polymorphic_downcast<IPv6Datagram *>(msg))
          <<" when previous packet was scheduled at waitTmr="<<waitTmr->arrivalTime();
      delete theMsg;
      return;
    } else if (!waitTmr->isScheduled() && 0 == msg)
    {
      msg = theMsg;
      scheduleAt(delay + simTime(), waitTmr);
      return;
    }
      assert(false);
    return;
  }

  // otherwise deliver/forward datagram with Multicast address
  bool ifaceSpecified = false;
  IPv6Datagram *datagram = boost::polymorphic_downcast<IPv6Datagram *>(msg);
  assert(msg);
  assert(datagram);
  IPv6Address destAddress(datagram->destAddress());
  assert(destAddress.isMulticast());

  // check for local delivery already done at IPv6ForwardCore

  // Check already completed in IPv6ForwardCore
  /* forward datagram only if IPv6Forward is true
     or sent locally */

  //According to Linux code it appears that multicast addresses also have
  //destination entries.  At least to identify which dev(iface) to send
  //through I guess. The best I can do is send out of one iface when
  //packet originates from this node with a source address If no src
  //address or packet is to be forwarded then dumbly output on every
  //iface.  Need real IGMP to be implemented in ICMP to do real
  //multicasting instead of broadcast.
  if (datagram->srcAddress() != IPv6_ADDR_UNSPECIFIED &&
      datagram->inputPort() != -1)
  {
    ifaceSpecified = true;
    ctrIP6InMcastPkts++;
  }

  for (size_t ifIndex = 0; ifIndex < rt->interfaceCount(); ifIndex++)
  {
    if (ifIndex == static_cast<size_t> (datagram->inputPort()))
      //Don't send on incoming interface again otherwise we'll get a
      //feedback loop
      continue;
    if (ifaceSpecified)
    {
      Interface6Entry& ie = rt->getInterfaceByIndex(ifIndex);
      if (ie.addrAssigned(datagram->srcAddress()))
      {
        dupAndSendPacket(datagram, ifIndex);
        break;
      }
      else
        continue;
    }
    dupAndSendPacket(datagram, ifIndex);
  }

  ifaceSpecified = false;
  // only copies sent, original datagram deleted
  delete datagram;
  msg = 0;

}

void IPv6Multicast::finish()
{
  recordScalar("IP6InMcastPkts", ctrIP6InMcastPkts);

}

inline void IPv6Multicast::dupAndSendPacket(const IPv6Datagram* datagram, size_t ifIndex)
{
  IPv6Datagram* datagramCopy = datagram->dup();
  if (datagramCopy->srcAddress() == IPv6_ADDR_UNSPECIFIED)
  {
    datagramCopy->setSrcAddress(fc->determineSrcAddress(
                              datagram->destAddress(), ifIndex));
     //Do determine SrcAddress if none exist
    if (datagramCopy->srcAddress() == IPv6_ADDR_UNSPECIFIED)
    {
      cerr<<rt->nodeId()<<":"<<ifIndex<<" Unable to find a suitable src address for packet destined to "<<datagramCopy->destAddress()<<endl;
      return;
    }
  }

  AddrResInfo info = {datagramCopy, datagramCopy->destAddress(),
                      ifIndex,
                      multicastLLAddr(datagramCopy->destAddress())};
  AddrResMsg* addrResMsg = new AddrResMsg(info);
  
  send(addrResMsg, "fragmentationOut");
}

/**
   For now this function returns an Ethernet MAC broadcast address for all
   multicast addresses.
   Future improvement would be to create correct Multicast Link layer addr from
   addr.
*/
string IPv6Multicast::multicastLLAddr(const ipv6_addr& addr)
{
  //For now just do it like so assuming MAC addr
  if ((addr & IPv6_ADDR_MULTICAST_PREFIX) == IPv6_ADDR_MULTICAST_PREFIX)
    return "FF:FF:FF:FF:FF:FF";

  assert(false);
  return "";
}
