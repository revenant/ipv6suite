// -*- C++ -*-
// $Header: /home/cvs/IPv6Suite/IPv6SuiteWithINET/Network/IPv6/IPv6Fragmentation.h,v 1.1 2005/02/09 06:15:58 andras Exp $
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
	@file IPv6Fragmentation.h
	@brief Header file for IPv6Fragmentation

	Responsibilities: 
	   -receive valid IP datagram from Routing or Multicast
	   -Fragment datagram if size > MTU [output port]
	   -send fragments to IPOutput[output port]

    Error Messages
       -ICMP Packet Too Big when link MTU is less than packet size

    @author Johnny Lai
*/

#ifndef IPFRAGMENTATION_H
#define IPFRAGMENTATION_H

#ifndef ROUTINGTABLE6ACCESS_H
#include "RoutingTable6Access.h"
#endif //ROUTINGTABLE6ACCESS_H

class ICMPv6Message;
struct AddrResInfo;
template <typename T> class cTypedMessage;
typedef cTypedMessage<AddrResInfo> AddrResMsg;

/**
 * @class IPv6Fragmentation
 * @brief Fragmentation module for IPv6 
 *
 * Currently fragmentation is not well tested so if something bigger than mtu is
 * sent problems are likely to occur.
 */

class IPv6Fragmentation:public RoutingTable6Access
{
public:
  Module_Class_Members(IPv6Fragmentation, RoutingTable6Access, 0);

  virtual void initialize();
  virtual void handleMessage(cMessage*);
  virtual void finish();

private:

  void sendErrorMessage(ICMPv6Message* err);
  void sendOutput(AddrResMsg* msg );  

  int numOfPorts;
  simtime_t delay;
  cMessage* waitTmr;
  AddrResMsg*  curPacket;
  ///Arriving packets are placed in queue first if another packet is awaiting
  ///processing
  cQueue waitQueue;
  unsigned int mtu;

  unsigned int ctrIP6InTooBig;
  unsigned int ctrIP6FragCreates;
  unsigned int ctrIP6FragFails;
  unsigned int ctrIP6FragOKs;
  

};

#endif

