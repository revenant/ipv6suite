// -*- C++ -*-
//
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
    @file IPv6Send.h
    @brief IPv6Send simple module definition

    Responsibilities:
    receive IPInterfacePacket from Transport layer or ICMP
    or Tunneling (IP tunneled datagram)
    take out control information
    encapsulate data packet in IP datagram
    set version
    set hoplimit according to router's advertised value
    set Protocol to received value
    set destination address to received value
    send datagram to Routing

    if IPInterfacePacket is invalid (e.g. invalid source address),
     it is thrown away without feedback

    @author Johnny Lai
*/

#ifndef IPv6SEND_H
#define IPv6SEND_H

#include "QueueBase.h"

//SrcRoute includes
#include <map>
#include <vector>
#include <boost/shared_ptr.hpp>
#include "ipv6_addr.h"

class IPv6Datagram;
class InterfaceTable;
class RoutingTable6;
class IPv6Mobility;

//The last route is always the dest
typedef vector<ipv6_addr> _SrcRoute;
typedef boost::shared_ptr<_SrcRoute> SrcRoute;

/**
 * @class IPv6Send
 *
 * @brief Datagrams sent from upper layers arrive at the IP Layer here first to be
 * encapsulated by datagrams
 */

class IPv6Send : public QueueBase
{

public:
  Module_Class_Members(IPv6Send, QueueBase, 0);

  virtual void initialize();
  virtual void endService(cMessage *msg);
  virtual void finish();

private:
  IPv6Datagram *encapsulatePacket(cMessage *msg);
  bool insertSourceRoute(IPv6Datagram& datagram);

  void parseSourceRoutes();

  InterfaceTable *ift;
  RoutingTable6 *rt;
  IPv6Mobility *mob;

  /*
  class ipv6AddrHash: public unary_function<ipv6_addr, size_t>
  {
  public:

    size_t operator()(const ipv6_addr& addr) const
    {
      return static_cast<size_t> ((addr.extreme || addr.low) &&
                                  (addr.normal || addr.high));
    }
  };

  typedef std::hash_map<ipv6_addr, SrcRoute, ipv6AddrHash, less<ipv6_addr> > SrcRoutes;
  */
  ///Route is searched according to final dest (last address in SrcRoute).  The
  ///src address of packet is used to determine outgoing iface.
  typedef std::map<ipv6_addr, SrcRoute> SrcRoutes;
  typedef SrcRoutes::iterator SRI;

  ///Preconfigured source routes.  Only 1 preconfigured source route per
  ///destination. New ones to same destination will replace existing ones
  ///without warning.
  SrcRoutes routes; 

  int defaultTimeToLive;
  int defaultMCTimeToLive;

  // As we only do fragmentation at source perhaps we can streamline stack by
  // Moving frag to local IPv6 Out.  However this is not correct because during
  // Routing the src address can change when routing header is encountered.

  unsigned int ctrIP6OutRequests;
  unsigned int ctrIP6OutNoRoutes;

};

#endif

