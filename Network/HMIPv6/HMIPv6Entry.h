// -*- C++ -*-
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
 * @file HMIPv6Entry.h
 * @author Johnny Lai
 * @date 08 Sep 2002
 *
 * @brief Mobile Anchor Point(MAP) Entry used by MN
 * @test see HMIPv6EntryTest
 */

#ifndef HMIPV6ENTRY_H
#define HMIPV6ENTRY_H 1

#ifndef IPv6_ADDR_H
#include "ipv6_addr.h"
#endif //IPv6_ADDR_H

#ifndef CASSERT
#define CASSERT
#include <cassert>
#endif //CASSERT

#ifndef IOSFWD
#define IOSFWD
#include <iosfwd>
#endif //IOSFWD

#ifndef BOOST_OPERATORS_HPP
#include <boost/operators.hpp>
#endif //BOOST_OPERATORS_HPP

namespace HierarchicalMIPv6
{

class HMIPv6ICMPv6NDOptMAP;

/**
 * @class HMIPv6MAPEntry
 * @brief Entry for storing a MAP option
 */

class HMIPv6MAPEntry: boost::equality_comparable<HMIPv6MAPEntry>,
                      boost::less_than_comparable<HMIPv6MAPEntry>
{

  friend std::ostream& operator<<(std::ostream& os, const HMIPv6MAPEntry& me);

public:
  HMIPv6MAPEntry(const ipv6_addr& address = IPv6_ADDR_UNSPECIFIED,
                 unsigned int expires = 0 , int dist = 1, int pref=9)
    :options(0), expires(expires), mapAddr(address)
    {
      setDistance(dist);
      setPreference(pref);
    }

  HMIPv6MAPEntry(const HMIPv6ICMPv6NDOptMAP& src);

  HMIPv6MAPEntry(const HMIPv6MAPEntry& src)
    :options(src.options), expires(src.expires), mapAddr(src.mapAddr)
    {}

  bool operator==(const HMIPv6MAPEntry& rhs) const
    {
      return options == rhs.options && expires == rhs.expires &&
        mapAddr == rhs.mapAddr;
    }

  ///default map selection algorithm  from draft (furthest map is first).
  bool operator<(const HMIPv6MAPEntry& rhs) const
    {
      return distance() > rhs.distance()?true:false;
    }

  unsigned int distance() const { return (options & 0xF000)>>12; }

  void setDistance(unsigned int dist)
    {
      assert(dist < 1<<4 && dist > 0);
      options = (options & 0x0FFF)|(dist<<12);
    }

  unsigned int preference() const { return (options & 0xF00)>>8; }

  void setPreference(unsigned  int pref)
    {
      assert(pref < 1<<4);
      options = (options & 0xF0FF) | (pref<<8);
    }

  //must form rcoa based on map prefix
  bool r() const { return (options & 1<<7); }

  void setR(bool r) { r?options |= 1<<7:options &= ~(1<<7); }

  ///minimum lifetime of map addr/prefix in seconds
  unsigned int lifetime() const  { return expires; }

  void setLifetime(unsigned int exp) { expires = exp; }

  const ipv6_addr& addr() const { return mapAddr; }

  void setAddr(ipv6_addr addr) { mapAddr = addr; }

private:

  unsigned int options;
  unsigned int expires;
  ipv6_addr mapAddr;
};

  std::ostream& operator<<(std::ostream& os, const HMIPv6MAPEntry& me);

} //namespace HierarchicalMIPv6

#endif /* HMIPV6ENTRY_H */
