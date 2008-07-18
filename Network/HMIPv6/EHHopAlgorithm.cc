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
 * @file   EHHopAlgorithm.cc
 * @author Johnny Lai
 * @date   06 May 2008
 *
 * @brief  Implementation of EHHopAlgorithm
 *
 *
 */

//Headers for libcwd debug streams have to be first (remove if not used)
#include "sys.h"
#include "debug.h"

//Headers for libcwd debug streams have to be first (remove if not used)
#include "sys.h"
#include "debug.h"

#include "EHHopAlgorithm.h"
#include "IPv6Mobility.h"
#include "NeighbourDiscovery.h"
#include "cTimerMessage.h"
#include "MIPv6MNEntry.h"
#include "EHTimers.h"
#include "MIPv6CDSMobileNode.h"
#include "HMIPv6CDSMobileNode.h"
#include "EHCDSMobileNode.h"
#include "MIPv6MStateMobileNode.h"
#include "RoutingTable6.h"

namespace EdgeHandover
{

EHHopAlgorithm::EHHopAlgorithm(NeighbourDiscovery* mod):
  EHNDStateHost(mod), hopCount(0), hopCountThreshold(1)
{
  //Read xml info on Timed algorithm params
  hopCountThreshold = mob->par("hopCountThreshold");
}

EHHopAlgorithm::~EHHopAlgorithm()
{
}

void EHHopAlgorithm::hopAlgorithm()
{
  hopCount++;
  if (hopCountThreshold != hopCount)
    return;

  //change bmap by
  //send a map bu to currentMap
  //then we'll bind to currentMap at mapAlgorithm called by processBA
  if (hmipv6cdsMN.remoteCareOfAddr() == IPv6_ADDR_UNSPECIFIED ||
      ehcds->boundCoa() != hmipv6cdsMN.remoteCareOfAddr())
  {
    IPv6Datagram* dgram = new IPv6Datagram();
    dgram->setInputPort(0);
    preprocessMapHandover(hmipv6cdsMN.currentMap(),
			  mipv6cdsMN->currentRouter(), dgram);
    Dout(dc::eh, mob->nodeName()<<" "<<nd->simTime()
	 <<" EHHopAlgorithm::hopAlgorithm sent bu to new MAP: "
	 <<hmipv6cdsMN.currentMap());
    
    delete dgram;
  }
}

void EHHopAlgorithm::boundMapChanged(const ipv6_addr& old_map)
{
  Dout(dc::eh, mob->nodeName()<<" "<<nd->simTime()<<" hop count reset was "<<hopCount
       <<" as bound map changed to "<<ehcds->boundMapAddr()<<" from "<<old_map);
  hopCount = 0;
  
}

}; //namespace EdgeHandover

