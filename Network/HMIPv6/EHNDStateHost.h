// -*- C++ -*-
// Copyright (C) 2004, 2008 Johnny Lai
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
 * @file EHNDStateHost.h
 * @author Johnny Lai
 * @date 22 Aug 2004
 *
 * @brief Definition of class EHNDStateHost
 *
 */

#ifndef EHNDSTATEHOST_H
#define EHNDSTATEHOST_H

#ifndef HMIPV6NDSTATEHOST
#include "HMIPv6NDStateHost.h"
#endif //HMIPV6NDSTATEHOST

namespace EdgeHandover
{

  class EHCDSMobileNode;

/**
 * @class EHNDStateHost
 *
 * @brief Base class for Edge Handover
 *
 * HMIPv6 whenever it changes Maps will send a BU to HA.  
 * In EH we stop it from doing that and decide when to do 
 * so ourselves.  curentMap/rcoa/lcoa in this context is
 * used as a temporary store when bmap not established yet. 
 * It can also mean the map entry for the current AR and 
 * hence can be different from bmap. A bmap is strictly what
 * the HA acknowledges in BA with coa = bcoa equiv to prefix 
 * formed from bmap. 
 * Most EH operations are in EHNDStateHost or descendants. 
 * However HMIPv6MStateMobileNodes::processBA contains some EH
 * code unless we create a parallel hierarchy for EH there too. 
 */

class EHNDStateHost: public HierarchicalMIPv6::HMIPv6NDStateHost
{
 public:
#ifdef USE_CPPUNIT
  friend class EHNDStateHostTest;
#endif //USE_CPPUNIT

  typedef HierarchicalMIPv6::MAPOptions MAPOptions;
  typedef HierarchicalMIPv6::HMIPv6MAPEntry HMIPv6MAPEntry;

  ///creates the correct algorithm depending on handoverType
  static EHNDStateHost* create(NeighbourDiscovery* mod, const std::string& handoverType);

  void invokeMapAlgorithmCallback();

  ipv6_addr invokeBoundMapChangedCallback(const HierarchicalMIPv6::HMIPv6MAPEntry& map,
                                          unsigned int ifIndex);
 protected:

  //@name constructors, destructors and operators
  //@{
  EHNDStateHost(NeighbourDiscovery* mod);

  virtual ~EHNDStateHost();
  //@}

  //@{ overridden HMIPv6NDStateHost functions
  ///Check and do returning home case
  virtual void returnHome();

  ///idea is for subclasses to either override this
  ///or mapAlgorithm to do their own things
  virtual std::auto_ptr<RA> discoverMAP(std::auto_ptr<RA> rtrAdv);

  ///selectMAP unused currently was used previously when hmip sort of
  ///called this and got all confused.
  virtual HMIPv6MAPEntry selectMAP(MAPOptions &maps, MAPOptions::iterator& new_end);
  //@}

  //@{ EH Algorithm functions (may include discoverMAP too)

  ///Subclasses have to override this
  virtual void mapAlgorithm() = 0;

  /**
     @brief Subclasses can override this if they are interested default method
     does nothing

  */
  virtual void boundMapChanged(const ipv6_addr& old_map);
  //@}

  ///contains bcoa and bmap as mentioned in EH spec
  EHCDSMobileNode* ehcds;
 private:

  //@{ Don't generate these
  EHNDStateHost(const EHNDStateHost& src);

  EHNDStateHost& operator=(EHNDStateHost& src);

  bool operator==(const EHNDStateHost& rhs);
  //@}
};

};
#endif /* EHNDSTATEHOST_H */

