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
 * @file   INeighbourCache.cc
 * @author Johnny Lai
 * @date   18 Apr 2002
 *
 * @brief Implementation of INeighbourCache
 * @todo
 *
 */


#include "INeighbourCache.h"
#include "stlwatch.h"


namespace IPv6NeighbourDiscovery
{

std::ostream& operator<<
  (std::ostream& os, const INeighbourCache::NeighbourCache::value_type & p)
{
  os<<" neighbour="<<p.first<<" "<<*(p.second);
  return os;
}

std::ostream& operator<<
  (std::ostream& os, const std::pair<ipv6_addr, boost::shared_ptr<NeighbourEntry> > & p)
{
  os<<" neighbour="<<p.first<<" "<<*(p.second);
  return os;
}

INeighbourCache::INeighbourCache()
{
  WATCH_PTRMAP(neighbourCache);
}


/**
 * @param entry Insert a newly constructed NeighbourEntry or RouterEntry into
 * the NeighbourCache
 *
 *
 * @warning It is assumed that you want to replace the existing neighbour if one
 * exists with the same address
 */
boost::weak_ptr<NeighbourEntry> INeighbourCache::insertNeighbourEntry(NeighbourEntry* entry)
{
  assert(entry != 0);

  //Create new or find existing entry
  boost::weak_ptr<NeighbourEntry> ptr = neighbourCache[entry->addr()];
#ifdef NDCDSDEBUG
  if (ptr.get() != 0)
    cout << "Replacing an existing NeighbourEntry in NC with addr="
         <<entry->addr() << " replaced ptr="<<ptr.get()<<" new ptr="<<entry<<'\n';

#endif //NDCDSDEBUG

  //Insert neighbour entry here
  ptr = neighbourCache[entry->addr()] = boost::shared_ptr<NeighbourEntry> (entry);
  return ptr;
}



void INeighbourCache::removeNeighbourEntry(const ipv6_addr& addr)
{
  assert(neighbourCache.erase(addr) == 1);
}

bool INeighbourCache::findNeighbourAndRemoveEntry(const ipv6_addr& addr)
{
  return neighbourCache.erase(addr) == 1;
}

} //namespace IPv6NeighbourDiscovery
