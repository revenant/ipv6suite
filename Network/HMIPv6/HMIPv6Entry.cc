//
// Copyright (C) 2002, 2004 CTIE, Monash University
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
 * @file   HMIPv6Entry.cc
 * @author Johnny Lai
 * @date   08 Sep 2002
 *
 * @brief  Implementation of HMIPv6MAPEntry
 *
 *
 */


#include "HMIPv6Entry.h"
#include "HMIPv6ICMPv6NDMessage.h"

#include <iostream>
#include <algorithm>

namespace HierarchicalMIPv6
{
  std::ostream& operator<<(std::ostream& os, const HMIPv6MAPEntry& me)
  {
    return os<<me.addr()<<" pref="<<me.preference()<<" dist="
	     <<me.distance()<<" R="<<me.r()<<" lifetime="<<me.lifetime()
             <<" option="<<me.options<<endl;

  }

  HMIPv6MAPEntry::HMIPv6MAPEntry(const HMIPv6ICMPv6NDOptMAP& src)
    :options(0)
  {
    setDistance(src.dist());
    setPreference(src.pref());

    setR(src.r());
    setLifetime(src.lifetime());
    setAddr(src.addr());
  }

} //namespace HierarchicalMIPv6

#ifdef USE_CPPUNIT
#include <cppunit/extensions/HelperMacros.h>
#include <algorithm> //copy
#include <iterator> //ostream_iterator
#include <boost/shared_ptr.hpp>
#include <iostream>
#include <list>

using HierarchicalMIPv6::HMIPv6MAPEntry;

/**
 *  @class HMIPv6EntryTest
 *  @brief UnitTest for HMIPv6Entry
 *  @ingroup TestCases
 */

class HMIPv6EntryTest: public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE( HMIPv6EntryTest );
  CPPUNIT_TEST( test );
  CPPUNIT_TEST_SUITE_END();
public:

  HMIPv6EntryTest();

  void test();

  void setUp();
  void tearDown();

private:
  HMIPv6MAPEntry *mapA, *mapB;
  ipv6_addr mapAddr;
};

CPPUNIT_TEST_SUITE_REGISTRATION( HMIPv6EntryTest );

HMIPv6EntryTest::HMIPv6EntryTest()
  :mapAddr(c_ipv6_addr("1:2:3:4:5:6:7:8"))
{}

template<typename T> std::ostream& operator<<(std::ostream& os,
                                              boost::shared_ptr<T> const & a)
{
  //return std::less<T*>()(a.get(), b.get());
  a?os<<*a:os<<"missing";
  return os;
}

inline bool lessThanMAP(boost::shared_ptr<HMIPv6MAPEntry> const& a, boost::shared_ptr<HMIPv6MAPEntry> const& b)
{
  //Default boost is pointer comparison which is not what we want
  //std::less<T*>()(a.get(), b.get());
  return *a < *b;
}

void HMIPv6EntryTest::test()
{
  mapA->setDistance(5);
  mapB->setDistance(10);

  CPPUNIT_ASSERT(*mapB < *mapA);

  mapA->setDistance(10);

  CPPUNIT_ASSERT(!(*mapA<*mapB) && !(*mapB<*mapA));

  mapB->setLifetime(10);

  mapB->setAddr(mapAddr);

  stringstream a, b;
  a<<*mapA;
  b<<*mapB;
  CPPUNIT_ASSERT(a.str() == b.str());

  CPPUNIT_ASSERT(*mapB == *mapA);

  mapA->setR(true);

  CPPUNIT_ASSERT(*mapB != *mapA);

  mapA->setR(false);
  CPPUNIT_ASSERT(*mapB == *mapA);

  mapB->setM(true);
  mapB->setI(true);

  HMIPv6MAPEntry* mapC = new HMIPv6MAPEntry(*mapB);
  CPPUNIT_ASSERT(*mapC == *mapB);

  CPPUNIT_ASSERT(*mapC != *mapA);

  delete mapC;

  HMIPv6MAPEntry* mapD = new HMIPv6MAPEntry(*mapB);
  CPPUNIT_ASSERT(*mapD == *mapB);

  CPPUNIT_ASSERT(!(*mapA<*mapD) && !(*mapD<*mapA));

  list<boost::shared_ptr<HMIPv6MAPEntry> > sortMaps;
  sortMaps.push_back(boost::shared_ptr<HMIPv6MAPEntry>(mapA));
  sortMaps.push_back(boost::shared_ptr<HMIPv6MAPEntry>(mapB));
  sortMaps.push_back(boost::shared_ptr<HMIPv6MAPEntry>(mapD));

  std::copy(sortMaps.begin(), sortMaps.end(), ostream_iterator<boost::shared_ptr<HMIPv6MAPEntry> >(cout, "\n"));


  //preference does not affect sort now

  mapB->setPreference(8);
  mapB->setDistance(1);
  CPPUNIT_ASSERT(*mapB>*mapD);
  sortMaps.sort(lessThanMAP);

  mapD->setDistance(1);
  //equivalence
  CPPUNIT_ASSERT(!(*mapD<*mapB) && !(*mapB<*mapD));

  std::vector<HMIPv6MAPEntry> maps;
//  std::copy(sortMaps.begin(), sortMaps.end(), std::back_insert_iterator<std::vector<boost::shared_ptr<HMIPv6MAPEntry> > > (maps));

  for (list<boost::shared_ptr<HMIPv6MAPEntry> >::iterator it = sortMaps.begin(); it != sortMaps.end(); it++)
  {

  }

}

void HMIPv6EntryTest::setUp()
{
  mapA = new HMIPv6MAPEntry(mapAddr, 10);
  mapB = new HMIPv6MAPEntry();

}

void HMIPv6EntryTest::tearDown()
{
  //delete mapA;
  //delete mapB;
}

#endif //USE_CPPUNIT

