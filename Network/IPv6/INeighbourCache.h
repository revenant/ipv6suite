// -*- C++ -*-
// $Header: /home/cvs/IPv6Suite/IPv6SuiteWithINET/Network/IPv6/Attic/INeighbourCache.h,v 1.1 2005/02/09 06:15:57 andras Exp $
// Copyright (C) 2002, 2003 CTIE, Monash University 
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
 * @file INeighbourCache.h
 * @author Johnny Lai
 * @date 18 Apr 2002
 * @brief Conceptual Data Structures for IPv6 Neighbour Discovery
 */

#ifndef INEIGHBOURCACHE_H
#define INEIGHBOURCACHE_H 1

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

namespace IPv6NeighbourDiscovery
{
  
/**
   @class INeighbourCache
   @brief Interface for Conceptual Data Structure NeighbourCache

   IDestinationCache holds destination entries. Each dest entry points to a
   neighbour entry in here. More than one dest entry can refer to same
   neighbour. The neighbour entry itself has its own dest entry.
*/

class INeighbourCache//: public IDestinationCache
{
public:

  // Constructor/destructor.
  //~INeighbourCache();

protected:

  ///@name Neighbour Cache/Destination Cache
  //@{

  /// Insert a newly created entry into NC
  boost::weak_ptr<NeighbourEntry> insertNeighbourEntry(NeighbourEntry* entry);
  
  /// Remove Neighbour entry  (neighbour must exist)
  void removeNeighbourEntry(const ipv6_addr& addr);

  /// Returns true if neighbour found and removed false otherwise
  bool findNeighbourAndRemoveEntry(const ipv6_addr& addr);
 
  //@}

private:
  /**
   * @typedef NeighbourCache
   *
   * The key is ipv6_addr instead of IPv6Address because a neighbour is a real
   * network node, not some offlink subnet prefix stored in the destCache as
   * used by static routes in the XML Configuration file.  So it does not need
   * to store prefix length
   */

  typedef std::map<ipv6_addr, boost::shared_ptr<NeighbourEntry> > NeighbourCache;
  typedef NeighbourCache::iterator NCI;

  /**
     keyed on ipv6_addr of neighbour.
     Neighbour can be either RouterEntry or NeighbourEntry.
     
     Had to add a separate neighbour cache to ensure that neighbour gets deleted
     once (and only once) and also with the destEntries altered to hold
     weak_ptrs they observe that the shared_ptr has been deleted (without
     manually having to go through each destEntry looking for the correct
     neighbour to set back to 0)
   */
  NeighbourCache neighbourCache;
};
 
} //namespace IPv6NeighbourDiscovery

#endif /* INEIGHBOURCACHE_H */

