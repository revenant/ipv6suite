// -*- C++ -*-
// Copyright (C) 2004 Johnny Lai
//
// This file is part of IPv6Suite
//
// IPv6Suite is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// IPv6Suite is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.


/**
 * @file   UDPApplication.cc
 * @author Johnny Lai
 * @date   25 May 2004
 *
 * @brief  Implementation of UDPApplication
 *
 *
 */

//Headers for libcwd debug streams have to be first (remove if not used)
#include "sys.h"
#include "debug.h"

#include <boost/cast.hpp>

#include "UDPApplication.h"
#include "UDPMessages_m.h"

using std::string;

int UDPApplication::gateId = -1;

Define_Module(UDPApplication);

void UDPApplication::initialize()
{
  RoutingTable6Access::initialize();
  server = par("server");
  if (server)
  {
    port = par("UDPPort");
  }
  address = (const char*) par("UDPServerAddress");
  gateId = findGate("physOut");
  bindPort();
  bound = false;
}

void UDPApplication::finish()
{
}

void UDPApplication::handleMessage(cMessage* msg)
{
  if (!bound && msg->kind() != KIND_BIND)
    DoutFatal(dc::core|dc::udp, fullPath()<<" should send bind message to UDP layer first") ;
  if (!bound && msg->kind() == KIND_BIND)
  {
    if (!server)
      port = (check_and_cast<UDPPacketBase*>(msg))->getSrcPort();

    //for ports that fail to bind on specific port UDP will never send back
    //"ack" so clients should check if isReady() is true before initiating listening/sending mode
    bound = true;
    Dout(dc::udp|dc::notice, fullPath()<<" port "<<port
         <<" bound for application "<<className());
    return;
  }
  assert(bound && msg->kind() == KIND_DATA);
}

void UDPApplication::bindPort()
{
  UDPPacketBase* udpPkt = new UDPPacketBase("bindPort");
  udpPkt->setKind(KIND_BIND);
  if (server)
    udpPkt->setSrcPort(port);

  sendDelayed(udpPkt, 0.1, gateId);
}
