// $Header: /home/cvs/IPv6Suite/IPv6SuiteWithINET/Network/IPv6/Attic/INeighbourCache.cc,v 1.1 2005/02/09 06:15:57 andras Exp $
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
 * @file   INeighbourCache.cc
 * @author Johnny Lai
 * @date   18 Apr 2002
 * 
 * @brief Implementation of INeighbourCache 
 * @todo
 * 
 */


#include "INeighbourCache.h"

namespace IPv6NeighbourDiscovery
{
  
/**
 * @param entry Insert a newly constructed NeighbourEntry or RouterEntry into
 * the NeighbourCache
 * 
 * Will also insert create the corresponding DestinationEntry for this
 * NeighbourEntry.
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
