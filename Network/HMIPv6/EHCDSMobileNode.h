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
 * @file EHCDSMobileNode.h
 * @author Johnny Lai
 * @date 30 Aug 2004
 *
 * @brief Definition of class EHCDSMobileNode
 *
 */

#ifndef EHCDSMOBILENODE_H
#define EHCDSMOBILENODE_H

#ifndef IPv6_ADDR_H
#include "ipv6_addr.h"
#endif

#ifndef BOOST_SHARED_PTR_HPP
#include <boost/shared_ptr.hpp>
#endif

#ifndef EHTIMERS_H
#include "EHTimers.h"
#endif //EHTIMERS_H

class cTimerMessage;

namespace MobileIPv6
{
  class MIPv6CDS;
  class MIPv6RouterEntry;
}

namespace HierarchicalMIPv6
{
  class HMIPv6MAPEntry;
}

namespace EdgeHandover
{

/**
 * @class EHCDSMobileNode
 *
 * @brief Basic state for Edge Handover
 *
 * Contains the bcoa and bmap and associated functions
 */

  class EHCDSMobileNode
{
 public:
  friend class EHNDStateHost;
#ifdef USE_CPPUNIT
  friend class EHCDSMobileNodeTest;
#endif //USE_CPPUNIT

  //@name constructors, destructors and operators
  //@{
  EHCDSMobileNode(MobileIPv6::MIPv6CDS* mipv6cds, unsigned int iface_count);

  ~EHCDSMobileNode();

  const ipv6_addr& boundCoa(void) const { return bcoa; }

  const ipv6_addr& boundMapAddr(void) const { return bmap; }

  boost::shared_ptr<MobileIPv6::MIPv6RouterEntry> boundMap();

  void setBoundMap(const HierarchicalMIPv6::HMIPv6MAPEntry& map, unsigned int ifIndex = 0);

  void setNoBoundMap();
 protected:

 private:

  //@{ Don't generate these
  EHCDSMobileNode(const EHCDSMobileNode& src);

  EHCDSMobileNode& operator=(EHCDSMobileNode& src);

  bool operator==(const EHCDSMobileNode& rhs);
  //@}

  MobileIPv6::MIPv6CDS* mipv6cds;

 /**
     @brief bound care-of-address (coa)

     bcoa is usually the HA BC's coa except when we send a BU to HA and the coa
     is different. Bcoa does not change until we receive BA from HA.
  */
  ipv6_addr bcoa;
  ///bound MAPs address
  ipv6_addr bmap;

  BoundMapChangedCB bcoaChangedNotifier;
};

}; //namespace EdgeHandover

#endif /* EHCDSMOBILENODE_H */

