// $Header: /home/cvs/IPv6Suite/IPv6SuiteWithINET/Network/IPv6/IPv6InputQueue.cc,v 1.1 2005/02/22 07:13:27 andras Exp $
//
// Copyright (C) 2001, 2003 CTIE, Monash University
// Copyright (C) 2000 Institut fuer Telematik, Universitaet Karlsruhe
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


void IPv6InputQueue::initialize()
{
  delay = par("procDelay");
  datagram = 0;
  waitTmr = new cMessage("InputQueueWait");
}

void IPv6InputQueue::handleMessage(cMessage* msg)
{
  if (!msg->isSelfMessage())
  {
    if (!waitTmr->isScheduled() && datagram == 0 )
    {
      scheduleAt(delay+simTime(), waitTmr);
      if (!(msg->className() == std::string("IPv6Datagram")))
      {
          std::cerr<<"What is msg"<< msg<<" name="<<msg->name()<<" class="<<msg->className()<< " nodename="<<OPP_Global::nodeName(this)<< " kind="<<msg->kind()<<" prio="<<msg->priority()<<" encap="<<msg->encapsulatedMsg() <<" senderModuleId="<<msg->senderModuleId()<<std::endl;
          cPacket* pkt = boost::polymorphic_downcast<cPacket*>(msg);
          std::cerr<<" prot="<<pkt->protocol()<<" pdu="<<pkt->pdu();
      }
      datagram = check_and_cast<IPv6Datagram*>(msg);
      assert(datagram);
      return;
    }
    else if (waitTmr->isScheduled())
    {
      std::cerr<<fullPath()<<" "<<simTime()<<" received new packet "
               <<" when previous packet was scheduled at waitTmr="<<waitTmr->arrivalTime();
      (boost::polymorphic_downcast<IPv6Datagram*> (msg))->writeTo(std::cerr);
      std::cerr<<std::endl;
      delete msg;
      return;
    }
    assert(false);
  }

  assert(datagram);
  //assuming gate fromNW is declared first in list of in gates for IPv6InputQueue
  //ned module
  datagram->setInputPort(datagram->arrivalGate()->index());

  send(datagram, "toIP");
  datagram = 0;
}


