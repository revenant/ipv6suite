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
 * @file   UDPVideoStreamSvr.cc
 * @author Johnny Lai
 * @date   01 Jun 2004
 *
 * @brief  Implementation of UDPVideoStreamSvr
 *
 *
 */

//Headers for libcwd debug streams have to be first (remove if not used)
#include "sys.h"
#include "debug.h"

#include <boost/cast.hpp>
#include <sstream>

#include "UDPVideoStreamSvr.h"
#include "UDPMessages_m.h"
#include "opp_akaroa.h"
#include "opp_utils.h" //auto_downcast

Define_Module(UDPVideoStreamSvr);

using std::endl;

void UDPVideoStreamSvr::initialize()
{
  server = true; // must be before we invoke base class (this is bad design, but udpapplication needs to be rewritten anyway)
  UDPApplication::initialize();
  assert(server == true);
  //Leave address for now but can be used for listening on certain interfaces
  //only

  minWaitInt = par("minWaitInt");
  maxWaitInt = par("maxWaitInt");
  minPacketLen = par("minPacketLen");
  maxPacketLen = par("maxPacketLen");
  videoSize = par("videoSize");
  speed = par("speed");

  assert(videoSize > 0);
  assert(maxWaitInt > 0);
  assert(maxPacketLen > 0);

  Dout(dc::udp_video_svr|flush_cf, fullPath()<<" pars for app "<<className()<<
       " minWaitInt="<<minWaitInt<<" maxWaitInt="<<maxWaitInt<<" minPacketLen="
       <<minPacketLen<<" maxPacketLen="<<maxPacketLen<<" videoSize="<<videoSize
       <<" speed="<<speed);

  cs = new ExpiryEntryList<ClientStreamData,
                  Loki::cTimerMessageCB<void, TYPELIST_1(ClientStreamData)> >
    (new Loki::cTimerMessageCB<void, TYPELIST_1(ClientStreamData)>
       (6789, this, this, &UDPVideoStreamSvr::transmitClientStream,
        "VideoStreamSvrThreads")
     );

}

void UDPVideoStreamSvr::finish()
{
}

static const float RNGFactor = 1000.0;

void UDPVideoStreamSvr::handleMessage(cMessage* theMsg)
{
  std::auto_ptr<cMessage> msg(theMsg);
  if (!isReady())
  {
    UDPApplication::handleMessage(msg.get());
    return;
  }

  if (msg->isSelfMessage())
  {
    //Timer for a particular client so send packet to them
    cTimerMessage* noDelete =
      check_and_cast<cTimerMessage*>(msg.release());
    noDelete->callFunc();
    return;
  }


  ///processRequest

   std::auto_ptr<UDPAppInterfacePacket> pkt =
    OPP_Global::auto_downcast<UDPAppInterfacePacket>(msg);

   std::string src = ipv6_addr_toString(pkt->getSrcIPAddr()); // FIXME why convert to string... Andras
   UDPPacketBase* udpPkt =
     check_and_cast<UDPPacketBase*>(pkt->encapsulatedMsg());
   double pause = OPP_UNIFORM(minWaitInt*RNGFactor, maxWaitInt*RNGFactor)/RNGFactor;

   ClientStreamData d(src, udpPkt->getSrcPort(), videoSize, pause + simTime());

   if (cs->findEntry(d))
     Dout(dc::udp_video_svr, fullPath()<<" "<<simTime()<<" "<<src<<":"<<d.port
          <<" requested stream again so it is now reset");
   else
     Dout(dc::udp_video_svr, fullPath()<<" "<<simTime()<<" "<<src<<":"<<d.port
           <<" requested stream");

   ///Re/start the video streaming based on pause calculated now
   cs->addEntry(d);

}

/*
  @arg c is rather misleading because we never actually assign the callback
  argument. Instead we get it from cs->smallestExpiryEntry.
 */
void UDPVideoStreamSvr::transmitClientStream(ClientStreamData& c)
{
  //Read in c
  assert(cs->smallestExpiryEntry(c));

  UDPPacketBase* rudpPkt = new UDPPacketBase;
  rudpPkt->setSrcPort(port);
  rudpPkt->setDestPort(c.port);
  //rudpPkt->setKind()  //Can be used for sequence numbers to detect missing pkts
  unsigned int pkt_size = (unsigned int) OPP_UNIFORM(minPacketLen, maxPacketLen);
  pkt_size = std::min(pkt_size, c.bytesLeft);
  rudpPkt->setLength(pkt_size);
  UDPAppInterfacePacket* rpkt = new UDPAppInterfacePacket;
  rpkt->setKind(KIND_DATA);
  rpkt->setDestIPAddr(c_ipv6_addr(c.host.c_str())); // FIXME yet another $#%# conversion....
  rpkt->encapsulate(rudpPkt);
  rudpPkt->setTimestamp(simTime());

  c.bytesLeft -= pkt_size;
  c.packetCount++;

  Dout(dc::udp_video_svr|continued_cf, fullPath()<<" "<<simTime()<<" packet of size "
       <<pkt_size<<" sent to "<<c.host<<":"<<c.port<<" bytes Left="<<c.bytesLeft);

  if (c.bytesLeft != 0)
  {
    double pause = OPP_UNIFORM(minWaitInt*RNGFactor, maxWaitInt*RNGFactor)/RNGFactor;
    c.txTime = simTime() + pause;
    cs->addEntry(c);
    Dout(dc::finish, " Stream scheduled for "<< c.txTime);
  }
  else
  {
    //end of stream indicator. This means seq no. start from 1.
    rudpPkt->setKind(0);

    cs->removeExpiredEntry(Loki::Int2Type<Loki::TypeTraits<CSEL::ElementType>::isPointer>());
    Dout(dc::finish, " Stream finished with"<<c.packetCount<<" packets sent");
    std::ostringstream os;
    os<<c.host<<":"<<c.port<<" packet count";
    recordScalar(os.str().c_str(), c.packetCount);
  }

  send(rpkt, gateId);

}
