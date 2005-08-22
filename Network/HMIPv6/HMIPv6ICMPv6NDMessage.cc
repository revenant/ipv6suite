//
// Copyright (C) 2001, 2004 CTIE, Monash University
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
    @file HMIPv6ICMPv6NDMessage.cc

    @brief Specific HMIPv6 options

    @author Eric Wu
    @date 8/5/2002

*/

#include "HMIPv6ICMPv6NDMessage.h"
#include "ICMPv6NDOptions.h"

namespace HierarchicalMIPv6
{

  std::ostream& operator<<(std::ostream& os, const HMIPv6ICMPv6NDOptMAP& map)
  {
    return map.operator<<(os);
  }

HMIPv6ICMPv6NDOptMAP::HMIPv6ICMPv6NDOptMAP(const int dist, const int pref,
                                           const unsigned int exp,
                                           const ipv6_addr& addr,
                                           bool r, bool m, bool i, bool t,
                                           bool p, bool v
)
  : ICMPv6_NDOptionBase(IPv6NeighbourDiscovery::NDO_MAP, 3), expires(exp), map_addr(addr)
{
  assert(ipv6_addr_scope(addr) == ipv6_addr::Scope_Global);
  setDist(dist);
  setPref(pref);
  setR(r);
  setM(m);
  setI(i);
  setT(t);
  setP(p);
  setV(v);
}

void HMIPv6ICMPv6NDOptMAP::setDist(int dist)
{
  assert( dist < 0xF );
  _dist = dist;
}

void HMIPv6ICMPv6NDOptMAP::setPref(int pref)
{
  assert( pref < 0xF );
  _pref = pref;
}

void HMIPv6ICMPv6NDOptMAP::setR(bool r)
{
  _r = r;
}

void HMIPv6ICMPv6NDOptMAP::setM(bool m)
{
  _m = m;
}

void HMIPv6ICMPv6NDOptMAP::setI(bool i)
{
  _i = i;
}

void HMIPv6ICMPv6NDOptMAP::setT(bool t)
{
  _t = t;
}

void HMIPv6ICMPv6NDOptMAP::setP(bool p)
{
  _p = p;
}

void HMIPv6ICMPv6NDOptMAP::setV(bool v)
{
  _v = v;
}

} // end namespace HierarchicalMIPv6
