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
    @file IPv6SendCore.cc
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
#include <memory> //auto_ptr
#include <boost/cast.hpp>


#include "IPv6SendCore.h"
#include "IPv6Datagram.h"
#include "IPv6InterfacePacket.h"
#include "HdrExtRteProc.h"
#include "RoutingTable6.h"

Define_Module( IPv6SendCore );

/**
 @name Public Functions
*/
///@{
void IPv6SendCore::initialize()
{
  rt = RoutingTable6Access().get();

  delay = par("procdelay");
  defaultMCTimeToLive = par("multicastTimeToLive");
  hasHook = (findGate("netfilterOut") != -1);
  ctrIP6OutRequests = 0;
  waitTmr = new cMessage("IPv6SendWait");
  curPacket = 0;
  ifaceReady = false;
}

void IPv6SendCore::handleMessage(cMessage* msg)
{
  if (!msg->isSelfMessage())
  {
    if (waitTmr->isScheduled())
    {
//      cerr<<fullPath()<<" "<<simTime()<<" sending new packet ";
//      (check_and_cast<IPv6InterfacePacket*>(msg))->writeContents(cerr);
//      cerr<<" when previous packet was scheduled at waitTmr="<<waitTmr->arrivalTime();
      Dout(dc::custom, fullPath()<<" "<<simTime()<<" sending new packet "
           <<"when previous packet was scheduled at waitTmr="<<waitTmr->arrivalTime());
      waitQueue.insert(msg);
      return;
    } else if (!waitTmr->isScheduled() && 0 == curPacket)
    {
      IPv6InterfacePacket *interfaceMsg = check_and_cast<IPv6InterfacePacket*>(msg);
      assert(interfaceMsg != 0);

      assert(interfaceMsg->destAddress() != IPv6_ADDR_UNSPECIFIED);

      sendDatagram(interfaceMsg);

      scheduleAt(delay + simTime(), waitTmr);
      return;
    }
    assert(false);
    delete msg;
    return;
  }


  IPv6Datagram* dgram = curPacket;
  if (ifaceReady)
    assert(dgram);
  else
    return;

  // send new datagram
  send(dgram, "routingOut");

  if (waitQueue.empty())
    curPacket = 0;
  else
  {
    IPv6InterfacePacket *interfaceMsg =
      check_and_cast<IPv6InterfacePacket*>(waitQueue.pop());
    sendDatagram(interfaceMsg);
    scheduleAt(delay + simTime(), waitTmr);
  }

}

///@}
/*  ----------------------------------------------------------
        Private Functions
    ----------------------------------------------------------  */

void IPv6SendCore::sendDatagram(IPv6InterfacePacket *theInterfaceMsg)
{
    std::auto_ptr<IPv6InterfacePacket> interfaceMsg(theInterfaceMsg);
    std::auto_ptr<IPv6Datagram> datagram(new IPv6Datagram()); // XXX FIXME why auto_ptr if we release() at the end? --AV

    datagram->encapsulate(interfaceMsg->decapsulate());
    datagram->setTransportProtocol((IPProtocolId)interfaceMsg->protocol());  // XXX khmm..

    datagram->setName(interfaceMsg->name());

    assert(!interfaceMsg->isSelfMessage());

    // set source and destination address
    datagram->setDestAddress(interfaceMsg->destAddr());

    // XXX This needs to be done in llpkt itself as take is a protected function,
    //except that's a template class.  We do not know the type of the template
    //parameter can contain members that are cobjects. Guess cTypedMessage needs
    //refactoring too

    //datagram->take(datagram->encapsulatedMsg());

    // if no interface exists, do not send datagram
    if (rt->interfaceCount() == 0 ||
        rt->getInterfaceByIndex(0).inetAddrs.size() == 0)
    {
      cerr<<rt->nodeId()<<" 1st Interface is not ready yet"<<endl;
      Dout(dc::warning, rt->nodeName()<<" 1st Interface is not ready yet");
      ifaceReady = false;
      return;
    }
    ifaceReady = true;

    // when source address given in Interface Message, use it
    if (interfaceMsg->srcAddress() != IPv6_ADDR_UNSPECIFIED)
    {
Debug(
      //Test if source address actually exists
      bool found = false;
      for (size_t ifIndex = 0; ifIndex < rt->interfaceCount(); ifIndex++)
      {
        Interface6Entry& ie = rt->getInterfaceByIndex(ifIndex);
        if (ie.addrAssigned(c_ipv6_addr(interfaceMsg->srcAddr())))
        {
          found = true;
          break;
        }
//           else
//             cerr<<rt->interfaceCount()<<" "<<ie.inetAddrs.size()<<" "
//                 <<interfaceMsg->srcAddr()<<" "<<(ipv6_addr)ie.inetAddrs[i]<<endl;
      }

      if (!found)
        Dout(dc::warning|flush_cf, rt->nodeName()<<" src addr not found in ifaces "<<interfaceMsg->srcAddress());
//      assert(found);
      );

      datagram->setSrcAddress(interfaceMsg->srcAddr());
    }
    else
    {
      //Let RoutingCore determine src addr
    }

    datagram->setTransportProtocol ( static_cast<IPProtocolId> (interfaceMsg->protocol()));
    datagram->setInputPort(-1);

    if (interfaceMsg->timeToLive() > 0)
      datagram->setHopLimit(interfaceMsg->timeToLive());

    //TODO check dest Path MTU here
    ctrIP6OutRequests++;

/*
    // pass Datagram through netfilter if it exists
    if (hasHook)
    {
        send(datagram.release(), "netfilterOut");
        cMessage *dfmsg = receiveNewOn("netfilterIn");
        if (dfmsg->kind() == DISCARD_PACKET)
        {
            delete dfmsg;

            return;
        }
        datagram.reset(static_cast<IPv6Datagram*>(dfmsg));
    }
*/

    curPacket = datagram.release();  // XXX rather

}

void IPv6SendCore::finish()
{
  recordScalar("IP6OutRequests", ctrIP6OutRequests);
}

