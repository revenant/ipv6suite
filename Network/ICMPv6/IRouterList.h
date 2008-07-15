// -*- C++ -*-
//
// Copyright (C) 2002, 2003 CTIE, Monash University
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
 * @file IRouterList.h
 * @author Johnny Lai
 * @date 18 Apr 2002
 * @brief Conceptual Data Structures for IPv6 Neighbour Discovery
 */

#ifndef IROUTERLIST_H
#define IROUTERLIST_H 1

#include <list>

#include <boost/utility.hpp>
#include <boost/weak_ptr.hpp>

#ifndef IPV6NDENTRY_H
#include "NDEntry.h"
#endif //IPV6NDENTRY_H

namespace IPv6NeighbourDiscovery
{

/**
 * @class IRouterList
 * @brief Interface for Conceptual Data Structure RouterList
 *
 * detailed description
 */

class IRouterList: public boost::noncopyable
{
  friend std::ostream& operator<<(std::ostream& os,
                                  const IPv6NeighbourDiscovery::IRouterList& rl);

public:

  //@name constructors, destructors and operators
  //@{
  IRouterList();

  ~IRouterList();

  //@}

  ///@name Default Router List
  //@{
  /// returns default router entry with interface index given
  //migrate to Routing?
  boost::weak_ptr<RouterEntry> defaultRouter(void);

  /// Make re the default next hop (re is already in DRL)
  void setDefaultRouter(boost::weak_ptr<RouterEntry> re);

  /// insert router entry into the list
  boost::weak_ptr<RouterEntry> insertRouterEntry(RouterEntry* re, bool setDefault = false);

  /// delete router entry from the list
  void removeRouterEntry(const ipv6_addr& addr);

  ///Return a router Object or 0 if not found
  boost::weak_ptr<RouterEntry> router(const ipv6_addr& addr);

  size_t routerCount() const { return routers.size(); }

  //@}

protected:

  /**
     @var routers
     Not all routers in destinationEntry are default Routers (I
     think) A router becomes a default router when it exists in the
     DefaultRouterList 'routers'.
     Favours reachable over those whose reachability is suspect.
     Sort according to reachability. i.e. define a custom compare function.
  */
  typedef std::list<boost::shared_ptr<RouterEntry> > DefaultRouterList;
  typedef DefaultRouterList::iterator DRLI;
  DefaultRouterList routers;

  friend std::ostream& operator<<(std::ostream& os,
                                  const IPv6NeighbourDiscovery::IRouterList::DefaultRouterList::value_type& rl);

};

  std::ostream& operator<<(std::ostream& os,
                           const IPv6NeighbourDiscovery::IRouterList& rl);
  std::ostream& operator<<(std::ostream& os,
                           const IPv6NeighbourDiscovery::IRouterList::DefaultRouterList::value_type& bre);

} //namespace IPv6NeighbourDiscovery


#endif /* IROUTERLIST_H */
