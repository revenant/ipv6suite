//
// Copyright (C) 2001, 2003, 2004 CTIE, Monash University
// Copyright (C) 2000 Institut fuer Telematik, Universitaet Karlsruhe
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
    @file PreRouting6Core.cc
    @brief Implementation of PreRouting
    -Responsibilities:
        receive IP datagram
        send correct datagram to Forwarding Module
    @author Johnny Lai
    @date 27/08/01
    Based on PreRoutingCore module by Jochen Reber
*/

#include "sys.h"
#include "debug.h"

#include "PreRouting6Core.h"

#include <boost/cast.hpp>



#include "IPv6Datagram.h"
#include "IPv6ForwardCore.h" //routingInfoDisplay
#include "opp_utils.h"

Define_Module( PreRouting6Core );

/*  ----------------------------------------------------------
        Public Functions
    ----------------------------------------------------------  */

void PreRouting6Core::initialize()
{
    delay = par("procdelay");
    hasHook = (findGate("netfilterOut") != -1);
    ctrIP6InReceive = 0;
    waitTmr = new cMessage("PreRouting6CoreWait");
    curPacket = 0;

    cModule* forward = OPP_Global::findModuleByName(this, "forwarding");
    forwardMod = boost::polymorphic_downcast<IPv6ForwardCore*>
      (forward->submodule("core"));
    assert(forwardMod != 0);

}

/**
   Can make it check for Hop-By-Hop/Destination options at this point and
   see if we recognise them.  If we don't generate some error depending on
   first 2 bits of Option Type (IPv6 Spec RFC)

   Destination options are handled by LocalDeliver6Core

 */
void PreRouting6Core::handleMessage(cMessage* msg)
{

  if (!msg->isSelfMessage())
  {
    IPv6Datagram *datagram = boost::polymorphic_downcast<IPv6Datagram *>(msg);
    assert(datagram != 0);

    ctrIP6InReceive++;

    bool directionOut = false;
    OPP_Global::printRoutingInfo(forwardMod->routingInfoDisplay, datagram, OPP_Global::nodeName(this), directionOut);

    if (!waitTmr->isScheduled() && curPacket == 0 )
    {
      scheduleAt(delay+simTime(), waitTmr);
      curPacket = datagram;
      return;
    }
    else if (waitTmr->isScheduled())
    {
      Dout(dc::custom, fullPath()<<" "<<simTime()<<" received new packet "<<*datagram
           <<" when previous packet was scheduled at waitTmr="<<waitTmr->arrivalTime());
      waitQueue.insert(msg);
      return;
    }
    assert(false);
  }


/*
    //No header checksums for IPv6 only ICMPv6
    // check for header biterror
    //        if (datagram->hasBitError())
    //        {
    //     probability of bit error in header =
    //    size of header / size of total message
    relativeHeaderLength =
      datagram->headerLength() / datagram->totalLength();
    if (dblrand() <= relativeHeaderLength)
    {
      sendErrorMessage(datagram, ICMP_PARAMETER_PROBLEM, 0);
      continue;
    }
  }
*/

  assert(curPacket);

  send(curPacket, "routingOut");

  if (waitQueue.empty())
    curPacket = 0;
  else
  {
    curPacket = boost::polymorphic_downcast<cMessage*>(waitQueue.pop());
    scheduleAt(delay + simTime(), waitTmr);
  }

}


void PreRouting6Core::finish()
{
  recordScalar("IP6InReceive", ctrIP6InReceive);
  delete waitTmr;
  waitTmr = 0;
  delete curPacket;
  curPacket = 0;
}
