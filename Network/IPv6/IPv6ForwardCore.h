// -*- C++ -*-
//
// Copyright (C) 2001, 2003 CTIE, Monash University
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
    @file IPv6Forward.h
    @brief Implementation of IP packet forwarding via table lookups

    Responsibilities:
        -Receive valid IPv6 datagram
        -send datagram with Multicast addr. to Multicast module
        -Forward to IPv6Encapsulation module if next hop is a tunnel entry point
        -Drop datagram away and notify ICMP if dest. addr not in routing table
        -send to local Deliver if dest. addr. = 127.0.0.1
         or dest. addr. = NetworkCardAddr.[]
         otherwise, send to Fragmentation module
        -Check hopLimit and drop or decrement at end of routing header proc.
    Error Messages
       ICMP Time exceeded when hopLimit of packet is at 0
    @author Johnny Lai
    @date 28/08/01
*/

#ifndef ROUTING6CORE_H
#define ROUTING6CORE_H

#ifndef MAP
#define MAP
#include <map>
#endif //MAP
#ifndef VECTOR
#define VECTOR
#include <vector>
#endif //VECTOR

#ifndef BOOST_SHARED_PTR_HPP
#include <boost/shared_ptr.hpp>
#endif //BOOST_SHARED_PTR_HPP
#ifndef FUNCTIONAL
#include <functional>
#endif //FUNCTIONAL

#ifndef ROUTINGTABLE6ACCESS_H
#include "RoutingTable6Access.h"
#endif //ROUTINGTABLE6ACCESS_H
#ifndef IPv6_ADDR_H
#include "ipv6_addr.h"
#endif //IPv6_ADDR_H
#ifndef IPV6DATAGRAM_H
#include "IPv6Datagram.h"
#endif //IPV6DATAGRAM_H

//The last route is always the dest
typedef vector<ipv6_addr> _SrcRoute;
typedef boost::shared_ptr< _SrcRoute > SrcRoute;

class ipv6AddrHash: public unary_function<ipv6_addr, size_t>
{
public:

  size_t operator()(const ipv6_addr& addr) const
    {
      return static_cast<size_t> ((addr.extreme || addr.low) &&
                                  (addr.normal || addr.high));
    }
};


/**
 * @class IPv6Forward
 * @brief Process datagrams and determine where they go
 */
class ICMPv6Message;
class AddrResInfo; //remove this dependency when conceptual sending is removed
class RoutingTable6;
class IPv6Encapsulation;
namespace IPv6NeighbourDiscovery
{
  class NDStateRouter; //for sending redirect
};

class IPv6Forward : public cSimpleModule
{
  friend std::ostream& operator<<(std::ostream & os,
                                  IPv6Forward& routeMod);
public:
  Module_Class_Members(IPv6Forward, cSimpleModule, 0);

  virtual void initialize(int stage);
  virtual void handleMessage(cMessage* theMsg);
  virtual int  numInitStages() const  {return 4;}
  virtual void finish();

  void addSrcRoute(const SrcRoute& routes);

  /**
     Return the interface index to send the packets with dest as
     destination address.
     @param info is assigned with the link local address and ifIndex too
     -1 if destination is not in routing table
     -2 if packets are pending addr res
  */
  //migrate to Routing
  int conceptualSending(AddrResInfo& info);

  ///Return the src address for a packet going out on ifIndex to dest
  //migrate to Routing?
  ipv6_addr determineSrcAddress(const ipv6_addr& dest, size_t ifIndex);

  bool insertSourceRoute(IPv6Datagram& datagram);

  ////Determine if the address is a local one, ie for delivery to localhost
  //bool localDeliver(const ipv6_addr& dest);

  unsigned int ctrIP6OutNoRoutes;

  //par from ini file
  bool routingInfoDisplay;

private:
  RoutingTable6 *rt;

  simtime_t delay;
  bool hasHook;
  unsigned int ctrIP6InAddrErrors;

  bool processReceived(IPv6Datagram& datagram);

  void sendErrorMessage (ICMPv6Message* err);

  //typedef std::hash_map<ipv6_addr, SrcRoute, ipv6AddrHash, less<ipv6_addr> > SrcRoutes;
  typedef std::map<ipv6_addr, SrcRoute> SrcRoutes;
  typedef SrcRoutes::iterator SRI;

  ///Preconfigured source routes.  Only 1 preconfigured source route per
  ///destination. New ones to same destination will replace existing ones
  ///without warning.
  SrcRoutes routes;

  ///Wait call is emulated with this timer
  cMessage* waitTmr;
  ///Current packet awaiting processing after wait delay
  IPv6Datagram* curPacket;
  ///Arriving packets are placed in queue first if another packet is awaiting
  ///processing
  cQueue waitQueue;

  ///Used for sending redirects on our behalf
  IPv6NeighbourDiscovery::NDStateRouter* nd;
  IPv6Encapsulation* tunMod;

};

std::ostream& operator<<(std::ostream & os, IPv6Forward& routeMod);

#endif
