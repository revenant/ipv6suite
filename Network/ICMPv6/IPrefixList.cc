//
// Copyright (C) 2002 CTIE, Monash University
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
 * @file   IPrefixList.cc
 * @author Johnny Lai
 * @date   18 Apr 2002
 *
 * @brief Implementation of IPrefixList
 *
 */

#include "sys.h"
#include "debug.h"

#include "IPrefixList.h"
#include "stlwatch.h"

namespace IPv6NeighbourDiscovery
{


std::ostream& operator<<(std::ostream& os, const std::pair<PrefixEntry,size_t>& pe) {
  return os << pe.first << ", " << pe.second;
}

IPrefixList::IPrefixList()
{
  WATCH_MAP(prefixList);
}

IPrefixList::~IPrefixList()
{}

/// Returns a vector of pointers to PrefixEntries for a particular iface
LinkPrefixes IPrefixList::getPrefixesByIndex(size_t ifIndex)
{
  LinkPrefixes linkPrefixes;

  PLI it;
  for(it = prefixList.begin(); it != prefixList.end(); it++)
    if(it->second.second == ifIndex)
      linkPrefixes.push_back(&(it->second.first));

  return linkPrefixes;

}

/**
 * @brief on-link prefix check
 *
 * Employs a longest prefix match i.e. the prefix with the longests lengths are
 * first compared to see if the dest matches.
 *
 * @arg dest is the address to check
 * @arg ifindex returns the interface on which the address is on-link
 * @return true if destination is on-link
 */


bool IPrefixList::lookupAddress(const ipv6_addr& dest, unsigned int& ifIndex)
{
  for(PLI it = prefixList.begin(); it != prefixList.end(); it++)
    if (it->first.isNetwork(dest))  //matches like lookup
    {
      ifIndex = it->second.second;
      Dout(dc::debug|dc::forwarding, "Longest Onlink Prefix Match for "<< dest<<" returned prefix="
           <<it->first<<" and outgoing ifIndex="<<ifIndex);
      return true;
    }

  return false;
}

void IPrefixList::insertPrefixEntry(const PrefixEntry& pe, size_t ifIndex)
{
  prefixList[pe.prefix()] = make_pair(pe, ifIndex);
}

} //namespace IPv6NeighbourDiscovery

#ifdef USE_CPPUNIT
#include <cppunit/extensions/HelperMacros.h>

using namespace IPv6NeighbourDiscovery;

/**
   @class PrefixListTest
   @brief Unit test for PrefixList
   @ingroup TestCases
*/

class PrefixListTest: public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE( PrefixListTest );
  CPPUNIT_TEST( testPrefixListOrder );
  CPPUNIT_TEST_SUITE_END();
public:

  PrefixListTest()
    {}

  void testPrefixListOrder();
  void setUp();
  void tearDown();

private:

  // Unused copy ctor and assignment op.
  PrefixListTest(const PrefixListTest&);
  PrefixListTest& operator=(const PrefixListTest&);
  IPrefixList* pl;

};

CPPUNIT_TEST_SUITE_REGISTRATION( PrefixListTest );

void PrefixListTest::testPrefixListOrder()
{
  PrefixEntry p1;
  p1.setPrefix("3271:2222:3312:4444:6433:0:0:0/40");
  pl->insertPrefixEntry(p1, 1);

  PrefixEntry p2;
  p2.setPrefix("3271:2222:3312:4444:6433:0:0:0/50");
  pl->insertPrefixEntry(p2, 2);

  PrefixEntry p3;
  p3.setPrefix("3271:2222:3312:4444:6433:3343:0:0/96");
  pl->insertPrefixEntry(p3, 3);

  PrefixEntry p4;
  p4.setPrefix("3271:1111:3312:4444:6433:3343:0:0/96");
  pl->insertPrefixEntry(p4, 4);

  IPrefixList::PLI it;
  for (it = pl->prefixList.begin(); it != pl->prefixList.end(); it++)
    cout<<it->first<<"/"<<dec<<it->first.prefixLength()<<" -- "<<endl;
}

void PrefixListTest::setUp()
{
  pl = new IPrefixList;
}

void PrefixListTest::tearDown()
{
  delete pl;
}

#endif //USE_CPPUNIT

