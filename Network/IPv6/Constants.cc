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
 * @file   Constants.cc
 * @author Johnny Lai
 * @date   06 Jul 2004
 * 
 * @brief  Actual value of constants are here.
 *
 * 
 */

#include "Constants.h"

const int IMPL_INPUT_PORT_LOCAL_PACKET = -1;
///From Tunnel RFC 6.3
const int DEFAULT_ROUTER_HOPLIMIT = 64;

namespace IPv6NeighbourDiscovery
{
  const int IPv6NeighbourDiscovery::ICMPv6_OCTETS_UNIT = 8;
}
