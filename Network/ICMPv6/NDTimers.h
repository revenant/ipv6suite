// -*- C++ -*-
// Copyright (C) 2002, 2004 CTIE, Monash University
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
   @file NDTimers.h

   @brief Definition of classes that to implement various timers required by
   Neigbhour Discovery as defined in RFC2461

   Author: Johnny Lai

   Date: 01.03.02
 */

#if !defined NDTIMERS_H
#define NDTIMERS_H

#ifndef BOOST_UTILITY_HPP
#include <boost/utility.hpp>
#endif //BOOST_UTILITY_HPP

#include <omnetpp.h>

#ifndef IPv6_ADDR_H
#include "ipv6_addr.h"
#endif //IPv6_ADDR_H

#include "IPv6Address.h"

class cTimerMessage;
class IPv6Datagram;
class RoutingTable6;


namespace IPv6NeighbourDiscovery
{
  /**
     @class NDTimerBase
     @brief Abstract base class for all timer objs
  */
  class NDTimerBase: public boost::noncopyable
  {
  public:
    cTimerMessage* msg;
  protected:
    NDTimerBase(cTimerMessage* omsg = 0);
    virtual ~NDTimerBase() = 0;
  };

  /**
     @class NDTimer
     @brief    General data for timer messages
  */
  class NDTimer: public NDTimerBase
  {
  public:
    NDTimer();
    ///Buffered datagram used for retries
    IPv6Datagram* dgram;
    int counter;
    int max_sends;
    double timeout;
    size_t ifIndex;
    IPv6Address tentativeAddr;
    ~NDTimer();
  };

  /**
     @class RtrTimer
     @brief  Contains state information for use in generating router adv.
  */
  class RtrTimer: public NDTimerBase
  {
  public:
    ///ifIndex of advertising interface
    size_t ifIndex;
    size_t noInitAds;
    ///Guess these two variables below are constants so don't need to store
    //separate ones for each interface.
    size_t maxInitAds;
    double maxInitRtrInterval;
    ~RtrTimer();
  };

  class NDARTimer;
  /**
     @class NDARTimer
     @brief Encapsulates Address Resolution Neighbour Solicitation retries
  */
  class NDARTimer: public NDTimerBase
  {
  public:
    NDARTimer();
    ~NDARTimer();
    NDARTimer* dup(size_t ifIndex) const;
    ipv6_addr targetAddr;
    IPv6Datagram* dgram;
    size_t counter;
    //int max_sends;
    //double timeout;
    //If this is UINT_MAX then that means addr res is trying on all ifaces
    //for the LL addr of the targetAddr (on multihomed hosts)
    size_t ifIndex;
  };
}

#endif //NDTIMERS_H
