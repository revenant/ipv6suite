// -*- C++ -*-
// $Header: /home/cvs/IPv6Suite/IPv6SuiteWithINET/Network/IPv6/Attic/IPrefixList.h,v 1.1 2005/02/09 06:15:57 andras Exp $
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
 * @file IPrefixList.h
 * @author Johnny Lai
 * @date 18 Apr 2002
 * @brief Conceptual Data Structures for IPv6 Neighbour Discovery
 * @test Refer to PrefixListTest
 */

#ifndef IPREFIXLIST_H
#define IPREFIXLIST_H 1

#ifndef MAP
#define MAP
#include <map>
#endif //MAP
#ifndef NDENTRY_H
#include "NDEntry.h"
#endif //NDENTRY_H
#ifndef BOOST_UTILITY_HPP
#include "boost/utility.hpp"
#endif //BOOST_UTILITY_HPP

#ifdef USE_CPPUNIT
class PrefixListTest;
#endif //USE_CPPUNIT

namespace IPv6NeighbourDiscovery
{

/**
 * @class IPrefixList
 * @brief Interface for the Conceptual Data Structure PrefixList
 *
 *
 */

class IPrefixList: boost::noncopyable
{
public:

#ifdef USE_CPPUNIT
  friend class ::PrefixListTest;
#endif //USE_CPPUNIT

  //@name constructors, destructors and operators
  //@{
  IPrefixList();

  ~IPrefixList();

  //@}

  PrefixEntry* getPrefixEntry(const IPv6Address& prefix)
    {
      PLI it;
      if ((it = prefixList.find(prefix)) == prefixList.end())
        return 0;
      return &it->second.first;      
    }
  
  /// Remove pref from list.  Pref can be one obtained here or a created one
  bool removePrefixEntry(const PrefixEntry& pref)
    {      
      //Returns no. of elements erased should be one exactly
      if (prefixList.erase(pref.prefix()) == 1)
        return true;
      assert(false);
      return false;      
    }

  bool removePrefixEntry(const ipv6_prefix& pref)
    {
      if (prefixList.erase(IPv6Address(pref)) == 1)
        return true;
      return false;
    }
  
  /// Returns a vector of pointers to PrefixEntries for a particular iface
  LinkPrefixes getPrefixesByIndex(size_t ifIndex);
  
  /// Returns 
  //PrefixEntry* findPrefix(const ipv6_addr& prefix, size_t prefixLength,
  //                        PrefixEntry& returnPrefEntry, size_t& ifIndex);

  /// returns the outgoing interface for the prefix if found
  bool lookupAddress(const ipv6_addr& prefix, unsigned int& ifIndex);

protected:

  ///Associate the prefix with interface link on which it belongs  
  void insertPrefixEntry(const PrefixEntry& pe, size_t ifIndex);

private:
  
  /**
   * This map of a pair looks bad.  Because well we haven't really needed to
   * modify the interface anyway.  Should have made ifIndex a member of
   * PrefixEntry
   * 
   */

  typedef std::map<IPv6Address, std::pair<PrefixEntry, size_t> > PrefixList;
  typedef PrefixList::iterator PLI;

  /**
     @var prefixList
     Keyed on ipv6_prefix
     
     purpose: This is implementation-wise for looking up the longest
     match of the prefix list that is stored in the interface
  */
  PrefixList prefixList;
};

} //namespace IPv6NeighbourDiscovery

#endif //IPREFIXLIST_H
