// -*- C++ -*-
//
// Copyright (C) 2001, 2003 CTIE, Monash University
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
   @file NDStates.h
   @brief Definition of classes that handle Neigbhour Discovery
   Mechanism as defined in RFC2461

   @author Johnny Lai

   @date 24.9.01
 */

#if !defined NDSTATES_H
#define NDSTATES_H

#ifndef ICMPV6NDMESSAGE_H
#include "ICMPv6NDMessage.h"
#endif //ICMPV6NDMESSAGE_H

#ifndef MEMORY
#define MEMORY
//auto_ptr
#include <memory>
#endif //MEMORY

class ICMPv6Message;
class cMessage;
class NeighbourDiscovery;
class IPv6Datagram;
class IPv6Address;
class IPv6InterfaceData;

namespace IPv6NeighbourDiscovery
{
  class NDState;

/**
   @class NDState
   @brief Neighbour Discovery base class for State Pattern

   Provide the same interface to encapsulate different behaviour
   dependent on whether a node is a host or router.
*/

  class NDState
  {
  public:
    ///Processing of received ND messages
    virtual std::auto_ptr<ICMPv6Message> processMessage(std::auto_ptr<ICMPv6Message> ndm) = 0;
    static NDState* startND(NeighbourDiscovery* mod);
    NDState* changeState();
    virtual ~NDState(){};
    virtual void print(){};

  protected:

    NDState(NeighbourDiscovery* mod);

    virtual void enterState() = 0;
    virtual void leaveState() = 0;
    ///Factory Method: subclasses determine what next state is
//Define this later on if necessary
//    virtual NDState* createNextState() = 0;
    NeighbourDiscovery* nd;
    NDState* nextState;

  private:
    //Can't copy
    NDState(const NDState&);
    void operator=(const NDState&);
  };

  typedef IPv6NeighbourDiscovery::ICMPv6NDMRtrAd RA;
  typedef IPv6NeighbourDiscovery::ICMPv6NDMRtrSol RS;
  typedef IPv6NeighbourDiscovery::ICMPv6NDMNgbrAd NA;
  typedef IPv6NeighbourDiscovery::ICMPv6NDMNgbrSol NS;
  typedef IPv6NeighbourDiscovery::ICMPv6NDMRedirect Redirect;

}

#endif //NDSTATES_H
