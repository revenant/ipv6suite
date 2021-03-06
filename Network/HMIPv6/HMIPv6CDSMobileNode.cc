//
// Copyright (C) 2002 CTIE, Monash University
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
 * @file   HMIPv6CDSMobileNode.cc
 * @author Johnny Lai
 * @date   08 Sep 2002
 *
 * @brief  Implementation of HMIPv6CDSMobileNode class
 *
 *
 */


#include "sys.h"
#include "debug.h"

#include "HMIPv6CDSMobileNode.h"
#include "MIPv6MNEntry.h"
#include "MIPv6CDS.h"
#include "MIPv6CDSMobileNode.h"

using MobileIPv6::bu_entry;


namespace HierarchicalMIPv6
{

HMIPv6CDSMobileNode::HMIPv6CDSMobileNode(MobileIPv6::MIPv6CDS* mipv6cds, size_t interfaceCount)
    :mipv6cds(mipv6cds), mapAddr(IPv6_ADDR_UNSPECIFIED), rcoa(IPv6_ADDR_UNSPECIFIED)
{
  Dout(dc::custom, "HMIPv6CDSMobileNode ctor");
}

HMIPv6CDSMobileNode::~HMIPv6CDSMobileNode()
{}

const ipv6_addr& HMIPv6CDSMobileNode::localCareOfAddr() const
{
  if (isMAPValid())
  {
    bu_entry* bule = mipv6cds->mipv6cdsMN->findBU(currentMap().addr());
    if (bule)
      return bule->careOfAddr();
  }

  return IPv6_ADDR_UNSPECIFIED;
}

//returns the hoa of the current map
const ipv6_addr& HMIPv6CDSMobileNode::remoteCareOfAddr() const
{
  if (isMAPValid())
  {
    bu_entry* bule = mipv6cds->mipv6cdsMN->findBU(currentMap().addr());
    if (bule)
    {
        return bule->homeAddr();
    }
  }
  return IPv6_ADDR_UNSPECIFIED;
}

ipv6_addr HMIPv6CDSMobileNode::findMapOwnsCoa(const ipv6_addr& coa) const
{
  typedef Maps::const_iterator MapsIt;
  for (MapsIt it = maps.begin(); it != maps.end(); ++it)
  {
    if (ipv6_prefix(it->first, EUI64_LENGTH).matchPrefix(coa))
      return it->first;
  }
  return IPv6_ADDR_UNSPECIFIED;
}

};

