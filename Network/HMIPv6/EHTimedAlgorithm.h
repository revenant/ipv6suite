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
 * @file EHTimedAlgorithm.h
 * @author Johnny Lai
 * @date 30 Aug 2004
 *
 * @brief Definition of class EHTimedAlgorithm
 *
 *
 */

#ifndef EHTIMEDALGORITHM_H
#define EHTIMEDALGORITHM_H

#ifndef EHNDSTATEHOST_H
#include "EHNDStateHost.h"
#endif //EHNDSTATEHOST_H


namespace EdgeHandover
{

/**
 * @class EHTimedAlgorithm
 *
 * @brief Implements Timed Edge Handover algorithm
 *
 * binds to HA every x minutes according to XML config
 */

class EHTimedAlgorithm: public EHNDStateHost
{
 public:
#ifdef USE_CPPUNIT
  friend class EHTimedAlgorithmTest;
#endif //USE_CPPUNIT

  //@name constructors, destructors and operators
  //@{
  EHTimedAlgorithm(NeighbourDiscovery* mod);

  ~EHTimedAlgorithm();

  //@}

  //@name EHAlgorithm overrides
  //@{
  virtual void mapAlgorithm();

  ///reschedules timer so timed bindings are correct
  virtual void boundMapChanged(const ipv6_addr& old_map);
  //@}

 protected:

 private:
  
  simtime_t interval;

  //@{ Don't generate these
  EHTimedAlgorithm(const EHTimedAlgorithm& src);

  EHTimedAlgorithm& operator=(EHTimedAlgorithm& src);

  bool operator==(const EHTimedAlgorithm& rhs);
  //@}
};

}; //namespace EdgeHandover

#endif /* EHTIMEDALGORITHM_H */

