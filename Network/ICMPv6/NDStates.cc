//
// Copyright (C) 2001, 2004 CTIE, Monash University
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
   @file NDStates.cc

   @brief Definition of classes that handle Neigbhour Discovery Mechanism as
   defined in RFC2461

   @author Johnny Lai

   @date 14.10.01
 */

#include <climits>
#include "NDStates.h"
#include "config.h"
#include "NDStateHost.h"
#include "NDStateRouter.h"

#include "NeighbourDiscovery.h"
#include "IPv6Datagram.h"
#include "RoutingTable6.h"

#ifdef USE_MOBILITY
#include "MIPv6NDStateHost.h"
#include "MIPv6NDStateRouter.h"
#ifdef USE_HMIP
#include "HMIPv6NDStateHost.h"
#include "HMIPv6NDStateRouter.h"
#if EDGEHANDOVER
#include "EHNDStateHost.h"
#include "opp_utils.h"
#include "IPv6Mobility.h"
#endif //EDGEHANDOVER
#endif //USE_HMIP
#endif //USE_MOBILITY

using namespace IPv6NeighbourDiscovery;

NDState* NDState::startND(NeighbourDiscovery* mod)
{
  NDState* state = 0;

  assert(mod->rt != 0);
  if (mod->rt == 0)
  {
    cerr << "Cannot find IPv6 RoutingTable module"<<__FILE__<<":"<<__LINE__<<endl;
    abort_ipv6suite();
  }

#if defined USE_MOBILITY
  if (mod->rt->mobilitySupport())
#ifdef USE_HMIP
    if (!mod->rt->hmipSupport())
#endif //USE_HMIP
    {
      if (mod->rt->isRouter())
        state =  new MobileIPv6::MIPv6NDStateRouter(mod);
      else if ( mod->rt->isMobileNode())
        state = new MobileIPv6::MIPv6NDStateHost(mod);
      else //normal CN should return NDStateHost
        state = new NDStateHost(mod);
      return state;
    }
#ifdef USE_HMIP
    else
    {
      if (mod->rt->isMobileNode())
      {
#if EDGEHANDOVER
        IPv6Mobility* mob = check_and_cast<IPv6Mobility*>
          (OPP_Global::findModuleByType(mod->rt, "IPv6Mobility")); // XXX try to get rid of pointers to other modules --AV
        assert(mob);
        if (mob->edgeHandover())
          state = EdgeHandover::EHNDStateHost::create(mod, mob->edgeHandoverType());
        else
#endif
        state = new HierarchicalMIPv6::HMIPv6NDStateHost(mod);
      }
      else if (mod->rt->isRouter())
        state = new HierarchicalMIPv6::HMIPv6NDStateRouter(mod);
      else
        state = new NDStateHost(mod);
      return state;
    }
#endif //USE_HMIP
#endif //USE_MOBILITY

  if (mod->rt->isRouter())
    state = new NDStateRouter(mod);
  else
    state = new NDStateHost(mod);

  return state;
}

NDState::NDState(NeighbourDiscovery* mod)
  :nd(mod), nextState(0)
{}

NDState* NDState::changeState()
{
  if (!nextState)
  {
    //Create the next state
//      createNextState();
  }

  leaveState();
  nextState->enterState();
  return nextState;
}
