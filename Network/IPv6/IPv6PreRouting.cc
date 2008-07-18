//
// Copyright (C) 2001, 2003, 2004 CTIE, Monash University
// Copyright (C) 2000 Institut fuer Telematik, Universitaet Karlsruhe
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
    @file IPv6PreRouting.cc
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

#include "IPv6PreRouting.h"

#include <boost/cast.hpp>



#include "IPv6Datagram.h"
#include "IPv6Forward.h" //routingInfoDisplay
#include "opp_utils.h"
#include "IPv6Utils.h"

Define_Module( IPv6PreRouting );

/*  ----------------------------------------------------------
        Public Functions
    ----------------------------------------------------------  */

void IPv6PreRouting::initialize()
{
    QueueBase::initialize();

    ctrIP6InReceive = 0;

    cModule* forward = OPP_Global::findModuleByName(this, "forwarding"); // XXX try to get rid of pointers to other modules --AV
    forwardMod = check_and_cast<IPv6Forward*>(forward);
}

/**
   Can make it check for Hop-By-Hop/Destination options at this point and
   see if we recognise them.  If we don't generate some error depending on
   first 2 bits of Option Type (IPv6 Spec RFC)

   Destination options are handled by IPv6LocalDeliver
 */
void IPv6PreRouting::endService(cMessage* msg)
{
  IPv6Datagram *datagram = check_and_cast<IPv6Datagram *>(msg);
  ctrIP6InReceive++;

  bool directionOut = false;
  IPv6Utils::printRoutingInfo(forwardMod->routingInfoDisplay, datagram, OPP_Global::nodeName(this), directionOut);

  send(datagram, "routingOut");
}


void IPv6PreRouting::finish()
{
  recordScalar("IP6InReceive", ctrIP6InReceive);
}
