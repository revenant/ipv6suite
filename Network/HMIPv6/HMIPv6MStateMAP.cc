// $Header: /home/cvs/IPv6Suite/IPv6SuiteWithINET/Network/HMIPv6/HMIPv6MStateMAP.cc,v 1.1 2005/02/09 06:15:58 andras Exp $
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
  HMIPv6MStateMAP* HMIPv6MStateMAP::_instance = 0;

  HMIPv6MStateMAP* HMIPv6MStateMAP::instance()
  {
    if (_instance == 0)
      _instance = new HMIPv6MStateMAP;
    
    return _instance;
  }

  HMIPv6MStateMAP::~HMIPv6MStateMAP()
  {}

  HMIPv6MStateMAP::HMIPv6MStateMAP()
  {
    //MobileIPv6::MIPv6MStateHomeAgent::MIPv6MStateHomeAgent();
  }
 
  bool HMIPv6MStateMAP::processBU(IPv6Datagram* dgram, MIPv6MHBindingUpdate* bu,
                                  IPv6Mobility* mod)
  {
    if ( MobileIPv6::MIPv6MStateHomeAgent::processBU(dgram, bu, mod) == false)
      return false;
    else
    {
      // process the 'M' bit
    }
    return true;
  }
} //namespace HierarchicalMIPv6
