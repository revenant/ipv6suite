// -*- C++ -*-
// Copyright (C) 2006 Johnny Lai
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
 * @file HMIPv6MStateMobileNode.h
 * @author Johnny Lai
 * @date 20 Nov 2006
 *
 * @brief Definition of class HMIPv6MStateMobileNode
 *
 * @test see HMIPv6MStateMobileNodeTest
 *
 * @todo Remove template text
 */

#ifndef HMIPV6MSTATEMOBILENODE_H
#define HMIPV6MSTATEMOBILENODE_H

#ifndef __OMNETPP_H
#include <omnetpp.h>
#endif //__OMNETPP_H

#ifndef MIPV6MSTATEMOBILENODE_H
#include "MIPv6MStateMobileNode.h"
#endif

namespace HierarchicalMIPv6
{

/**
 * @class HMIPv6MStateMobileNode
 *
 * @brief hmip6 specific processing
 *
 * was rafactored from MIPv6MStateMobileNode
 */

class HMIPv6MStateMobileNode: public MobileIPv6::MIPv6MStateMobileNode
{
 public:
  
  //@name constructors, destructors and operators
  //@{
  HMIPv6MStateMobileNode(IPv6Mobility* mob);

  ~HMIPv6MStateMobileNode();

  virtual void initialize(int stage = 0);

  //@}

 protected:
  //update the tunnels based on BU just done
  virtual bool updateTunnelsFrom(ipv6_addr budest, ipv6_addr coa, unsigned int ifIndex,
		    bool homeReg, bool mapReg);

  virtual bool processBA(BA* ba, IPv6Datagram* dgram);

 private:
  //not needed
  HMIPv6MStateMobileNode(const HMIPv6MStateMobileNode& src);
  HMIPv6MStateMobileNode();
};

};

#endif /* HMIPV6MSTATEMOBILENODE_H */

