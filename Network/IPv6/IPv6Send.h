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

class IPv6InterfacePacket;
class IPv6Datagram;
class RoutingTable6;
class InterfaceTable;

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
  IPv6Datagram *encapsulatePacket(IPv6InterfacePacket *interfaceMsg);

  InterfaceTable *ift;
  RoutingTable6 *rt;

  int defaultTimeToLive;
  int defaultMCTimeToLive;

  // As we only do fragmentation at source perhaps we can streamline stack by
  // Moving frag to local IPv6 Out.  However this is not correct because during
  // Routing the src address can change when routing header is encountered.

  unsigned int ctrIP6OutRequests;
};

#endif

