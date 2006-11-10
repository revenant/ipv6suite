// -*- C++ -*-
// Copyright (C) 2002, 2003, 2004 CTIE, Monash University
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
 * @file HMIPv6NDStateHost.h
 * @author Johnny Lai
 * @date 04 Sep 2002
 *
 * @brief MN operations to support HMIPv6
 * Basic mode implementation
 */

#ifndef HMIPV6NDSTATEHOST_H
#define HMIPV6NDSTATEHOST_H 1

#ifndef MIPV6NDSTATEHOST_H
#include "MIPv6NDStateHost.h"
#endif //MIPV6NDSTATEHOST_H

#ifndef BOOST_TUPLE_HPP
#include <boost/tuple/tuple.hpp>
#endif //BOOST_TUPLE_HPP

#ifndef HMIPV6ENTRY_H
#include "HMIPv6Entry.h"
#endif //HMIPV6ENTRY_H

#ifndef HMIPV6FWD_HPP
#include "HMIPv6fwd.hpp"
#endif //HMIPV6FWD_HPP

namespace HierarchicalMIPv6
{

  class HMIPv6CDSMobileNode;

/**
 * @class HMIPv6NDStateHost
 *
 * @brief Operations required for HMIPv6 support in MN
 *
 * Only Basic mode implemented
 */

class HMIPv6NDStateHost: public MobileIPv6::MIPv6NDStateHost
{
public:

  //@name constructors, destructors and operators
  //@{
  HMIPv6NDStateHost(NeighbourDiscovery* mod);

  virtual ~HMIPv6NDStateHost();

  //@}

  ///Override MIPv6NDStateHost
  std::auto_ptr<ICMPv6Message> processMessage(std::auto_ptr<ICMPv6Message> msg);


  /**
   * Handle RtrAd with the HomeAgent bit set
   * MAP discovery and handover between MAPs.
   */

  virtual std::auto_ptr<RA> processRtrAd(std::auto_ptr<RA> msg);

protected:

  ///Returns the best MAP to bind with according to HMIPv6MAPEntry::operator<
  virtual HierarchicalMIPv6::HMIPv6MAPEntry selectMAP(MAPOptions& maps, MAPOptions::iterator& new_end);

  ///Selects a map based on HMIPv6NDStateHost::selectMAP and does local or map
  ///handover
  std::auto_ptr<RA> discoverMAP(std::auto_ptr<RA> rtrAdv);

  //arhandover used by MIPv6NDStateHost::sendBU;
  friend class MobileIPv6::MIPv6NDStateHost;
  /**
   * @brief Does AR-AR handover with local MAP
   *
   * Also does pcoa forwarding from par to nar if they have HA func.
   */
  bool arhandover(const ipv6_addr& lcoa);

  ///Prepares for the MAP to MAP handover
  void preprocessMapHandover(const HMIPv6MAPEntry& bestMap,
                             boost::shared_ptr<MobileIPv6::MIPv6RouterEntry> accessRouter,
                             IPv6Datagram* dgram);

  //virtual void handover(boost::shared_ptr<MobileIPv6::MIPv6RouterEntry> newRtr);

  typedef boost::tuple<HMIPv6MAPEntry, ipv6_addr, ipv6_addr,simtime_t, unsigned int> ArgMapHandover;

  ///Actual MAP-MAP handover
  void mapHandover(const ArgMapHandover& t);

  ///Forms a remote coa from prefix of MAP me at the interface ifIndex
  ipv6_addr formRemoteCOA(const HMIPv6MAPEntry& me, unsigned int ifIndex);

  HMIPv6CDSMobileNode& hmipv6cdsMN;

private:
};

}  //namespace HierarchicalMIPv6

#endif /* HMIPV6NDSTATEHOST_H */
