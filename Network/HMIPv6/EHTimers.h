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
 * @file EHTimers.h
 * @author Johnny Lai
 * @date 30 Aug 2004
 *
 * @brief Some typedefs that are shared by two loosely coupled classes
 * MobileIPv6::MIPv6MStateMobileNode and EdgeHandover::EHNDStateHost
 *
 */

#ifndef EHTIMERS_H
#define EHTIMERS_H

namespace EdgeHandover
{

  typedef Loki::cTimerMessageCB<void, TYPELIST_1(IPv6Datagram*)> EHCallback;
  typedef Loki::cTimerMessageCB<ipv6_addr,
                                TYPELIST_2(HierarchicalMIPv6::HMIPv6MAPEntry,
                                           unsigned int)>
  BoundMapChangedCB;

};

#endif //EHTIMERS_H
