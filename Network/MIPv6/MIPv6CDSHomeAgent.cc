// $Header: /home/cvs/IPv6Suite/IPv6SuiteWithINET/Network/MIPv6/MIPv6CDSHomeAgent.cc,v 1.1 2005/02/09 06:15:58 andras Exp $
// Copyright (C) 2002, 2004 CTIE, Monash University 
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
 * @file   MIPv6CDSHomeAgent.cc
 * @author Johnny Lai
 * @date   05 May 2002
 * 
 * @brief Implementation of MIPv6CDSHomeAgent
 * @todo Implement HomeAgent CDS interface functions and complete the interface
 * 
 */


#include "MIPv6CDSHomeAgent.h"

namespace MobileIPv6
{

  MIPv6CDSHomeAgent::MIPv6CDSHomeAgent(size_t interfaceCount)
  {
    //Home Agents maintain separate lists for each interface
    halists.resize(interfaceCount);
  }
  
  MIPv6CDSHomeAgent::~MIPv6CDSHomeAgent()
  {
    halists.clear();
  }

  boost::weak_ptr<ha_entry> MIPv6CDSHomeAgent::findHomeAgent(const ipv6_addr& addr)
  {
    HALI it = hal.find(addr);
    if (it != hal.end())
      return it->second;
    return boost::weak_ptr<ha_entry>();
  }
  
  void MIPv6CDSHomeAgent::insertHomeAgent(ha_entry* ha)
  {
    boost::shared_ptr<ha_entry> bha(ha);
    hal[ha->local_addr()] = bha;
    assert(ha->ifIndex() < halists.size());
    halists[ha->ifIndex()].push_back(bha);
  }
  
  /**
   * @pre ha has to exist inside the home agent list
   * 
   */
  void MIPv6CDSHomeAgent::removeHomeAgent(boost::weak_ptr<ha_entry> ha)
  {
    assert(hal.erase(ha.lock()->local_addr()) == 1);
  }

} //namespace MobileIPv6
