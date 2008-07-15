//
// Copyright (C) 2003 Johnny Lai
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
 * @file   RoutingProtocolRIP.cc
 * @author Johnny Lai
 * @date   25 Jul 2003
 *
 * @brief  Implementation of RoutingProtocolRIP
 *
 * Implements split horizon with poison reverse with configuration in xml to
 * disable poisoned reverse.
 * Triggerered updates for changed route metrics at limited rate acc. to 3.10.1
 *
 * Format of packet is
 */


#include "RoutingProtocolRIP.h"

using namespace RoutingProtocol;

// Define_Module(RoutingProtocolRIP);

Define_Module_Like(RoutingProtocolRIP, ParentStringType);

int RoutingProtocolRIP::numInitStages() const
{
  return 2;
}

void RoutingProtocolRIP::initialize(int stageNo)
{
}

void RoutingProtocolRIP::finish()
{
}

void RoutingProtocolRIP::handleMessage(cMessage* msg)
{
}


#if defined USE_CPPUNIT
#include <cppunit/extensions/HelperMacros.h>


/**
   @class RoutingProtocolRIPTest
   @brief
   @ingroup TestCases
*/

class RoutingProtocolRIPTest: public CppUnit::TestFixture
{

  CPPUNIT_TEST_SUITE( RoutingProtocolRIPTest );
  CPPUNIT_TEST( testExample1 );
  CPPUNIT_TEST( testExample2 );
  CPPUNIT_TEST_SUITE_END();

 public:

  // Constructor/destructor.
  RoutingProtocolRIPTest();
  virtual ~RoutingProtocolRIPTest();

  void testExample1();
  void testExample2();

  void setUp();
  void tearDown();

private:

  // Unused ctor and assignment op.
  RoutingProtocolRIPTest(const RoutingProtocolRIPTest&);
  RoutingProtocolRIPTest& operator=(const RoutingProtocolRIPTest&);
};

CPPUNIT_TEST_SUITE_REGISTRATION( RoutingProtocolRIPTest );

RoutingProtocolRIPTest::RoutingProtocolRIPTest()
{
}

RoutingProtocolRIPTest::~RoutingProtocolRIPTest()
{
}

void RoutingProtocolRIPTest::setUp()
{
}

void RoutingProtocolRIPTest::tearDown()
{
}

void RoutingProtocolRIPTest::testExample1()
{
  CPPUNIT_ASSERT(1==1);
}

void RoutingProtocolRIPTest::testExample2()
{
  CPPUNIT_ASSERT((2+3)==5);
}

#endif //defined USE_CPPUNIT

