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
 * @file   IDestinationCache.cc
 * @author Johnny Lai
 * @date   18 Apr 2002
 * 
 * @brief  Implementation of IDestinationCache
 * 
 */

#include "sys.h"
#include "debug.h"

#include "IDestinationCache.h"

namespace IPv6NeighbourDiscovery
{

#ifndef USE_MOBILITY

std::ostream& operator<<
  (std::ostream& os, const pair<IPv6Address, DestinationEntry> & p)
{
  //Not implemented yet
  assert(false);
  return os;
}

#else

/**
 * @todo Output BCE too
 * 
 */

std::ostream& operator<<
  (std::ostream& os, const pair<IPv6Address, MobileIPv6::MIPv6DestinationEntry> & p)
{
  os<<"addr="<<p.first;
  
  if (p.second.neighbour.lock().get()!=0)
    os<<" "<<*(p.second.neighbour.lock().get());
  else
    os<<" No neighbour";
  return os;
}
#endif //ifndef USE_MOBILITY
  
/**
   @todo Provide a separate interface into the NC just to test for existence of
   a neighbour.
 */
boost::weak_ptr<NeighbourEntry> IDestinationCache::neighbour(const ipv6_addr& addr)
{
  boost::weak_ptr<NeighbourEntry> ne;
  DCI it;
  if (findDestEntry(addr, it))
  {
    ne = it->second.neighbour;
  }
  return ne;
}


/**
   Implements longest prefix match.  Will return 0 when the default route is
   reached in case the destination is on link.
  */
boost::weak_ptr<NeighbourEntry> IDestinationCache::lookupDestination(const ipv6_addr& addr)
{
  boost::weak_ptr<NeighbourEntry> ne;

  DCI dcit;
  if (findDestEntry(addr, dcit))
  {
    //This is not a valid assertion now since neighbour is used in place of
    //findDestEntry
    //assert(it->first == it->second.neighbour->addr());
    ne = dcit->second.neighbour;

#if defined VERBOSE || defined ROUTING
    //cout <<nodeName()<<" Found Dest entry for dest="<<addr;
    cout <<" Found Dest entry for dest="<<addr;
    if (ne.get() == 0)
      cout <<" nexthop neigbhour entry for dest is invalid"<<endl;
    else
      cout <<" nexthop addr="<<ne->addr();
    cout << endl;
#endif //defined VERBOSE || defined ROUTING

    return ne;
  }
  
  // lookup the destination cache by comparing the longest prefix
  // match between the destination addreess (subnet or the destination
  // host) in the DE and the input address

#if defined VERBOSE || defined ROUTING
  //cout <<nodeName()<<" Looking up dest addr: " << addr<<endl;
  cout <<" Looking up dest addr: " << addr<<endl;
#endif //defined VERBOSE || defined ROUTING

  for(DCI it = destCache.begin(); it != destCache.end(); it++)
  {

#if (defined VERBOSE && defined ROUTING) || defined ROUTINGTABLELOOKUP
    cout << " Dest Entry: " << it->first.address() 
         << " -- " <<__FILE__ << endl;
#endif //(defined VERBOSE && defined ROUTING) || defined ROUTINGTABLELOOKUP

    if (it->first.isNetwork(addr))  //matches like lookup
//    if (it->first.prefixMatch(addr)) //exact match
    {

#if defined VERBOSE || defined ROUTING
      cout << " Next hop neighbour found: "
           << " interface index " << it->second.neighbour->ifIndex()
           << " address " << it->second.neighbour->addr()
           << " -- " <<__FILE__ << endl;
#endif //defined VERBOSE || defined ROUTING

      ne = it->second.neighbour;

      if (it->first == IPv6_ADDR_UNSPECIFIED)
        return boost::weak_ptr<NeighbourEntry>();

      break;
    }
  }

  return ne;  
}

///Creates a new one if doesn't exist otherwise returns a reference to allow
///This form for neighbours or router entries (should create entry with 128 bit
///prefix length)
DestinationEntry& IDestinationCache::operator[] (const ipv6_addr& addr)
{
  return destCache[IPv6Address(addr)];
}

///Use this form for subnets to specify arbitrary bit length (used during static
///route creation time)
DestinationEntry& IDestinationCache::operator[] (const IPv6Address& addr)
{
  return destCache[addr];
}

///Returns the DestinationEntry at it and moves it forward by one. 
///Returns 0 when reaching the end.
///Make sure that it is a valid it obtained originally with beginDC.
inline DestinationEntry*  IDestinationCache::nextDC(DCI& it)
{
  DestinationEntry* retVal = 0;
  
  if (it != destCache.end())
  {    
    retVal = &(it->second);
    ++it;
  }  
  
  return retVal;
}

int IDestinationCache::removeDestEntryByNeighbour(const ipv6_addr& addr)
{
  DCI it, temp;
  int deletedCount = 0;

  Dout(dc::debug|continued_cf|flush_cf,
       "DC before removing dest entries with next hop="<<addr<<"\n");
#ifdef CWDEBUG
  if (libcwd::channels::dc::debug.is_on())
    copy(destCache.begin(), destCache.end(), ostream_iterator<IDestinationCache::DestinationCache::value_type>(*libcwd::libcw_do.get_ostream(), "\n"));
#endif //CWDEBUG

  for (it = destCache.begin(); it != destCache.end();   )
  {
    if (it->second.neighbour.lock().get()->addr() == addr)
    {
      temp = it;
      it++;
      deletedCount++;
      Dout(dc::dest_cache_maint, " dcache removed dest="<<temp->first<<" ngbr="<<(temp->second.neighbour.lock().get()?ipv6_addr_toString(temp->second.neighbour.lock().get()->addr()):"none"));
      destCache.erase(temp);
    }
    else
      it++;
  }

  Dout(dc::continued|flush_cf, "DC after");
#ifdef CWDEBUG
  
  if (libcwd::channels::dc::debug.is_on())
    copy(destCache.begin(), destCache.end(), ostream_iterator<IDestinationCache::DestinationCache::value_type>(*libcwd::libcw_do.get_ostream(), "\n"));
#endif //CWDEBUG
  Dout(dc::finish|flush_cf, "-|");
  return deletedCount;
}

namespace
{
  struct updateDestEntry:public std::unary_function<IDestinationCache::DestinationCache::value_type, void>
  {
    updateDestEntry(const ipv6_addr& key,
                    const boost::weak_ptr<NeighbourEntry>& replace)
      :key(key), ngbr(replace), count(0)
      {}
    //This is safe provided we don't modify data structures in value_type that
    //would reorder the map because for_each is supposed to be a non modifying
    //operation
    void operator()(IDestinationCache::DestinationCache::value_type& val)
      {
        if (val.second.neighbour.lock().get() != 0 &&
            val.second.neighbour.lock().get()->addr() == key)
        {

          Dout(dc::dest_cache_maint, " dcache replaced dest="<<val.first<<" ngbr="<<(val.second.neighbour.lock().get()?ipv6_addr_toString(val.second.neighbour.lock().get()->addr()):"none")<<" with ngbr="<<ngbr.lock().get()->addr());

          val.second.neighbour = ngbr;
          count++;
        }
        else
        {
          Dout(dc::dest_cache_maint, " dcache did NOT replace dest="<<val.first<<" ngbr="<<(val.second.neighbour.lock().get()?ipv6_addr_toString(val.second.neighbour.lock().get()->addr()):"none"));
        }
      }

    const ipv6_addr& key;
    const boost::weak_ptr<NeighbourEntry>& ngbr;
    int count;
  };

}

int IDestinationCache::updateDestEntryByNeighbour(const ipv6_addr& addr,
                                                  const ipv6_addr& ngbr)
{
  assert(addr != ngbr);
  
  boost::weak_ptr<NeighbourEntry> ne = neighbour(ngbr);
  assert(ne.lock().get() != 0);
  Dout(dc::debug|continued_cf|flush_cf, "updateDestEntryByNeighbour DC before update: "<<addr
       <<" replaced by next hop="<<ne.lock()->addr()<<"\n");

#ifdef CWDEBUG
  if (libcwd::channels::dc::debug.is_on())
    copy(destCache.begin(), destCache.end(), ostream_iterator<IDestinationCache::DestinationCache::value_type>(*libcwd::libcw_do.get_ostream(), "\n"));
#endif //CWDEBUG

  updateDestEntry update(addr, ne);
  for_each(destCache.begin(), destCache.end(), update);
  
  Dout(dc::continued|flush_cf, "DC after");
#ifdef CWDEBUG
  if (libcwd::channels::dc::debug.is_on())
    copy(destCache.begin(), destCache.end(), ostream_iterator<IDestinationCache::DestinationCache::value_type>(*libcwd::libcw_do.get_ostream(), "\n"));
#endif //CWDEBUG

  Dout(dc::finish|flush_cf, "-|");
  return update.count;
}


}//namespace NeighbourDiscovery
