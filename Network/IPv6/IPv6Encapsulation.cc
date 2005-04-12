//
// Copyright (C) 2002, 2004 CTIE, Monash University
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


#include "sys.h"
#include "debug.h"

#include <omnetpp.h>
#include <stdlib.h>
#include <memory> //auto_ptr
#include <algorithm>
#include <iterator> //ostream_iterator

#include "IPv6Encapsulation.h"
#include "IPv6ControlInfo_m.h"
#include "ipv6addrconv.h"
#include "InterfaceTableAccess.h"
#include "RoutingTable6Access.h"
#include "Constants.h"
#include "IPv6Datagram.h"
#include "NDEntry.h"
#include "opp_utils.h"
#include "IPv6CDS.h"
#include "cTTimerMessageCB.h"
#include <memory>

using IPv6NeighbourDiscovery::NeighbourEntry;
using std::auto_ptr;

IPv6Encapsulation::Tunnel::Tunnel(const ipv6_addr& src,
                                  const ipv6_addr& dest,
                                  size_t oifIndex, bool forOnePkt)
  :entry(src), exit(dest), ifIndex(oifIndex),
   tunnelMTU(IPv6_MIN_MTU - 20), forOnePkt(forOnePkt)
{}

IPv6Encapsulation::Tunnel::~Tunnel()
{}

Define_Module( IPv6Encapsulation );


int IPv6Encapsulation::numInitStages() const
{
  return 2;
}

void IPv6Encapsulation::initialize(int stageNo)
{
  if (stageNo == 0)
  {
    ift = InterfaceTableAccess().get();
    rt = RoutingTable6Access().get();

    mipv6CheckTunnelCB = 0;
    delay = par("procDelay");

    trafficClass = 0; //Use the value of 0 which is protocol default
    tunHopLimit = 0; //0 to use defaults again
    ///not implemented 6.6
    encapLimit = 4;
    vIfIndexTop = UINT_MAX - ift->numInterfaces() - 1;
  }
}

/**
 * Create tunnel and create lookup entries by inserting into Dest Cache
 * Returns the virtual interface index or 0 if tunnel not created
 *
 * @param src tunnel entry point
 * @param dest tunnel exit point
 * @param ifIndex is the outgoing interface of encapsulated packet
 * @param destTrigger packets with destination address that match will be
 * encapsulated by this tunnel
 * @param forOnePkt When true tunnel is destroyed after one packet goes through it
 * @return virtual interface id (always greater than the actual coqunt of no. of
 * real interfaces.  Starts from 2^32-3 and decrements after every successful
 * addition
 *
 */

size_t IPv6Encapsulation::createTunnel(const ipv6_addr& src, const ipv6_addr& dest,
                                     size_t ifIndex, const ipv6_addr& destTrigger, bool forOnePkt)
{
  assert(findTunnel(src, dest) == 0);

  //Test for entry and exit point node pointing to same node i.e. localDeliver
  //addresses to prevent loopback encapsulation 4.1.2
  if (rt->localDeliver(src) && rt->localDeliver(dest))
  {

    cerr<<rt->nodeId()<<":"<<name()<<": Cannot create tunnel with local endpoints"
        <<" (prevents loopback tunneling)"<<endl;
    return 0;
  }

  tunnels[vIfIndexTop] = Tunnel(src, dest, ifIndex, forOnePkt);


  if (destTrigger != IPv6_ADDR_UNSPECIFIED)
    tunnelDestination(destTrigger, vIfIndexTop);

  return vIfIndexTop--;

}

/**
 * @param dest packets with this matching destination address will be
 * encapsulated by tunnel
 * @param vIfIndex is the virtual interface index of tunnel
 * @return true if destination can be forwarded to tunnel false otherwise
 */
bool IPv6Encapsulation::tunnelDestination(const ipv6_addr& dest, size_t vIfIndex)
{
  if (tunnels.count(vIfIndex) > 0)
  {
    Tunnel& tun = tunnels[vIfIndex];
    if (tun.ne.get() == 0)
      tun.ne.reset(new NeighbourEntry(tun.entry, vIfIndex, 0,
                                      NeighbourEntry::REACHABLE));
    (*(rt->cds))[dest].neighbour = tun.ne;


    return true;
  }
  return false;
}

/**
 * @param dest packets with this matching destination prefix will be
 * encapsulated by tunnel
 * @param vIfIndex is the virtual interface index of tunnel
 * @return true if destination can be forwarded to tunnel false otherwise
 */
bool IPv6Encapsulation::tunnelDestination(const IPv6Address& dest,
        size_t vIfIndex)
{
  Dout(dc::encapsulation|flush_cf, "Tunnel Dest pref: addrObj=" << dest
    << " prefixLen=" << dest.prefixLength());
  if (tunnels.count(vIfIndex) > 0)
  {
    Tunnel& tun = tunnels[vIfIndex];
    if (tun.ne.get() == 0)
      tun.ne.reset(new NeighbourEntry(tun.entry, vIfIndex, 0,
                                      NeighbourEntry::REACHABLE));
    /// attempts to preserve prefix length
    (*(rt->cds))[dest].neighbour = tun.ne;


    return true;
  }
  return false;
}

///Returns the vIfIndex of tunnel if found, 0 otherwise
size_t IPv6Encapsulation::findTunnel(const ipv6_addr& src, const ipv6_addr& dest)
{
  Dout(dc::debug|flush_cf, "findTunnel entry="<<src<<" exit="<<dest);
  //Check that no existing tunnel has the same value i.e. src/dest address equivalent
  TI it = find_if(tunnels.begin(), tunnels.end(),
                  bind1st(equalTunnel(),
                          make_pair((size_t) 0, Tunnel(src, dest)) ));
  if (it != tunnels.end())
    return it->first;
  return 0;
}

///Remove tunnel and the associated entries in Dest Cache
bool IPv6Encapsulation::destroyTunnel(const ipv6_addr& src, const ipv6_addr& dest)
{
  Dout(dc::encapsulation|flush_cf, "destroy tunnel entry="<<src<<" exit="<<dest);
  //remove all dest entries with this as link and delete ne
  size_t vIfIndex = findTunnel(src, dest);
  if (vIfIndex == 0)
  {
    Dout(dc::warning|dc::encapsulation|flush_cf, "tunnel not found entry="<<src<<" exit="<<dest);
    return false;
  }

  tunnels.erase(vIfIndex);
  return true;
}

bool IPv6Encapsulation::destroyTunnel(size_t vIfIndex)
{
  Dout(dc::encapsulation, " tunnels are "<<*(this));

  if (tunnels.count(vIfIndex))
    Dout(dc::encapsulation|flush_cf, "destroy tunnel vIfIndex="<<hex<<vIfIndex<<dec
         <<" "<<tunnels[vIfIndex]);
  bool erased = tunnels.erase(vIfIndex);
  if (!erased)
    Dout(dc::warning|dc::encapsulation|flush_cf, "tunnel not found vIfIndex="<<hex<<vIfIndex<<dec);
  
  return erased;
}

std::ostream& operator<<(std::ostream & os, const IPv6Encapsulation::Tunnel& tun)
{
  os<<" entry="<<tun.entry<<" exit="<<tun.exit<<" ifIndex="<<tun.ifIndex<<" tunnelMTU="
    <<tun.tunnelMTU<<" ngbr=";
    if (tun.ne)
      os<<*(tun.ne);
    else
      os<<"none";
  return os;
}

std::ostream & operator<<
  (std::ostream& os, const pair<const size_t, IPv6Encapsulation::Tunnel> & p)
{
  os <<"Tunnel vifIndex="<<hex<<p.first <<dec<<p.second;
  return os;
}

std::ostream& operator<<(std::ostream & os, const IPv6Encapsulation& tunMod)
{
  copy(tunMod.tunnels.begin(), tunMod.tunnels.end(), ostream_iterator<
       IPv6Encapsulation::Tunnels::value_type >(os, "\n"));

  return os;
}

void IPv6Encapsulation::registerMIPv6TunnelCallback(TFunctorBaseA<IPv6Datagram>* cb)
{
  if (mipv6CheckTunnelCB != cb)
    delete mipv6CheckTunnelCB;
  mipv6CheckTunnelCB = cb;
}

void IPv6Encapsulation::handleMessage(cMessage* msg)
{
  auto_ptr<IPv6Datagram> dgram(check_and_cast<IPv6Datagram*> (msg));


  if (!strcmp(dgram->arrivalGate()->name(), "encapsulateRoutingIn") ||
      !strcmp(dgram->arrivalGate()->name(), "mobilityIn"))
  {
    auto_ptr<IPv6Datagram> origDgram(dgram);

    //outputPort() returns int and on TRU64 as sizeof(size_t) = 8 and unsigned
    //int 4 so this is necessary
    unsigned int ifIndex = origDgram->outputPort();

    if (!tunnels.count(ifIndex))
    {
      //We should send an error message as we can't find the tunnel referred to
      cerr<<rt->nodeName()<<" - tunnel vifIndex="<<hex<<ifIndex<<dec
          <<" does not exist anymore." <<endl;
      cerr<<"We should delete all dest entries pointing at this tunnel"<<endl;
      Dout(dc::warning|error_cf, rt->nodeName()<<" - tunnel vifIndex="<<hex<<ifIndex<<dec
           <<" does not exist anymore. (todo delete all dest entries that refer to tunnel)");
      return;
    }

    Tunnel& tun = tunnels[ifIndex];

    //reject encapsulating entry/exit node addresses same as original src/dest
    //address of packet 4.1.2
    if (tun.entry == origDgram->srcAddress() &&
        tun.exit == origDgram->destAddress())
    {
      cerr<<"Cannot tunnel packet with entry, exit point same as src,"
          <<" dest of original packet"<<endl;
      Dout(dc::warning|dc::encapsulation, "Cannot tunnel packet with entry/exit point same as src/"
           <<"dest of inner packet");
      return;
    }

    //Set up hoplimit of original packet if necessary.  This is done
    //by fragmentation but since packet is encapsulated we need it here.
    if (origDgram->inputPort() == IMPL_INPUT_PORT_LOCAL_PACKET)
    {
      //Uninitialised hopLimit
      if (origDgram->hopLimit() == 0)
      {
        if (!rt->isRouter())
          origDgram->setHopLimit(ift->interfaceByPortNo(0)->ipv6()->curHopLimit);
        else
          origDgram->setHopLimit(DEFAULT_ROUTER_HOPLIMIT);
      }

      //locally generated datagram so decrement hopLimit
      origDgram->setHopLimit(origDgram->hopLimit() - 1);
    }

    IPv6ControlInfo *ctrl = new IPv6ControlInfo;
    ctrl->setSrcAddr(mkIPv6Address_(tun.entry));
    ctrl->setDestAddr(mkIPv6Address_(tun.exit));
    ctrl->setProtocol(IP_PROT_IPv6);

    //Set tunnel packet properties (no support for tunnel encapsulation
    //limit option
    if (rt->isRouter() && tunHopLimit == 0)
      ctrl->setTimeToLive(DEFAULT_ROUTER_HOPLIMIT);
    else if (tunHopLimit != 0)
      ctrl->setTimeToLive(tunHopLimit);
    else
      ctrl->setTimeToLive(ift->interfaceByPortNo(tun.ifIndex)->ipv6()->curHopLimit);

    origDgram->setControlInfo(ctrl);

    sendDelayed(origDgram.release(), delay, "encapsulatedSendOut");

    if (tun.forOnePkt)
      destroyTunnel(tun.entry, tun.exit);
  }
  else if (!strcmp(dgram->arrivalGate()->name(), "decapsulateLocalDeliverIn"))
  {
    auto_ptr<IPv6Datagram> tunDgram(dgram);
    assert(tunDgram->transportProtocol() == IP_PROT_IPv6);
    if (tunDgram->transportProtocol() != IP_PROT_IPv6)
    {
      cerr<<"Unknown tunnel Packet "
          <<static_cast<int> (tunDgram->transportProtocol())<<endl;

      return;
    }

#ifdef USE_MOBILITY
    ///callback for MIPv6: check for need to send BU to CN or HA for route
    ///Optimsation
    if (mipv6CheckTunnelCB && rt->isMobileNode())
      (*mipv6CheckTunnelCB)(tunDgram.get());
#endif //USE_MOBILITY

    auto_ptr<IPv6Datagram> origDgram(
      check_and_cast<IPv6Datagram*>(tunDgram->decapsulate()));

    ///Preserve the original dgram's input port in decapsulated packets output
    ///port since if decap packet is delivered locally outputPort is unused.
    if (tunDgram->inputPort() > IMPL_INPUT_PORT_LOCAL_PACKET)
      origDgram->setOutputPort(tunDgram->inputPort());
#if !defined NOIPMASQ
    //Actually this is very simple NAT to trick router into thinking it
    //originated this packet itself so that it will send it off instead of
    //trying to forward it which it is prohibited from doing when the src/dest
    //addr are link local. Real NAT does not actually have these limitations.
    origDgram->setInputPort(IMPL_INPUT_PORT_LOCAL_PACKET);
#endif //!NOIPMASQ


    sendDelayed(origDgram.release(), delay/2, "decapsulatedPreRoutingOut");

  }
  else
  {
    assert(false);
    cerr<<rt->nodeId()<<":"<<name()<<": Unexpected message received \" "
        <<dgram->arrivalGate()->name()<<"\""<<endl;

  }

}


void IPv6Encapsulation::finish()
{
  // cleanup stuff must be moved to dtor! --AV
  delete mipv6CheckTunnelCB;
  mipv6CheckTunnelCB = 0;
}

