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
 * @file   UDPVideoStream.cc
 * @author Johnny Lai
 * @date   25 May 2004
 *
 * @brief  Implementation of UDPVideoStream
 *
 */

//Headers for libcwd debug streams have to be first (remove if not used)
#include "sys.h"
#include "debug.h"

#include <iostream>
#include <sstream>
#include <boost/cast.hpp>
#include <memory> //auto_ptr

#include "UDPVideoStream.h"
#include "UDPMessages_m.h"

Define_Module(UDPVideoStream);

using std::endl;

void UDPVideoStream::initialize()
{
  server = false; // must be before we invoke base class (this is bad design, but udpapplication needs to be rewritten anyway)
  UDPApplication::initialize();

  address = (const char*) par("UDPServerAddress");

  //server Address from UDPApplication

  if (!server)
    svrPort = par("UDPPort");

  startTime = par("startTime");

  Dout(dc::udp_video_svr, fullPath()<<" pars for app "<<className()
       <<" startTime="<<startTime<<" svrPort="<<svrPort);

  if (startTime != 0)
    scheduleAt(startTime, new cMessage("UDPVideoStreamStart"));
}

void UDPVideoStream::finish()
{
}

void UDPVideoStream::handleMessage(cMessage* theMsg)
{
  std::auto_ptr<cMessage> msg(theMsg);
  if (msg->isSelfMessage())
  {
    if (isReady())
      requestStream();
    else
      Dout(dc::udp|dc::warning, fullPath()<<" Trying to send when port was not bound");
    return;
  }

  if (!isReady())
  {
    UDPApplication::handleMessage(msg.get());
    return;
  }

  if (!eed.valuesStored())
  {
    std::ostringstream os;
    os<<className()<<":"<<port<<" eed";
    eed.setName(os.str().c_str());
  }
  receiveStream(msg.get());
}

void UDPVideoStream::requestStream()
{
  UDPPacketBase* udpPkt = new UDPPacketBase;
  udpPkt->setSrcPort(port);
  udpPkt->setDestPort(svrPort);
  UDPAppInterfacePacket* pkt = new UDPAppInterfacePacket;
  pkt->setKind(KIND_DATA);
  pkt->setDestIPAddr(c_ipv6_addr(address.c_str()));
  pkt->setName("reqVidStm");
  pkt->encapsulate(udpPkt);
  send(pkt, gateId);
  Dout(dc::udp_video_svr, fullPath()<<" requesting video stream from "
       <<pkt->getDestIPAddr()<<":"<<svrPort);
}

void UDPVideoStream::receiveStream(cMessage* msg)
{
   UDPAppInterfacePacket* pkt =
     check_and_cast<UDPAppInterfacePacket*>(msg);
   UDPPacketBase* udpPkt =
     check_and_cast<UDPPacketBase*>(pkt->encapsulatedMsg());
   assert(port == udpPkt->getDestPort());
   Dout(dc::udp_video_svr|flush_cf, className()<<":"<<port<<" received packet from "
    <<pkt->getSrcIPAddr()<<":"<<udpPkt->getSrcPort()<<" len="<<udpPkt->length());
   eed.record(simTime() - udpPkt->timestamp());

   //Check for ending kind in udpPkt
   if (udpPkt->kind() == 0)
     callFinish();
}

