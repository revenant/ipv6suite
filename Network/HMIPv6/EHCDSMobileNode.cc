// -*- C++ -*-
// Copyright (C) 2004 Johnny Lai
//
// This file is part of IPv6Suite
//
// IPv6Suite is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// IPv6Suite is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.


/**
 * @file   EHCDSMobileNode.cc
 * @author Johnny Lai
 * @date   30 Aug 2004
 *
 * @brief  Implementation of EHCDSMobileNode
 *
 */

//Headers for libcwd debug streams have to be first (remove if not used)
#include "sys.h"
#include "debug.h"

#include "EHCDSMobileNode.h"
#include "MIPv6MNEntry.h"
#include "EHTimers.h"
#include "MIPv6CDS.h"
#include "HMIPv6CDSMobileNode.h"
#include "MIPv6CDSMobileNode.h"

namespace EdgeHandover
{

///For non omnetpp csimplemodule derived classes
EHCDSMobileNode::EHCDSMobileNode(MobileIPv6::MIPv6CDS* mipv6cds, unsigned int iface_count)
    :mipv6cds(mipv6cds), bcoa(IPv6_ADDR_UNSPECIFIED),
     bmap(IPv6_ADDR_UNSPECIFIED), bcoaChangedNotifier(0)
{
}

EHCDSMobileNode::~EHCDSMobileNode()
{
}



boost::shared_ptr<MobileIPv6::MIPv6RouterEntry> EHCDSMobileNode::boundMap()
{
  if (bmap != IPv6_ADDR_UNSPECIFIED)
  {
    return mipv6cds->mipv6cdsMN->findHomeAgent(bmap);
  }
  return boost::shared_ptr<MobileIPv6::MIPv6RouterEntry>();
}

/**
   @brief Set the bound map and the corresponding bound coa

   @arg map must be a valid map that is in both the Mobile Routers List and the list of Maps

   @arg ifIndex default of 0 assumes a single interface MN change this to the
   appropriate interface for other cases

   The  MAP as used in context of HMIP need not be in MRL. In Edge Handover context since we bind
   with MAPs at the AR level obviously they are MAPs that we have visited personally.


 */
 void  EHCDSMobileNode::setBoundMap(const HierarchicalMIPv6::HMIPv6MAPEntry& map, unsigned int ifIndex)
{
  ///I think when at home you would not bind to a map although I think that
  ///would be a good idea too so you reduce the very first handover time
  if (mipv6cds->mipv6cdsMN->awayFromHome())
  {
    assert(mipv6cds->hmipv6cdsMN->mapEntries().count(map.addr()));
    //invoke callback to notify EHState that boundMap was changed and help us
    //to find the new bcoa
    ipv6_addr test = bcoaChangedNotifier(map, ifIndex);

    assert(test == map.addr());
  }
  else
  {
    setNoBoundMap();
  }
}

void EHCDSMobileNode::setNoBoundMap()
{
    assert(!mipv6cds->mipv6cdsMN->awayFromHome());
    bmap = IPv6_ADDR_UNSPECIFIED;
    bcoa = IPv6_ADDR_UNSPECIFIED;
}

}; //namespace EdgeHandover
