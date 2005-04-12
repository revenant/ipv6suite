// -*- C++ -*-
// Copyright (C) 2002, 2005 CTIE, Monash University
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
 * @file IDestinationCache.h
 * @author Johnny Lai
 * @date 18 Apr 2002
 * @brief Conceptual Data Structures for IPv6 Neighbour Discovery
 * @test
 * @todo
 */

#ifndef IDESTINATIONCACHE_H
#define IDESTINATIONCACHE_H 1

#ifndef MAP
#define MAP
#include <map>
#endif //MAP

#ifndef BOOST_UTILITY_HPP
#include <boost/utility.hpp>
#endif //BOOST_UTILITY_HPP
#ifndef BOOST_WEAK_PTR_HPP
#include <boost/weak_ptr.hpp>
#endif //BOOST_WEAK_PTR_HPP
#ifndef NDENTRY_H
#include "NDEntry.h"
#endif //NDENTRY_H

namespace MobileIPv6
{
  class MIPv6MobilityState;
  class MIPv6NDStateHost;
}

namespace IPv6NeighbourDiscovery
{

/**
 * @class IDestinationCache
 * @brief Interface for Conceptual Data Structure DestinationCache
 *
 * detailed description
 */

class IDestinationCache: public boost::noncopyable
{
  friend class XMLConfiguration::IPv6XMLParser;
  friend class IPv6Forward;

#ifdef USE_MOBILITY
  friend class MobileIPv6::MIPv6MobilityState;
  friend class MobileIPv6::MIPv6NDStateHost;
#endif // USE_MOBILITY

public:

  ///Returns the next hop neighbour entry for destination of addr or return null
  ///if addr not in DC
  boost::weak_ptr<NeighbourEntry> neighbour(const ipv6_addr& addr);

  ///Does longest Prefix match for router - router comms
  boost::weak_ptr<NeighbourEntry> lookupDestination(const ipv6_addr& addr);

  /// returns the destination entry with IP address given
  DestinationEntry& operator[](const ipv6_addr& addr);
  /// Should use IPv6Address if you want to create a dest entry that preserves
  /// prefix length. (important for core routers to route traffic to different
  /// subnets by various pathways)
  DestinationEntry& operator[] (const IPv6Address& addr);

  /**
   * Used by MIPv6 to remove old routes manually since we still need to keep the
   * Router object
   *
   * @param addr is of neighbour entry
   * @return number of destination entries removed
   */

  int removeDestEntryByNeighbour(const ipv6_addr& addr);

  /**
   * @brief Update existing destination entries with neighbour of addr by
   * breaking that link and using ngbr as the next hop neighbour instead.
   *
   * @param addr address of the next hop neighbour search key
   * @param ngbr address of the new next hop neighbour to use intead of addr
   * @return number of destinatioin entires updated
   */

  int updateDestEntryByNeighbour(const ipv6_addr& addr, const ipv6_addr& ngbr);


  typedef std::map<IPv6Address, DestinationEntry > DestinationCache;

#ifndef CXX
protected:
#endif //ifndef CXX

  typedef DestinationCache::iterator DCI;

  ///Return by reference an iterator to the element if found.  This is the only
  ///method of looking up the Dest Cache without creating a Dest Entry in the
  ///process.
  bool findDestEntry(const ipv6_addr& addr, DCI& it)
  {
    it = destCache.find(IPv6Address(addr));
    return it!=destCache.end();
  }

#ifdef CXX
protected:
#endif //CXX
  DCI beginDC() { return destCache.begin(); }

  ///Returns the DestinationEntry at it and moves it forward by one.
  ///Returns 0 when reaching the end.
  ///Make sure iterator obtained originally with beginDC.
  DestinationEntry* nextDC(DCI& it);

  ///Remove the Destination Entry for addr
  bool removeDestEntry(const ipv6_addr& addr)
    {
      DCI it;
      if (findDestEntry(addr, it))
      {
        destCache.erase(it);
        return true;
      }

      return false;
    }

protected:
  /**
     @var destCache
     Keyed on ip_addr
  */
  DestinationCache destCache;

};

std::ostream& operator<<
  (std::ostream& os, const pair<IPv6Address, DestinationEntry> & p);

} //namespace IPv6NeighbourDiscovery


#endif /* IDESTINATIONCACHE_H */

