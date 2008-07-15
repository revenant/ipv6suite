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
 * @file   IPv6CDS.cc
 * @author Johnny Lai
 * @date   18 Apr 2002
 *
 * @brief Implementation of IPv6CDS
 *
 */


#include "IPv6CDS.h"


namespace
{
  const size_t ROUTER_LOOKUP_VIA_DC = 100;
}


namespace IPv6NeighbourDiscovery
{


IPv6CDS::IPv6CDS()
{}


IPv6CDS::~IPv6CDS()
{}


PrefixEntry* IPv6CDS::insertPrefixEntry(const PrefixEntry& pe, size_t ifIndex)
{
  IPrefixList::insertPrefixEntry(pe, ifIndex);
  return getPrefixEntry(pe.prefix());
}

/**
 * @param re must be a dynamically created object
 * @param setDefault Is re to become the default router
 * We do not create another separate NE object and place that in NC.  Instead a
 * reference to re is made in DC to signify reachability.  Thus if no dest entry
 * exists for a router you can assume its unreachable until further NUD or addr
 * res finds its back up again and reinserts it into DC.
 *
 */

void IPv6CDS::insertRouterEntry(RouterEntry* re, bool setDefault)
{

  DestinationEntry de(IRouterList::insertRouterEntry(re, setDefault));
  //Delete any old neighbour entry
  removeNeighbourEntry(re->addr());
  (*this)[re->addr()] = de;

  //Insert into DC only when it has a LL addr (suppose to be stale when created
  //for routers)
  //if (re->linkLayerAddr() != "")

  //25.01.02  Johnny Lai
  //However the conceptual sending algorithm will do address resolution if no
  //link layer address exists (This reduces the number of lookup lists to 1
  //namely the DC instead of looking up also an NC)).  If link layer address
  //unknown make sure you call setState(INCOMPLETE) to indicate address
  //Resolution required
}


void IPv6CDS::insertNeighbourEntry(NeighbourEntry* entry)
{
  destCache[IPv6Address(entry->addr())].neighbour
    = INeighbourCache::insertNeighbourEntry(entry);
}


void IPv6CDS::removeNeighbourEntry(const ipv6_addr& addr)
{
  if (removeDestEntry(addr))
    INeighbourCache::removeNeighbourEntry(addr);
}

/**
 * This is required to reinitiate next hop determination.  Routers are not in
 * the neighbour cache so by deleting the DestEntry corresponding to the router
 * that signifies that router as been unreachable and thus Default Router
 * Selection can work.
 *
 * @note the Neighbour Cache entry for neighbouring hosts (Routers are not in
 * NC) are also deleted if the de->addr = de->neigbhour->addr(). see Sec. 7.3.3
 * of RFC 2461
 */

void IPv6CDS::removeDestinationEntry(const ipv6_addr& addr)
{
  if (removeDestEntry(addr))
    findNeighbourAndRemoveEntry(addr);
}

boost::weak_ptr<RouterEntry> IPv6CDS::router(const ipv6_addr& addr)
{
//  if (routerCount() < ROUTER_LOOKUP_VIA_DC)
    return IRouterList::router(addr);

///Note the code below is no longer correct because addr Res will delete routers
///that have failed Addr Resolution from the NC

//   else
//   {
//     boost::weak_ptr<NeighbourEntry> ne;

//     if ((ne = neighbour(addr)).get() != 0)
//     {
//       assert(ne->addr() == addr);
//       if (ne->isRouter())
//         return boost::shared_static_cast<RouterEntry> (ne);
/*
      //NeighbourEntry needs a virtual function in order to test ne identity
      {
#if !defined TESTIPv6
        return static_cast<RouterEntry*>(ne);
#else
        assert(dynamic_cast<RouterEntry*>(ne) != 0);
        return static_cast<RouterEntry*>(ne);
#endif //TESTIPv6
      }
*/

//    }
//   }

//   return boost::weak_ptr<RouterEntry>();

}

simtime_t IPv6CDS::latestRAReceived(void)
{
  simtime_t latestRAReceived = 0;
  for (DRLI it = routers.begin(); it != routers.end(); it++)
  {
    if ( latestRAReceived < (*it)->lastRAReceived )
      latestRAReceived = (*it)->lastRAReceived;
  }
  return latestRAReceived;
}

} //namespace IPv6NeighbourDiscovery

#if defined USE_CPPUNIT
#include <cppunit/extensions/HelperMacros.h>

using namespace IPv6NeighbourDiscovery;

/**
   @class IPv6CDSTest
   @brief Unit Test for IPv6CDS
   @ingroup TestCases
*/

class IPv6CDSTest: public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE( IPv6CDSTest );
  CPPUNIT_TEST( testDC );
  CPPUNIT_TEST( testLongestPrefixMatch );
  CPPUNIT_TEST_SUITE_END();
public:

  void testDC();
  void testLongestPrefixMatch();

  void setUp();
  void tearDown();

private:

  IPv6CDS* cds;
};

CPPUNIT_TEST_SUITE_REGISTRATION( IPv6CDSTest );

#include "opp_utils.h"

/**
 * Test ordering of neighbour entries required for core router routing
 * function. Longest prefix lookups for nexthop route as prefixes are now stored
 * as a destination in DC.
 */

void IPv6CDSTest::testDC()
{

  IPv6Address* x1 = new IPv6Address("ffff:0:222:0:1234:F545:1:0");
  NeighbourEntry* ne1 = new NeighbourEntry(*x1);
  cds->insertNeighbourEntry(ne1);

  IPv6Address* x2 = new IPv6Address("fffd:0:222:0:0:F234:D35:DD2");
  NeighbourEntry* ne2 = new NeighbourEntry(*x2);
  cds->insertNeighbourEntry(ne2);

  cds->destCache[IPv6Address("fffd:0:222:0:0:F234:0:0/96")] =
    cds->neighbour(c_ipv6_addr("fffd:0:222:0:0:F234:D35:DD2"));

  boost::weak_ptr<NeighbourEntry> ne = cds->neighbour(c_ipv6_addr("fffd:0:222:0:0:F234:0:0/96"));

  //Intuitively this seems incorrect.
  assert(ne.lock().get() == 0);
  //But if you are searching for the next hop of a prefix that has been entered
  //via xml file then this is the only implementation since prefix list
  //is only for onlink prefixes.  Its a hack for core routers to function properly
  //Need a function that preserves prefix length (only available in RT 6)
  ne = cds->destCache[IPv6Address("fffd:0:222:0:0:F234:0:0/96")].neighbour;
  assert(ne.lock().get() != 0 );
  assert(ne.lock().get() == ne2);

  cout << "Iterating DC"<<endl;

  IPv6CDS::DCI it = cds->destCache.begin();
  while(it != cds->destCache.end())
  {
    cout<<it->first.address()<<" next hop is "<<it->second.neighbour.lock()->addr()<<endl;
    it++;
  }
  delete x1;
  delete x2;


  cds->removeNeighbourEntry(c_ipv6_addr("fffd:0:222:0:0:F234:D35:DD2"));
  //cds->removeDestEntryByNeighbour(c_ipv6_addr("fffd:0:222:0:0:F234:D35:DD2"));

  //Test boost weak_ptr going to zero when shared_ptr removed
  assert(ne.lock().get() == 0);

  //Test the pointers inside DC to see if they are set to zero too as neighbour
  //has just been revmoed.
  assert(cds->destCache[IPv6Address(c_ipv6_addr("fffd:0:222:0:0:F234:0:0/96"))].neighbour.lock().get() == 0);
  assert(!cds->findNeighbourAndRemoveEntry(c_ipv6_addr("fffd:0:222:0:0:F234:D35:DD2")));


}


/**
 * perform the longest prefix matching and
 *
 */

void IPv6CDSTest::testLongestPrefixMatch()
{
  PrefixEntry p1;
  p1.setPrefix("3271:2222:3312:4444:6433:0:0:0/40");
  cds->insertPrefixEntry(p1, 1);

  PrefixEntry p2;
  p2.setPrefix("3271:2222:3312:4444:6433:0:0:0/50");
  cds->insertPrefixEntry(p2, 2);

  PrefixEntry p3;
  p3.setPrefix("3271:2222:3312:4444:6433:3343:0:0/96");
  cds->insertPrefixEntry(p3, 3);

  PrefixEntry p4;
  p4.setPrefix("3271:1111:3312:4444:6433:3343:0:0/96");
  cds->insertPrefixEntry(p4, 4);

  size_t ifIndex = 0;

  assert(cds->lookupAddress(IPv6Address("3271:2222:3312:4444:6433:3343:1234:0"),
                       ifIndex));
  assert(ifIndex == 3);
}


void IPv6CDSTest::setUp()
  {
    cds = new IPv6CDS();
  }


void IPv6CDSTest::tearDown()
{
  delete cds;
  cds = 0;
}

#endif //defined USE_CPPUNIT
