// -*- C++ -*-
// Copyright (C) 2008 Johnny Lai
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
 * @file EHHopAlgorithm.h
 * @author Johnny Lai
 * @date 06 May 2008
 *
 * @brief Definition of class EHHopAlgorithm
 *
 * @test see EHHopAlgorithmTest
 *
 */

#ifndef EHHOPALGORITHM_H
#define EHHOPALGORITHM_H


#ifndef EHNDSTATEHOST_H
#include "EHNDStateHost.h"
#endif //EHNDSTATEHOST_H


namespace EdgeHandover
{

/**
 * @class EHHopAlgorithm
 *
 * @brief Implements Timed Edge Handover algorithm
 *
 * binds to HA every x minutes according to XML config
 */

class EHHopAlgorithm: public EHNDStateHost
{
 public:
#ifdef USE_CPPUNIT
  friend class EHHopAlgorithmTest;
#endif //USE_CPPUNIT

  //@name constructors, destructors and operators
  //@{
  EHHopAlgorithm(NeighbourDiscovery* mod);

  ~EHHopAlgorithm();

  //@}

  //@name EHAlgorithm overrides
  //@{
  virtual void hopAlgorithm();

  ///reschedules timer so timed bindings are correct
  virtual void boundMapChanged(const ipv6_addr& old_map);
  //@}

 protected:

  unsigned int hopCount;
  unsigned int hopCountThreshold; 

private:

 //@{ Don't generate these
  EHHopAlgorithm(const EHHopAlgorithm& src);

  EHHopAlgorithm& operator=(EHHopAlgorithm& src);

  bool operator==(const EHHopAlgorithm& rhs);
  //@}
};

}; //namespace EdgeHandover



#endif /* EHHOPALGORITHM_H */

