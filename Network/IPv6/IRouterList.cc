// $Header: /home/cvs/IPv6Suite/IPv6SuiteWithINET/Network/IPv6/Attic/IRouterList.cc,v 1.1 2005/02/09 06:15:58 andras Exp $
// Copyright (C) 2002, 2004 CTIE, Monash University 
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
 * @file   IRouterList.cc
 * @author Johnny Lai
 * @date   18 Apr 2002
 * 
 * @brief  Implementation of IRouterList class
 * @todo
 * 
 */


#include "IRouterList.h"


namespace IPv6NeighbourDiscovery
{

std::ostream& operator<<(std::ostream& os,
                         const IPv6NeighbourDiscovery::IRouterList::DefaultRouterList::value_type& bre)
{
  assert(bre.get());
  return os<< *(bre.get());
}

std::ostream& operator<<(std::ostream & os, const IPv6NeighbourDiscovery::IRouterList& rl)
{
 copy(rl.routers.begin(), rl.routers.end(), ostream_iterator<
       IPv6NeighbourDiscovery::IRouterList::DefaultRouterList::value_type >(os, "\n"));

  return os;
}



IRouterList::IRouterList()
{}
  

IRouterList::~IRouterList()
{}
  
///Would iterating through routers be faster or looking through DC
boost::weak_ptr<RouterEntry> IRouterList::router(const ipv6_addr& addr)
{
  for (DRLI it = routers.begin(); it != routers.end(); it++)
    if ((*it)->addr() == addr)
      return *it;
  
  return boost::weak_ptr<RouterEntry>();
}

/**
   Add router to Default Router List and place into DC   
 */
 boost::weak_ptr<RouterEntry> IRouterList::insertRouterEntry(RouterEntry* re, bool setDefault)
{
  assert(re != 0);
  boost::shared_ptr<RouterEntry> bre(re);

  if (!setDefault)
    routers.push_back(bre);
  else
    routers.push_front(bre);

  return bre;
}

/**
   Remove router entry and remove all references to it in the DC so that
   next hop neighbours for destination entries are empty
 */
void IRouterList::removeRouterEntry(const ipv6_addr& addr)
{
  for (DRLI it = routers.begin(); it != routers.end(); it++)  
    if ((*it)->addr() == addr)
    {
      //As the DRL contains the only shared_ptr by erasing it the contained
      //RouterEntry is deleted and all weak_ptr observers have the RouterEntry
      //ptr set to 0. (or so in theory)
//         DestinationEntry* de = 0;
//         DCI it2;        
//         for (it2 = beginDC(), de = nextDC(it2); de != 0; de = nextDC(it2))
//           if (de->neighbour == (*it))
//             de->neighbour = 0;
        
//         delete *it;
        routers.erase(it);  
        break;        
    }  
}


/**
 * RFC 2461 Sec. 6.3.6 algorithm for determing default router from list
 * Favour reachable i.e. with known link layer addr over those that aren't
 *
 * @note can be improved to favour only in REACHABLE or STALE state once NUD is
 * implemented
 *
 * @todo Returning default router regardless of its State i.e. we will do ND on
 * it even if other router's link layer addresses are known now. Only when NUD
 * is really implemented should we skip over the ones that are truly unreachable
 * not ones tha have undiscovered link layer addresses.
 */
boost::weak_ptr<RouterEntry> IRouterList::defaultRouter(void)
{
  boost::weak_ptr<RouterEntry> re;
  
  for (DRLI it = routers.begin(); it != routers.end(); it++)
  {
/*

  //If we do not skip this check then the default router which can reach some
  //exclusive part of the network will not be used and we get packets going back
  //and forth if another router exists and we know its link layer address.

    if ((*it)->state() == NeighbourEntry::INCOMPLETE)
      continue;
*/
    re = *it;
    break;
  }  

  //If all the routers are not reachable then just return the default one
  //and addr res will begin
  if (re.lock().get() == 0 && !routers.empty())
    re = *(routers.begin());

  return re;
}

/**
 * Default Router is always first in the list
 * 
 */

void IRouterList::setDefaultRouter(boost::weak_ptr<RouterEntry> re)
{
  boost::shared_ptr<RouterEntry> bre;
  DRLI it;
  for (it = routers.begin(); it != routers.end(); it++)
    if ((*it).get() == re.lock().get())
      bre = *it;
  
  assert(bre.get() != 0);
  routers.remove(bre);
  routers.push_front(bre);
}

}  //namespace IPv6NeighbourDiscovery


