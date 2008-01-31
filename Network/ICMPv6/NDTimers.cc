// -*- C++ -*-
// Copyright (C) 2002, 2003, 2004 CTIE, Monash University
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
   @file NDTimers.cc

   @brief Implementation of classes that implement various timers required
   by Neigbhour Discovery as defined in RFC2461

   Author: Johnny Lai

   Date: 01.03.02
 */

#include "sys.h"
#include "debug.h"

#include "NDTimers.h"
#include "IPv6Datagram.h"
#include "cTimerMessage.h"
#include "NeighbourDiscovery.h"
#include "RoutingTable6.h"

using namespace IPv6NeighbourDiscovery;

//Needs a definition even though its pure virtual (for vtabl)
NDTimerBase::NDTimerBase(cTimerMessage* omsg)
  :msg(omsg)
{}

NDTimerBase::~NDTimerBase()
{}

NDTimer::NDTimer()
  :dgram(0), counter(0), max_sends(0), timeout(0), ifIndex(UINT_MAX)
{}

NDTimer::~NDTimer()
{
  //This is necessary when a timer message is cancelled and deleted as a
  //response arrived on time
  delete dgram;
}

RtrTimer::~RtrTimer()
{
  delete msg;
  msg = 0;
}

NDARTimer::NDARTimer()
  :targetAddr(IPv6_ADDR_UNSPECIFIED), dgram(0),
   counter(0), ifIndex(UINT_MAX-1)
{
}

NDARTimer::~NDARTimer()
{
  delete dgram;
  //This was never deleted before.  As we can set the ownership of
  //arg to false it is now safe to delete this without causing message to
  //delete arg which is NDARTimer and prevent endless recursion.
  delete msg;
  msg = 0;
}

///Create a duplicate timer with duplicated data members
NDARTimer* NDARTimer::dup(size_t ifIndex) const
{
  NDARTimer* tmrCopy = new NDARTimer;
  //msg is not shared.  Clients will have to create one for each duplicate
  tmrCopy->targetAddr = targetAddr;
  tmrCopy->dgram = dgram->dup();
  tmrCopy->ifIndex = ifIndex;
  return tmrCopy;
}


