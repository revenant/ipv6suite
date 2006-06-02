// -*- C++ -*-
//
// Copyright (C) 2006 by Johnny Lai
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
 * @file HMIPv6NDStateRouter.h
 * @author Johnny Lai
 * @date 05 Sep 2002
 * @brief Interface for HMIPv6NDStateRouter class
 */

#ifndef HMIPV6NDSTATEROUTER_H
#define HMIPV6NDSTATEROUTER_H 1

#ifndef MIPV6NDSTATEROUTER_H
#include "MIPv6NDStateRouter.h"
#endif //MIPV6NDSTATEROUTER_H

namespace XMLConfiguration
{
  class IPv6XMLParser;
}

namespace HierarchicalMIPv6
{

/**
 * @class HMIPv6NDStateRouter
 * @brief Advertise own MAP options and collect MAP options from
 * higher up in MAP hierarchy for forwarding purposes.
 *
 */

class HMIPv6NDStateRouter: public MobileIPv6::MIPv6NDStateRouter
{
  friend class XMLConfiguration::IPv6XMLParser;

public:

  //@name constructors, destructors and operators
  //@{
  HMIPv6NDStateRouter(NeighbourDiscovery* mod);

  virtual ~HMIPv6NDStateRouter();
  //@}

  void addMAP(HMIPv6ICMPv6NDOptMAP mapEntry)
    {
      mapOptions.push_back(mapEntry);
    }

  virtual void print();

  bool mnMUSTSetRCoAAsSource;

 protected:
  virtual ICMPv6NDMRtrAd* createRA(const IPv6InterfaceData::RouterVariables& rtr, size_t ifidx);
/*
 private:
  HMIPv6ICMPv6NDOptMAP* getMAPbyInterface(size_t iface_Idx);
*/
 private:
  MAPOptions mapOptions;
};

} //namespace HierarchicalMIPv6

#endif /* HMIPV6NDSTATEROUTER_H */

