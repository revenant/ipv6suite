//
// Copyright (C) 2001, 2003 CTIE, Monash University
// Copyright (C) 2000 Institut fuer Telematik, Universitaet Karlsruhe
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

Define_Module( IPv6Send );

void IPv6Send::initialize()
{
  QueueBase::initialize();
  ift = InterfaceTableAccess().get();
  rt = RoutingTable6Access().get();

  defaultMCTimeToLive = par("multicastTimeToLive");
  ctrIP6OutRequests = 0;
}

void IPv6Send::endService(cMessage* msg)
{
  IPv6Datagram *dgram = encapsulatePacket(msg);

  // send new datagram
  if (dgram)
    send(dgram, "routingOut");
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
Debug(
    //Test if source address actually exists
    bool found = false;
    for (size_t ifIndex = 0; ifIndex < ift->numInterfaceGates(); ifIndex++)
    {
      InterfaceEntry *ie = ift->interfaceByPortNo(ifIndex);
      if (ie.addrAssigned(mkIpv6_addr(ctrl->srcAddr())) ||
					  (rt->odad() && ie->ipv6()->addrAssigned(mkIpv6_addr(ctrl->srcAddr())))
      {
        found = true;
        break;
      }
    }

    if (!found)
      Dout(dc::warning|flush_cf, rt->nodeName()<<" src addr not found in ifaces "<<mkIpv6_addr(ctrl->srcAddr()));
); // end Debug

    datagram->setSrcAddress(mkIpv6_addr(ctrl->srcAddr()));
  }
  else
  {
    //Let IPv6Routing determine src addr
  }

  datagram->setInputPort(-1);

  if (ctrl->timeToLive() > 0)
    datagram->setHopLimit(ctrl->timeToLive());

  //TODO check dest Path MTU here
  ctrIP6OutRequests++;

  delete ctrl;
  return datagram;
}

void IPv6Send::finish()
{
  recordScalar("IP6OutRequests", ctrIP6OutRequests);
}

