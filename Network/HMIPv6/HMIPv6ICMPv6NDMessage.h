// -*- C++ -*-
// Copyright (C) 2001 CTIE, Monash University 
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
	@file HMIPv6ICMPv6NDMessage.h
    
	@brief Specific HMIPv6 options
    
	@author Eric Wu
    @date 7/5/2002
 
*/

#ifndef HMIPV6ICMPV6NDMESSAGE_H
#define HMIPV6ICMPV6NDMESSAGE_H

#ifndef ICMPV6NDOPTIONBASE_H
#include "ICMPv6NDOptionBase.h"
#endif // ICMPV6NDOPTIONBASE_H

#ifndef VECTOR
#include <vector>
#endif //VECTOR

#ifndef IPV6_ADDR_H
#include "ipv6_addr.h"
#endif //IPv6_ADDR_H

namespace XMLConfiguration
{
  class IPv6XMLParser;
  class IPv6XMLWrapManager;
  class XMLOmnetParser;
}

namespace HierarchicalMIPv6
{

class HMIPv6ICMPv6NDOptMAP : public IPv6NeighbourDiscovery::ICMPv6_NDOptionBase
{
  friend class XMLConfiguration::IPv6XMLParser;
  friend class XMLConfiguration::IPv6XMLWrapManager;
  friend class XMLConfiguration::XMLOmnetParser;

 public:
  HMIPv6ICMPv6NDOptMAP(const int dist = 0, const int pref = 0, 
                       const unsigned int expires = 0,
                       const ipv6_addr& map_addr = IPv6_ADDR_UNSPECIFIED,
                       bool r = false,
                       bool m = false, bool i = false, bool t = false,
                       bool p = false, bool v = false);

  virtual HMIPv6ICMPv6NDOptMAP* dup() const
    {
      return new HMIPv6ICMPv6NDOptMAP(*this);
    }

  void setDist(int dist);
  void setPref(int pref);
  
  void setR(bool r);
  void setM(bool m);
  void setI(bool i);
  void setT(bool t);
  void setP(bool p);
  void setV(bool v);

  size_t ifaceIdx() { return iface_idx; }

  unsigned int dist(void) const { return _dist; }
  unsigned int pref(void) const { return _pref; }
  bool r(void) const { return _r; }
  bool m(void) const { return _m; }
  bool i(void) const { return _i; }
  bool t(void) const { return _t; }
  bool p(void) const { return _p; }
  bool v(void) const { return _v; }

  /**
   * prefix length assumed to be 24
   * 
   */

  const ipv6_addr& addr() const
    {
      return map_addr;
    }
  
  unsigned int lifetime() const
    {
      return expires;
    }
  
 private:
  unsigned int _dist;
  unsigned int _pref;
  bool _r; // basic mode flag
  bool _m; // extended mode flag
  bool _i; // a flag indicates the MN MAY use its RCoA as source address
  bool _t; // a flag indicates the on-link prefix is topogically incorrect
  bool _p; // a flag indicates the MN MUST use its RCoA as source address
  bool _v; // a flag indicates reverse tunnelling of outbound traffic to the MAP is allowed
  unsigned int expires;
  size_t iface_idx;

  ipv6_addr map_addr; // MAP's global address
};

typedef std::vector<HMIPv6ICMPv6NDOptMAP> MAPOptions;
  
} // end namespace HierarchicalMIPv6

#endif // HMIPV6ICMPV6NDMESSAGE_H
