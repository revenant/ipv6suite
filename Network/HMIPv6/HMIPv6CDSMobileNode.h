// -*- C++ -*-
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
 * @file HMIPv6CDSMobileNode.h
 * @author Johnny Lai
 * @date 08 Sep 2002
 *
 * @brief Conceptual Data Structure model for a HMIPv6 Mobile Node
 *
 *
 */

#ifndef HMIPV6CDSMOBILENODE_H
#define HMIPV6CDSMOBILENODE_H 1

#ifndef MIPV6CDSMOBILENODE_H
#include "MIPv6CDSMobileNode.h"
#endif //MIPV6CDSMOBILENODE_H

#ifndef HMIPV6ENTRY_H
#include "HMIPv6Entry.h"
#endif //HMIPV6ENTRY_H

#ifndef CASSERT
#define CASSERT
#include <cassert>
#endif //CASSERT

namespace HierarchicalMIPv6
{

/**
 * @class HMIPv6CDSMobileNode
 *
 * @brief Contains operations for MAP entries necessary for HMIPv6 MNs.
 *
 *
 */

class HMIPv6CDSMobileNode: public MobileIPv6::MIPv6CDSMobileNode
{
public:
  typedef std::map<ipv6_addr, HMIPv6MAPEntry> Maps;
  typedef Maps::iterator MapsIt;

  //@name constructors, destructors and operators
  //@{
  HMIPv6CDSMobileNode(size_t interfaceCount);

  ~HMIPv6CDSMobileNode();
  //@}

  const Maps& mapEntries() const
    {
      return maps;
    }

  Maps& mapEntries()
    {
      return maps;
    }

  ///Is the current map valid
  bool isMAPValid() const
    {
      return mapAddr != IPv6_ADDR_UNSPECIFIED && maps.count(mapAddr);
    }

  const ipv6_addr& localCareOfAddr() const;
  const ipv6_addr& remoteCareOfAddr() const;

  ///Required func as BU to pHA is not done until BA from MAP received
  void setRemoteCareOfAddr(const ipv6_addr& orcoa) { rcoa = orcoa; }

  HMIPv6MAPEntry& currentMap()
    {
      assert(maps.count(mapAddr));
      return maps[mapAddr];
    }

  const HMIPv6MAPEntry& currentMap() const
    {
      Maps::const_iterator it = maps.find(mapAddr);
      assert(it != maps.end());
      return it->second;
    }

  /**
   * @par addr address of MAP assumed to be in the list of MAPS already
   *
   */

  bool setCurrentMap(const ipv6_addr& addr)
    {
      assert(maps.count(addr) == 1);
      if (!maps.count(addr))
        return false;
      mapAddr = addr;
      return true;
    }

  void setNoCurrentMap()
    {
      mapAddr = IPv6_ADDR_UNSPECIFIED;
    }

protected:

private:
  Maps maps;

  ipv6_addr mapAddr;
  ///Home address registered at prospective MAP
  mutable ipv6_addr rcoa;
};

} //namespace HierarchicalMIPv6

#endif /* HMIPV6CDSMOBILENODE_H */
