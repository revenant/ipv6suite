//
// Copyright (C) 2004 CTIE, Monash University
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
   @file IPv6Fragmentation.cc
   @brief Implementation for IPv6Fragmentation

   Responsibilities:
   receive valid IP datagram from Routing or Multicast
   Fragment datagram if size > MTU [output port] && src_addr == node_addr
   send fragments to IPOutput[output port]

   @author Johnny Lai

   Based on IPFragmentation by Jochen Reber
*/
#include "sys.h"
#include "debug.h"

#include <cassert>

#include "IPv6Fragmentation.h"
#include "RoutingTable6.h"
#include "Constants.h"
#include "HdrExtFragProc.h"
#include "IPv6Datagram.h"
#include "ICMPv6Message.h"
#include "IPv6InterfaceData.h"
#include "AddrResInfo_m.h"
#include "LL6ControlInfo_m.h"
#include "AddrResInfo_m.h"

Define_Module( IPv6Fragmentation );

/*  ----------------------------------------------------------
        Public Functions
    ----------------------------------------------------------  */

void IPv6Fragmentation::initialize()
{
  QueueBase::initialize();
  rt = RoutingTable6Access().get();

  numOfPorts = par("numOfPorts");

  ctrIP6InTooBig = 0;
  ctrIP6FragCreates = 0;
  ctrIP6FragFails = 0;
  ctrIP6FragOKs = 0;
}

/**
 * @todo Use Path MTU in Destination Entry once that's implemented instead of
 * just the device MTU
 * TODO delay per fragmented datagram sent
 */
void IPv6Fragmentation::endService(cMessage* msg)
{
  IPv6Datagram*  datagram = check_and_cast<IPv6Datagram*> (msg);
  AddrResInfo *info = check_and_cast<AddrResInfo*>(datagram->controlInfo());
  Interface6Entry* ie = rt->getInterfaceByIndex(info->ifIndex());
  mtu = ie->mtu;

  assert(mtu >= IPv6_MIN_MTU); //All IPv6 links must conform


  if (datagram->inputPort() == -1 && datagram->hopLimit() == 0)
  {
    if (!rt->isRouter())
      datagram->setHopLimit(ie->curHopLimit);
    else
      datagram->setHopLimit(DEFAULT_ROUTER_HOPLIMIT);
  }

  // check if datagram does not require fragmentation
  if (datagram->totalLength() <= mtu)
  {
    sendOutput(datagram);
    return;
  }

  //Source fragmentation only in IPv6
  if (datagram->inputPort() != -1 ||
      //Do not fragment ICMP
      datagram->transportProtocol() == IP_PROT_IPv6_ICMP)
  {
    //ICMP packets do come through here however they should be limited
    //in size during creation time. Either that or we drop them
    assert(datagram->transportProtocol() != IP_PROT_IPv6_ICMP);
    ICMPv6Message* err = new ICMPv6Message(ICMPv6_PACKET_TOO_BIG);
    err->encapsulate(datagram->dup());
    err->setName("ICMPv6_ERROR:PACKET_TOO_BIG");
    err->setOptInfo(mtu);
    sendErrorMessage(err);
    if (datagram->inputPort() != -1) //Tried to forward a big packet
      ctrIP6InTooBig++;
    delete datagram;
    return;
  }

  // TBD test fragmentation code
  error("fragmentation not implemented");

  HdrExtFragProc* fragProc = datagram->acquireFragInterface();
  assert(fragProc != 0);
  if (!fragProc)
    ctrIP6FragFails++;

  unsigned int noOfFragments = 0;
  IPv6Datagram** fragment = fragProc->fragmentPacket(datagram, mtu, noOfFragments);
  for (size_t i=0; i<noOfFragments; i++)
  {
    //TBD duplicate ctrnInfo as well
    //AddrResMsg* duplicate = addrResMsg->dup();
    //duplicate->data().dgram = fragment[i];

    //We never had fragmentation anyway so don't worry about implementing wait
    //within loops or use a delay proportional to number of frag packets and
    //send them all out at once after delay.
    ctrIP6FragCreates++;
    //TBD sendOutput(duplicate);
  }
  delete [] fragment;
  delete datagram;
  ctrIP6FragOKs++;
}


void IPv6Fragmentation::finish()
{
  recordScalar("IP6InTooBigErrors", ctrIP6InTooBig++);
  recordScalar("IP6FragCreates", ctrIP6FragCreates);
  recordScalar("IP6FragFails", ctrIP6FragFails);
  recordScalar("IP6FragOKs", ctrIP6FragOKs);

}


/*  ----------------------------------------------------------
        Private Functions
    ----------------------------------------------------------  */

//  send error message to ICMP Module
void IPv6Fragmentation::sendErrorMessage(ICMPv6Message* err)
{
  send(err, "errorOut");
}

void IPv6Fragmentation::sendOutput(IPv6Datagram* datagram )
{
  // assign link-layer address to datagram, and send it out
  AddrResInfo *info = check_and_cast<AddrResInfo*>(datagram->removeControlInfo());
  int outputPort = info->ifIndex();
  if (outputPort >= numOfPorts)
    error("illegal output port %d", outputPort);

  LL6ControlInfo *ctrlInfo = new LL6ControlInfo();
  ctrlInfo->setDestLLAddr(info->linkLayerAddr());
  datagram->setControlInfo(ctrlInfo);
  delete info;
  send(datagram, "outputOut", outputPort);
}

