//
// Copyright (C) 2001, 2003, 2004 CTIE, Monash University
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

/**
    @file IPv6Output.cc
    @brief Implementation for IPOutput core module

    Responsibilities:
    receive complete datagram from IPFragmentation
    hop counter check
    -> throw away and notify ICMP if ttl==0
    otherwise  send it on to output queue

    @author Johnny Lai
    based on IPOutputCore by Jochen Reber
*/

#include "sys.h"
#include "debug.h"

#ifdef CWDEBUG
#undef NDEBUG
#include <cassert>
#include "ICMPv6Message.h"
#endif //CWDEBUG

#include <omnetpp.h>
#include <iomanip>
#include <string>


#include "IPv6Output.h"
#include "IPv6Datagram.h"
#include "Messages.h"
#include "IPv6Multicast.h" //multicastAddr()
#include "RoutingTable6.h"
#include "IPv6Forward.h"
#include "opp_utils.h"
#include "IPv6CDS.h"
#include "NDEntry.h"
#include "LL6ControlInfo_m.h"
#include "IPv6Utils.h"

using namespace std;


Define_Module (IPv6Output);

void IPv6Output::initialize()
{
    QueueBase::initialize();

    rt = RoutingTable6Access().get();

    ctrIP6OutForwDatagrams = 0;
    ctrIP6OutMcastPkts = 0;

    parentModule()->displayString().setTagArg("q",0,"queue");
    displayString().setTagArg("q",0,"queue");

    cModule* forward = OPP_Global::findModuleByName(this, "forwarding"); // XXX try to get rid of pointers to other modules --AV
    forwardMod = check_and_cast<IPv6Forward*>(forward);
}

void IPv6Output::endService(cMessage* msg)
{
  IPv6Datagram *datagram = check_and_cast<IPv6Datagram*>(msg);

  static const string addrReslnIn("addrReslnIn");
  static const string neighbourDiscoveryIn("neighbourDiscoveryIn");

  if (string(datagram->arrivalGate()->name()) == string("mobilityIn"))
  {
    boost::weak_ptr<RouterEntry> re = rt->cds->defaultRouter();
    assert(re.lock().get());

    LL6ControlInfo *ctrlInfo = new LL6ControlInfo();
    ctrlInfo->setDestLLAddr(re.lock().get()->linkLayerAddr().c_str());
    datagram->setControlInfo(ctrlInfo);
  }
  else if (addrReslnIn == datagram->arrivalGate()->name() ||
           neighbourDiscoveryIn == datagram->arrivalGate()->name())
  {
    //Perhaps these should be sent to the multicast module with ifIdx and
    //let multicast handle how to talk to link layer instead of replicating
    //that here too.

    //Receive the IPv6Datagram directly as don't know what the target
    //LL addr is and want to specify which iface to send on

#ifdef CWDEBUG
    ICMPv6Message* icmpMsg = check_and_cast<ICMPv6Message*>(datagram->encapsulatedMsg());
    assert(icmpMsg->type() >= 0 && icmpMsg->type() < 138);
#endif //CWDEBUG

    LL6ControlInfo *ctrlInfo = new LL6ControlInfo();
    ctrlInfo->setDestLLAddr(IPv6Multicast::multicastLLAddr(datagram->destAddress()).c_str());
    datagram->setControlInfo(ctrlInfo);
  }
  else
  {
    // XXX when do we get here? --AV
    // must already contain link layer control info
    assert(check_and_cast<LL6ControlInfo*>(datagram->controlInfo()));
  }

  if (datagram->inputPort() != -1)
    ctrIP6OutForwDatagrams++;

  if (datagram->destAddress().isMulticast())
    ctrIP6OutMcastPkts++;

  bool directionOut = true;
  IPv6Utils::printRoutingInfo(forwardMod->routingInfoDisplay, datagram, rt->nodeName(), directionOut);

  send(datagram, "queueOut");
}

void IPv6Output::finish()
{
  recordScalar("IP6OutForwDatagrams", ctrIP6OutForwDatagrams);
  recordScalar("IP6OutMcastPkts", ctrIP6OutMcastPkts);
}


