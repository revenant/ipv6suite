// -*- C++ -*-
// Copyright (C) 2004 Johnny Lai
//
// This file is part of IPv6Suite
//
// IPv6Suite is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// IPv6Suite is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.


/**
 * @file HMIPv6fwd.hpp
 * @author Johnny Lai
 * @date 16 Nov 2004
 *
 * @brief Forward declaration of common HMIPv6 types
 *
 *
 */

#ifndef HMIPV6FWD_HPP
#define HMIPV6FWD_HPP

#ifndef VECTOR
#define VECTOR
#include <vector>
#endif //VECTOR

namespace HierarchicalMIPv6
{
 
  class HMIPv6ICMPv6NDOptMAP;
  typedef std::vector<HMIPv6ICMPv6NDOptMAP> MAPOptions;

};


#endif /* HMIPV6FWD_HPP */
