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

#ifndef __IPv6FORWARD_H__
#define __IPv6FORWARD_H__

#include "ipv6_addr.h"
#include "QueueBase.h"

class RoutingTable6;
class InterfaceTable;
class IPv6Datagram;
class ICMPv6Message;


/**
 * @class IPv6Forward
 * @brief Process datagrams and determine where they go
 */

namespace IPv6NeighbourDiscovery
{
  class NDStateRouter; //for sending redirect
};

class IPv6Forward : public QueueBase
{
public:
  Module_Class_Members(IPv6Forward, QueueBase, 0);

  virtual void initialize(int stage);
  virtual void endService(cMessage* theMsg);
  virtual int  numInitStages() const  {return 4;}
  virtual void finish();

  //par from ini file
  bool routingInfoDisplay;

private:
  InterfaceTable *ift;
  RoutingTable6 *rt;

  unsigned int ctrIP6InAddrErrors;

  bool processReceived(IPv6Datagram& datagram);

  void sendErrorMessage (ICMPv6Message* err);

  ///Used for sending redirects on our behalf
  IPv6NeighbourDiscovery::NDStateRouter* nd;

};

#endif

