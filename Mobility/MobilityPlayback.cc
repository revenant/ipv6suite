// -*- C++ -*-
// Copyright (C) 2006 
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
 * @file   MobilityPlayback.cc
 * @author 
 * @date   05 Jun 2006
 *
 * @brief  Implementation of MobilityPlayback
 *
 *
 */

//Headers for libcwd debug streams have to be first (remove if not used)
#include "sys.h"
#include "debug.h"

#include "MobilityPlayback.h"
#include "MobileEntity.h"
#include "WorldProcessor.h"

#include <boost/cast.hpp>

Define_Module_Like(MobilityPlayback, MobilityHandler);

std::ifstream MobilityPlayback::f;
std::map<MobileEntity*, std::list<MobilityPlayback::Move> > MobilityPlayback::moves;


void MobilityPlayback::initialize(int stage)
{
  MobilityHandler::initialize(stage);
  if (stage != 2)
    return;
  
  if (moves.empty())
    {
  f.open("walk.txt");
  //parse individual moves
  double x=0,y=0;
  std::string node;
  char space;
  simtime_t time;
  static std::ofstream of("walkout.txt");
  std::map<std::string, MobileEntity*> names;
  while (f)
  {
    f>>node>>space>>time>>space>>x>>space>>y;
    of<<node<<" "<<time<<" "<<x<<" "<<y;
    if (!names.count(node))
    {
      MobileEntity* me = boost::polymorphic_downcast<MobileEntity*> (wproc->findEntityByNodeName(node));
      if (!me)
	continue;
      names[node] = me;
    }
    Move m = {time, x, y};
    moves[names[node] ].push_back(m);
  }
    }
  selfMovingNotifier = new cMessage("PlaybackMove", TMR_WIRELESSMOVE);

  Move m = moves[mobileEntity].front();
  selfMovingNotifier->addPar("x") = m.x;
  selfMovingNotifier->addPar("y") = m.y;
  scheduleAt(m.time, selfMovingNotifier);
}

void MobilityPlayback::finish()
{
}

void MobilityPlayback::handleMessage(cMessage* msg)
{
  //search for entity matching name or actually
  moves[mobileEntity].pop_front();
  Move& m = moves[mobileEntity].front();
  selfMovingNotifier->par("x") = m.x;
  selfMovingNotifier->par("y") = m.y;
  scheduleAt(simTime() + m.time, selfMovingNotifier);
}

#if defined USE_CPPUNIT
#include <cppunit/extensions/HelperMacros.h>


/**
   @class MobilityPlaybackTest
   @brief Unit test for	MobilityPlayback
   @ingroup TestCases
*/

class MobilityPlaybackTest: public CppUnit::TestFixture
{

  CPPUNIT_TEST_SUITE( MobilityPlaybackTest );
  CPPUNIT_TEST( testExample1 );
  CPPUNIT_TEST( testExample2 );
  CPPUNIT_TEST_SUITE_END();

 public:

  // Constructor/destructor.
  MobilityPlaybackTest();
  virtual ~MobilityPlaybackTest();

  void testExample1();
  void testExample2();

  void setUp();
  void tearDown();

private:

  // Unused ctor and assignment op.
  MobilityPlaybackTest(const MobilityPlaybackTest&);
  MobilityPlaybackTest& operator=(const MobilityPlaybackTest&);
};

CPPUNIT_TEST_SUITE_REGISTRATION( MobilityPlaybackTest );

MobilityPlaybackTest::MobilityPlaybackTest()
{
}

MobilityPlaybackTest::~MobilityPlaybackTest()
{
}

void MobilityPlaybackTest::setUp()
{
}

void MobilityPlaybackTest::tearDown()
{
}

void MobilityPlaybackTest::testExample1()
{
  CPPUNIT_ASSERT(1==1);
}

void MobilityPlaybackTest::testExample2()
{
  CPPUNIT_ASSERT((2+3)==5);
}

#endif //defined USE_CPPUNIT

