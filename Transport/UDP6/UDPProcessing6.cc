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
 * @file   UDPProcessing6.cc
 * @author Johnny Lai
 * @date   21 May 2004
 *
 * @brief  Implementation of UDPProcessing6
 *
 *
 */

//Headers for libcwd debug streams have to be first (remove if not used)
#include "sys.h"
#include "debug.h"

#include <boost/cast.hpp>

#include "UDPProcessing6.h"
#include "UDPMessages_m.h"
#include "IPv6ControlInfo_m.h"
#include "ipv6addrconv.h"
#include "opp_utils.h"

const unsigned int UDP_HEADER_SIZE = 8;

Define_Module(UDPProcessing6);


void UDPProcessing6::initialize()
{
  ctrUdpOutDgrams = ctrUdpInDgrams = 0;
}

void UDPProcessing6::finish()
{
}

void UDPProcessing6::handleMessage(cMessage* theMsg)
{
  std::auto_ptr<cMessage> msg(theMsg);
  if (msg->kind() == KIND_BIND)
  {
    if (bind(msg.get()))
    {
      int gateId = msg->arrivalGate()->index();
      send(msg.release(), "to_app", gateId);
    }
  }
  else if (msg->kind() ==  KIND_DATA) // The message is from the application layer..
  {
    std::auto_ptr<UDPAppInterfacePacket> appIntPkt =
      OPP_Global::auto_downcast<UDPAppInterfacePacket>(msg);
    processApplicationMsg(appIntPkt.get());
  }
  else if (!strcmp(msg->arrivalGate()->name(), "from_ip"))
  {
    processNetworkMsg(msg.get());
  }
  else
    assert(false);
}

bool UDPProcessing6::bind(cMessage* msg)
{

  UDPPacketBase* pkt = check_and_cast<UDPPacketBase*>(msg);
  if (pkt->getSrcPort() == 0)
  {
    UDPPort tempPortID;
    do
      tempPortID = intuniform(MIN_RANDOM_PORT, MAX_PORT);
    while(isBound(tempPortID));

    pkt->setSrcPort(tempPortID);
  }
  if (isBound(pkt->getSrcPort()))
    {
        Dout(dc::warning|dc::udp|flush_cf, fullName()<<" UDP port "<<pkt->getSrcPort()<<" is already bound "
            <<" to application gate "<<boundPorts[pkt->getSrcPort()]<<" requested by "
          << msg->arrivalGate()->id());
        return false;
    }
  assert(findGate("to_app", msg->arrivalGate()->index()) != -1);
  boundPorts[pkt->getSrcPort()] = findGate("to_app", msg->arrivalGate()->index());
  Dout(dc::udp, fullName()<<" UDP port "<<pkt->getSrcPort()<<" bound to application gate "
       <<boundPorts[pkt->getSrcPort()]);
  return true;
}

void UDPProcessing6::processNetworkMsg(cMessage *msg)
{
  UDPPacketBase *pkt = check_and_cast<UDPPacketBase*>(msg);

  IPv6ControlInfo *ctrl = check_and_cast<IPv6ControlInfo *>(pkt->removeControlInfo());

  UDPAppInterfacePacket* appIntPkt = new UDPAppInterfacePacket;
  appIntPkt->setKind(KIND_DATA);
  appIntPkt->setSrcIPAddr(mkIpv6_addr(ctrl->srcAddr()));
  appIntPkt->setDestIPAddr(mkIpv6_addr(ctrl->destAddr()));
  appIntPkt->encapsulate(pkt);
  delete ctrl;

  Dout(dc::udp|continued_cf, fullPath()<<" Received data from "
       << ctrl->srcAddr() << ", port " << dec <<  pkt->getSrcPort());

  Dout(dc::finish," to " << ctrl->destAddr() << ", port "
       << dec <<  pkt->getDestPort()  );

  if(appIntPkt->getSrcIPAddr() == IPv6_ADDR_UNSPECIFIED)
    appIntPkt->setSrcIPAddr(mkIpv6_addr(ctrl->srcAddr()));

  UDPApplicationPorts::iterator it;
  it = boundPorts.find(pkt->getDestPort());

  if(it == boundPorts.end())
  {
    Dout(dc::udp|dc::warning, FILE_LINE<<" "<<fullPath()
         << " UDP Layer: Destination Process belonging to port "
         << pkt->getDestPort()<<" not found");
    cerr << "UDP Layer: Error -  Destination Port incorrect! "
         << pkt->getDestPort()<<" not found"<< endl;
    return;
  }

  ctrUdpInDgrams++;

  send(appIntPkt, it->second);
}

bool UDPProcessing6::isBound(UDPPort p)
{
  return boundPorts.count(p) == 1;
}

void UDPProcessing6::processApplicationMsg(UDPAppInterfacePacket* appIntPkt)
{
  UDPPacketBase* udpPkt =
    check_and_cast<UDPPacketBase*> (appIntPkt->decapsulate());

  // destination port not assigned
  if(udpPkt->getDestPort() == 0)
  {
    Dout(dc::udp|dc::core, FILE_LINE<<" "<<fullPath()
         << " UDP Layer: destination port not assigned for packet! ");
    return;
  }

  if (udpPkt->getSrcPort() == 0)
  {
    DoutFatal(dc::udp|dc::core, fullPath()<<" application src port not specified");
  }

  //Add UDP header length
  udpPkt->setLength(udpPkt->length() + UDP_HEADER_SIZE);

  IPv6ControlInfo *ctrl = new IPv6ControlInfo;
  ctrl->setSrcAddr(mkIPv6Address_(appIntPkt->getSrcIPAddr()));
  ctrl->setDestAddr(mkIPv6Address_(appIntPkt->getDestIPAddr()));
  ctrl->setProtocol(IP_PROT_UDP);
  udpPkt->setControlInfo(ctrl);

  ctrUdpOutDgrams++;
  Dout(dc::udp, fullPath()<<" sending UDP pkt src="<<mkIPv6Address_(appIntPkt->getSrcIPAddr())<<":"
       <<udpPkt->getSrcPort()<<" dest="<<mkIPv6Address_(appIntPkt->getDestIPAddr())<< ":"
       << udpPkt->getDestPort());

  send(udpPkt, "to_ip");
}

#if defined USE_CPPUNIT
#include <cppunit/extensions/HelperMacros.h>


/**
   @class UDPProcessing6Test
   @brief Unit test for    UDPProcessing6
   @ingroup TestCases
*/

class UDPProcessing6Test: public CppUnit::TestFixture
{

  CPPUNIT_TEST_SUITE( UDPProcessing6Test );
  CPPUNIT_TEST( testExample1 );
  CPPUNIT_TEST( testExample2 );
  CPPUNIT_TEST_SUITE_END();

 public:

  // Constructor/destructor.
  UDPProcessing6Test();
  virtual ~UDPProcessing6Test();

  void testExample1();
  void testExample2();

  void setUp();
  void tearDown();

private:

  // Unused ctor and assignment op.
  UDPProcessing6Test(const UDPProcessing6Test&);
  UDPProcessing6Test& operator=(const UDPProcessing6Test&);
};

CPPUNIT_TEST_SUITE_REGISTRATION( UDPProcessing6Test );

UDPProcessing6Test::UDPProcessing6Test()
{
}

UDPProcessing6Test::~UDPProcessing6Test()
{
}

void UDPProcessing6Test::setUp()
{
}

void UDPProcessing6Test::tearDown()
{
}

void UDPProcessing6Test::testExample1()
{
  CPPUNIT_ASSERT(1==1);
}

void UDPProcessing6Test::testExample2()
{
  CPPUNIT_ASSERT((2+3)==5);
}

#endif //defined USE_CPPUNIT
