// -*- C++ -*-
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
  @file IPv6Multicast.h
  @brief Definition of IPv6Multicast module

  Responsibilities:
  receive datagram with Multicast address from Routing
  duplicate datagram if it is sent to more than one output port
  map multicast address on output port, use multicast routing table
  send copy to  local deliver, if
  NetworkCardAddr.[] is part of multicast address
  if entry in multicast routing table requires tunneling,
  send to Tunneling module
  otherwise send to Fragmentation module

  receive IGMP message from LocalDeliver
  update multicast routing table

  @author Johnny Lai
  Based on IPMulticast by Jochen Reber
*/

#ifndef IPv6MULTICAST_H
#define IPv6MULTICAST_H

#ifndef ROUTINGTABLE6ACCESS_H
#include "RoutingTable6Access.h"
#endif //ROUTINGTABLE6ACCESS_H
#ifndef STRING
#define STRING
#include <string>
#endif //STRING

class IPv6Datagram;
class IPv6Forward;
struct ipv6_addr;

/**
 * @class IPv6Multicast
 * @brief Handle sending and forwarding of multicast packets
 */

class IPv6Multicast : public cSimpleModule
{

public:
  Module_Class_Members(IPv6Multicast, cSimpleModule, 0);

  virtual void initialize();
  virtual void handleMessage(cMessage*);
  virtual void finish();

  static std::string multicastLLAddr(const ipv6_addr& addr);
private:
  void dupAndSendPacket(const IPv6Datagram* datagram, size_t ifIndex);

  RoutingTable6 *rt;

  simtime_t delay;
  IPv6Forward* fc;
  cMessage* waitTmr;
  cMessage* msg;

  unsigned int ctrIP6InMcastPkts;
};

#endif //IPv6MULTICAST_H

