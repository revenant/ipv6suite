// -*- C++ -*-
// Copyright (C) 2001, 2003, 2004 CTIE, Monash University
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

    @file IPv6OutputCore.h
    @brief Header file for IPv6Output core module
    ------
    Responsibilities:
    receive complete datagram from IPFragmentation
        hop counter check
            -> throw away and notify ICMP if ttl==0
        otherwise  send it on to output queue
    @author Johnny Lai

*/

#include "RoutingTable6Access.h"

#ifndef IPv6OUTPUTCORE_H
#define IPv6OUTPUTCORE_H


class IPv6Datagram;
class LLInterfaceInfo;
template<typename T> class  cTypedMessage;
typedef cTypedMessage<LLInterfaceInfo> LLInterfacePkt;
class IPv6ForwardCore;

/**
 * @class IPv6OutputCore
 * @brief All datagrams pass through here before going down to layer two
 */

class IPv6OutputCore: public RoutingTable6Access
{
public:
  Module_Class_Members(IPv6OutputCore, RoutingTable6Access, 0);

  virtual void initialize();
  virtual void handleMessage(cMessage* msg);
  virtual void finish();

private:
  LLInterfacePkt* processArrivingMessage(cMessage* msg);

  simtime_t delay;
  bool hasHook;
  unsigned int ctrIP6OutForwDatagrams;
  unsigned int ctrIP6OutMcastPkts;
  LLInterfacePkt* curPacket;
  cMessage* waitTmr;
  cQueue waitQueue;
  ::IPv6ForwardCore* forwardMod;
};

#endif

