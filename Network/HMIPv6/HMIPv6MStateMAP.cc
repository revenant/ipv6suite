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
 * @file   HMIPv6MStateMAP.cc
 * @author Johnny Lai
 * @date   05 Sep 2002
 *
 * @brief Implementation of HMIPv6MStateMAP class
 *
 * Probably reuses exactly HA superclass for Basic mode but Extended mode would
 * be quite different
 */


#include "HMIPv6MStateMAP.h"

namespace HierarchicalMIPv6
{
  HMIPv6MStateMAP::~HMIPv6MStateMAP()
  {}

  HMIPv6MStateMAP::HMIPv6MStateMAP(IPv6Mobility* mob):MIPv6MStateHomeAgent(mob)
  {
    //MobileIPv6::MIPv6MStateHomeAgent::MIPv6MStateHomeAgent();
  }

  bool HMIPv6MStateMAP::processBU(IPv6Datagram* dgram, MIPv6MHBindingUpdate* bu)
  {
    if ( MobileIPv6::MIPv6MStateHomeAgent::processBU(dgram, bu) == false)
      return false;
    else
    {
      // process the 'M' bit
      //bu->mapreg()
    }
    return true;
  }
} //namespace HierarchicalMIPv6
