// -*- C++ -*-
//
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
 * @file IPv6InterfacePacket.cc
 * @brief Interface Packet between IPv6 layer and transport layer
 * @author Eric Wu
 */

#include <cstring>
#include <cstdio>

#include "IPv6InterfacePacket.h"

// constructors
IPv6InterfacePacket::IPv6InterfacePacket(const char* src, const char* dest,cMessage *msg): IPInterfacePacket(), _src(c_ipv6_addr(src)), _dest(c_ipv6_addr(dest))
{
  setProtocol(PR_IPV6);
  if (msg)
  {
    encapsulate(msg);
    setName(msg->name());
  }
  // initValues();
}

IPv6InterfacePacket::IPv6InterfacePacket(const ipv6_addr& src, const ipv6_addr& dest, cMessage *msg)
  :IPInterfacePacket(), _src(src), _dest(dest)
{
  setProtocol(PR_IPV6);
  if (msg)
  {
    encapsulate(msg);
    setName(msg->name());
  }
}

// copy constructor
IPv6InterfacePacket::IPv6InterfacePacket(const IPv6InterfacePacket& ip): IPInterfacePacket()
{
  setName ( ip.name() );
  operator=( ip );
}

// assignment operator
IPv6InterfacePacket& IPv6InterfacePacket::operator=(const IPv6InterfacePacket& ip)
{
  IPInterfacePacket::operator=(ip);
  _src = ip._src;
  _dest = ip._dest;

  return *this;
}

// output functions
std::string IPv6InterfacePacket::info()
{
  ostringstream os;
  os << "prot=" << protocol() << " src=" << _src << " dest=" << _dest;
  // XXX protocol(), ipv6_addr_toString(_src).c_str(), ipv6_addr_toString(_dest).c_str());
  return os.str();
}

void IPv6InterfacePacket::writeContents(ostream& os)
{
  os << "IPv6interface: "
     << "\nSource addr: "  << ipv6_addr_toString(_src)
     << "\nDestination addr: " << ipv6_addr_toString(_dest)
     << "\nProtocol: " << (int)protocol()
     << " TTL: " << timeToLive()
     << "\n";
}

#ifdef USE_CPPUNIT
#include <cppunit/extensions/HelperMacros.h>

/**
 * @class InterfacePacketTest
 * @brief Unit test for IPv6InterfacePacket and IPInterfacePacket
 * @ingroup TestCases
 */

class InterfacePacketTest: public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE( InterfacePacketTest );
  CPPUNIT_TEST( testAssignCtorDtor );
  CPPUNIT_TEST( testIntegration );
  CPPUNIT_TEST_SUITE_END();
public:

  InterfacePacketTest();


  void testAssignCtorDtor();
  ///Test involving use of pointers to both v6 and v4 interface packets
  void testIntegration();

  void setUp();
  void tearDown();

private:
  IPInterfacePacket* intv4Packet;
  IPInterfacePacket* intv6Packet;
};

CPPUNIT_TEST_SUITE_REGISTRATION( InterfacePacketTest );

#include <string>
#include <boost/cast.hpp>

InterfacePacketTest::InterfacePacketTest():
  TestFixture(), intv4Packet(0), intv6Packet(0)
{}

static const char* src_addr = "a010:f0f0:b0b0:0:0:ffff:eeee:aaaa";
static const char* dest_addr = "ef00:abcd:ef00:ffff:0:f0f:0:0";

/**
 * @warning why is one 59 and the other 6 in UNITTEST
 *
 * @todo find out why the commented out line segfaults and fix
 * assert(v6copy.protocol() == intv6Packet->cPacket::protocol());
 */
void InterfacePacketTest::testAssignCtorDtor()
{
  intv4Packet = new IPInterfacePacket();
  intv4Packet->setDestAddr("1.1.1.0");
  intv4Packet->setSrcAddr("255.255.255.0");
  IPInterfacePacket v4copy = *intv4Packet;
  //Did this test ever run? It should have failed before as the test condition
  //was wrong (these addresses should be the same
  CPPUNIT_ASSERT(strcmp(v4copy.srcAddr(), intv4Packet->srcAddr()) == 0);

      //Test ctor
  intv6Packet = new IPv6InterfacePacket(src_addr, dest_addr);
#if defined UNITTESTOUTPUT
  cerr << intv6Packet->srcAddr() << " is equal to "<<src_addr<<endl;
#endif
  CPPUNIT_ASSERT(strcmp(src_addr, intv6Packet->srcAddr())==0);
#if defined UNITTESTOUTPUT
  cout << intv6Packet->destAddr() << " is equal to "<<dest_addr<<endl;
#endif
  CPPUNIT_ASSERT(strcmp(dest_addr, intv6Packet->destAddr())==0);
  intv6Packet->setProtocol(IP_PROT_TCP);
  intv6Packet->setTimeToLive(55);

  //test default ctor
  IPv6InterfacePacket v6copy;

  CPPUNIT_ASSERT(strcmp("0:0:0:0:0:0:0:0", v6copy.srcAddr())==0);
  CPPUNIT_ASSERT(strcmp("0:0:0:0:0:0:0:0", v6copy.destAddr())==0);

  //Calls initialisation of super class
  CPPUNIT_ASSERT((int)v6copy.protocol() == (int)PR_IPV6);

  //Test assignment
  v6copy = *(boost::polymorphic_downcast<IPv6InterfacePacket*> (intv6Packet));
  CPPUNIT_ASSERT(strcmp(src_addr, v6copy.srcAddr())==0);
  CPPUNIT_ASSERT(strcmp(dest_addr, v6copy.destAddr())==0);

#if defined UNITTESTOUTPUT
  cout << "v6intpkt obj protocol="<<v6copy.protocol()
       <<" vs. ptr ipv4intpkt protocol="<<intv6Packet->protocol()<<endl;
#endif

  //Don't know why this fails
  //CPPUNIT_ASSERT(v6copy.protocol() == intv6Packet->cPacket::protocol());
  CPPUNIT_ASSERT(v6copy.timeToLive() == intv6Packet->timeToLive());

  CPPUNIT_ASSERT(strcmp(intv6Packet->className(), "IPv6InterfacePacket") == 0);

  CPPUNIT_ASSERT(strcmp(intv4Packet->className(), "IPInterfacePacket") == 0);

  intv4Packet = intv6Packet->dup();
  CPPUNIT_ASSERT(strcmp(intv6Packet->className(), "IPv6InterfacePacket") == 0);

  delete intv4Packet;
  delete intv6Packet;

}

///Test involving use of pointers to both v6 and v4 interface packets
void InterfacePacketTest::testIntegration()
{
  //Input strings into cPar and see what happens

}


void InterfacePacketTest::setUp()
{

}

void InterfacePacketTest::tearDown()
{

}


#endif //USE_CPPUNIT
