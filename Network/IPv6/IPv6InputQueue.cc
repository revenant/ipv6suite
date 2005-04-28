//
// Copyright (C) 2001, 2003 CTIE, Monash University
// Copyright (C) 2000 Institut fuer Telematik, Universitaet Karlsruhe
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

/*
	file: IPv6InputQueue.cc
	Purpose: Implementation of L2 IPv6InputQueue
	Responsibilities:
	author: Johnny Lai
    based on Jochen Reber
*/

#include <omnetpp.h>

#include <boost/cast.hpp>
#include <iostream>
#include <string>

#include "IPv6InputQueue.h"
#include "IPv6Datagram.h"
#include "opp_utils.h"

Define_Module( IPv6InputQueue );


void IPv6InputQueue::endService(cMessage* msg)
{
  IPv6Datagram *datagram = check_and_cast<IPv6Datagram*>(msg);
  datagram->setInputPort(datagram->arrivalGate()->index());
  send(datagram, "toIP");
}


