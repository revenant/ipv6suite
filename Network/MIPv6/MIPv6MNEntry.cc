//
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
 * @file   MIPv6MNEntry.cc
 * @author Johnny Lai
 * @date   06 Sep 2002
 *
 * @brief  Implementation of MIPv6RouterEntry and bu_entry classes
 *
 */


#include "MIPv6MNEntry.h"

#include "NDEntry.h"
#include <algorithm>

namespace MobileIPv6
{

  std::ostream& operator<<(std::ostream& os, const MobileIPv6::MIPv6RouterEntry& re)
  {
    return os<<"gaddr="<<re.addr()<<" isHA="<<(re.isHomeAgent()?
                                               "HA":"not HA")
             <<" lifetime="<<re.lifetime()<<" pref="<<re.preference()<<" "
             <<*(re.re.lock());
  }

  std::ostream& operator<<(std::ostream& os, const MobileIPv6::bu_entry& bule)
  {
    return bule.operator<<(os);
  }

  void MIPv6RouterEntry::addOnLinkPrefix(IPv6NeighbourDiscovery::PrefixEntry* pe)
  {
    assert(pe != 0);
    const ipv6_prefix prefix(pe->prefix(), pe->prefix().prefixLength());
    if (std::find(prefixes.begin(), prefixes.end(), prefix) == prefixes.end())
      prefixes.push_back(prefix);
  }

  /**
   * Will overide the router's lifetime iff router lifetime is longer than ha
   * lifetime and ha lifetime is not 0.  Its not implemented like this now
   * however it could be if there is no point in having two different lifetimes.
   */

  void MIPv6RouterEntry::setLifetime(unsigned int lifetime)
  {
    _lifetime = lifetime;
  }

  unsigned int MIPv6RouterEntry::lifetime() const
  {
    return _lifetime;
  }

  std::ostream& bu_entry::operator<<(std::ostream& os) const
  {
    return os<<"addr="<<addr()<<" hoa="<<homeAddr()<<" coa="<<careOfAddr()
      <<" h="<<homeReg()<<" sequence="<<sequence()<<" exp="<<expires()
      <<" lifetime="<<lifetime()<<" problem="<<problem<<" state="<<state
      <<" last="<<last_time_sent
#ifdef USE_HMIP
      <<" m="<<isMobilityAnchorPoint()
#endif //USE_HMIP
;
  }

  void bu_entry::setLifetime(unsigned int life)
  {
    _lifetime = life;
    setExpires(_lifetime);
  }

  void bu_entry::setExpires(unsigned int exp)
  {
    _expires = exp;
  }

} //namespace MobileIPv6
