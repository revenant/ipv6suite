//
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

using MobileIPv6::bu_entry;


namespace HierarchicalMIPv6
{

HMIPv6CDSMobileNode::HMIPv6CDSMobileNode(size_t interfaceCount)
  :MIPv6CDSMobileNode(interfaceCount), mapAddr(IPv6_ADDR_UNSPECIFIED),
   rcoa(IPv6_ADDR_UNSPECIFIED)
{
  Dout(dc::custom, "HMIPv6CDSMobileNode ctor");
}

HMIPv6CDSMobileNode::~HMIPv6CDSMobileNode()
{}

const ipv6_addr& HMIPv6CDSMobileNode::localCareOfAddr() const
{
  if (isMAPValid())
  {
    bu_entry* bule = findBU(currentMap().addr());
    if (bule)
      return bule->careOfAddr();
  }

  return IPv6_ADDR_UNSPECIFIED;
}

const ipv6_addr& HMIPv6CDSMobileNode::remoteCareOfAddr() const
{
  if (isMAPValid())
  {
    bu_entry* bule = findBU(currentMap().addr());
    if (bule)
    {
        return bule->homeAddr();
    }
  }
  return IPv6_ADDR_UNSPECIFIED;
}

};

