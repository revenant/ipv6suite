// -*- C++ -*-
//
// Copyright (C) 2002 CTIE, Monash University
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


/**
 * @file IPv6CDS.h
 * @author Johnny Lai
 * @date 17 Apr 2002
 * @brief Conceptual Data Structures for IPv6 Neighbour Discovery
 * Originally from the RoutingTable6 class.
 * @test see IPv6CDSTest
 */

#ifndef IPV6CDS_H
#define IPV6CDS_H 1

#ifndef IROUTERLIST_H
#include "IRouterList.h"
#endif //IROUTERLIST_H

#ifndef IPREFIXLIST_H
#include "IPrefixList.h"
#endif //IPREFIXLIST_H

#ifndef INEIGHBOURCACHE_H
#include "INeighbourCache.h"
#endif //INEIGHBOURCACHE_H

#ifndef IDESTINATIONCACHE_H
#include "IDestinationCache.h"
#endif //IDESTINATIONCACHE_H


#ifdef USE_CPPUNIT
class IPv6CDSTest;
#endif //USE_CPPUNIT

namespace IPv6NeighbourDiscovery
{

/**
 * @class IPv6CDS
 * @brief Container for IPv6 Conceptual Data structures
 *
 * Split from RoutingTable6
 */


class IPv6CDS: public IRouterList, public IPrefixList, public INeighbourCache,
               public IDestinationCache
{

#ifdef USE_CPPUNIT
  friend class ::IPv6CDSTest;
#endif //USE_CPPUNIT

public:

  /// Insert a newly created entry into NC and its associated Destination entry
  void insertNeighbourEntry(NeighbourEntry* entry);

  /// Remove Neighbour entry and its associated Destination entry
  void removeNeighbourEntry(const ipv6_addr& addr);

  /// Remove Destination entry and the Neighbour Entry if destination is a neighbour
  void removeDestinationEntry(const ipv6_addr& addr);

  ///Associate the prefix with interface link on which it belongs
  PrefixEntry* insertPrefixEntry(const PrefixEntry& pe, size_t ifIndex);

  /// insert router entry into the list
  void insertRouterEntry(RouterEntry* re, bool setDefault);

  ///Return a router Object or 0 if not found
  boost::weak_ptr<RouterEntry> router(const ipv6_addr& addr);

  simtime_t latestRAReceived(void);

public:

  //@name constructors, destructors and operators
  //@{
  IPv6CDS();

  virtual ~IPv6CDS();

//   IPv6CDS(const IPv6CDS& src);

//   IPv6CDS& operator=(IPv6CDS& src);

//   bool operator==(const IPv6CDS& rhs);
  //@}

private:

};

} //namespace IPv6NeighbourDiscovery


#endif /* IPV6CDS_H */
