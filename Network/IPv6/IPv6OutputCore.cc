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
    @file IPv6OutputCore.cc
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


#include "IPv6OutputCore.h"
#include "IPv6Datagram.h"
#include "Messages.h"
#include "IPv6Multicast.h" //multicastAddr()
#include "cTTimerMessageCB.h" //ZERO_WAIT_DELAY
#include "RoutingTable6.h"
#include "IPv6ForwardCore.h"
#include "opp_utils.h"
#include "IPv6CDS.h"
#include "NDEntry.h"
#include "LL6ControlInfo_m.h"
#include "IPv6Utils.h"

using namespace std;


Define_Module ( IPv6OutputCore );

/*  ----------------------------------------------------------
        Public Functions
    ----------------------------------------------------------  */

void IPv6OutputCore::initialize()
{
  RoutingTable6Access::initialize();

    delay = par("procdelay");
    if (delay == 0)
      delay = ZERO_WAIT_DELAY;
    //XXX hasHook = (findGate("netfilterOut") != -1);
    ctrIP6OutForwDatagrams = 0;
    ctrIP6OutMcastPkts = 0;
    curPacket = 0;
    waitTmr = new cMessage("IPv6OutputCoreWait");
    waitQueue.setName("IPv6OutputWaitQ");

/* XXX replaced with shorter version below
    //Can't really see from proc module
    std::string display(parentModule()->displayString());
    display += ";q=IPv6OutputWaitQ";
    parentModule()->setDisplayString(display.c_str());

    //so display at self
    display = static_cast<const char*>(displayString());
    display += ";q=IPv6OutputWaitQ";
    setDisplayString(display.c_str());
*/
    parentModule()->displayString().setTagArg("q",0,"IPv6OutputWaitQ");
    displayString().setTagArg("q",0,"IPv6OutputWaitQ");

    cModule* forward = OPP_Global::findModuleByName(this, "forwarding");
    assert(forward);
    forwardMod = check_and_cast<IPv6ForwardCore*>
      (forward->submodule("core"));
    assert(forwardMod != 0);
}

void IPv6OutputCore::handleMessage(cMessage* msg)
{
  //XXX LLInterfacePkt* llpkt = 0;
  if (!msg->isSelfMessage())
  {
    if (!waitTmr->isScheduled() && curPacket == 0 )
    {
      curPacket = check_and_cast<IPv6Datagram*>(msg);
      processArrivingMessage(curPacket);
      scheduleAt(delay+simTime(), waitTmr);
      //XXX curPacket = llpkt;
      return;
    }
    else if (waitTmr->isScheduled())
    {
//      cerr<<fullPath()<<" "<<simTime()<<" received new packet "
//          <<"when previous packet was scheduled at waitTmr="<<waitTmr->arrivalTime()
//          <<endl;
      Dout(dc::custom|flush_cf, fullPath()<<" "<<simTime()<<" received new packet "
           <<"when previous packet was scheduled at waitTmr="<<waitTmr->arrivalTime()
           <<"delay "<<setprecision(8)<<delay);
      waitQueue.insert(msg);
      return;
    }
    assert(false);
  }

  // timer expired: send out curPacket
  assert(curPacket);
  //XXX llpkt = curPacket;
  // TODO: TEMPARORY FIX FOR ENQUEUE
  //XXX llpkt->setKind(llpkt->data().dgram->kind());

  // XXX This needs to be done in llpkt itself as take is a protected function,
  //except that's a template class.  We do not know the type of the template
  //parameter can contain members that are cobjects. Guess cTypedMessage needs
  //refactoring too

  //llpkt->take(llpkt->data().dgram);

  send(curPacket, "queueOut");

  // start processing next packet if queue not empty
  if (waitQueue.empty())
    curPacket = 0;
  else
  {
    curPacket = check_and_cast<IPv6Datagram*>(waitQueue.pop());
    processArrivingMessage(curPacket);
    scheduleAt(delay + simTime(), waitTmr);
  }
}

void IPv6OutputCore::processArrivingMessage(IPv6Datagram* datagram)
{
  static const string addrReslnIn("addrReslnIn");
  static const string neighbourDiscoveryIn("neighbourDiscoveryIn");

  //LLInterfacePkt* llpkt = 0;
  //IPv6Datagram* datagram = 0;

  if (string(datagram->arrivalGate()->name()) == string("mobilityIn")) //XXX IPv6Datagram from Mobility??? --AV
  {
    //XXX datagram = check_and_cast<IPv6Datagram*> (msg);
    //XXX assert(datagram != 0);

    //XXX FIXME why newly find RoutingTable6 every time ?? --AV
    RoutingTable6* rt =  check_and_cast<RoutingTable6*>(OPP_Global::findModuleByType(this, "RoutingTable6"));
    assert(rt);
    boost::weak_ptr<RouterEntry> re = rt->cds->defaultRouter(); // XXX why this weak_ptr stuff here? don't understand... --AV
    assert(re.lock().get());

    /* next lines replaced with control info stuff --AV
    LLInterfaceInfo info = { datagram, re.lock().get()->linkLayerAddr() };
    llpkt = new LLInterfacePkt(info);
    llpkt->setName(info.dgram->name());
    */
    //XXX TBD create utility function add6LLControlInfo()!
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

    //XXX datagram = check_and_cast<IPv6Datagram*> (msg);
    //XXX assert(datagram != 0);

#ifdef CWDEBUG
    ICMPv6Message* icmpMsg = check_and_cast<ICMPv6Message*> (datagram->encapsulatedMsg());
    assert(icmpMsg != 0);

    assert(icmpMsg->type() >= 0 && icmpMsg->type() < 138);
    //Dout( dc::debug, "type is "<<(int)icmpMsg->type());
#endif //CWDEBUG

    /* XXX next lines replaced with control info --AV
    LLInterfaceInfo info = { datagram,
                             IPv6Multicast::multicastLLAddr(datagram->destAddress())};

    llpkt = new LLInterfacePkt(info);
    llpkt->setName(info.dgram->name());
    */
    LL6ControlInfo *ctrlInfo = new LL6ControlInfo();
    ctrlInfo->setDestLLAddr(IPv6Multicast::multicastLLAddr(datagram->destAddress()).c_str());
    datagram->setControlInfo(ctrlInfo);

  }
  else
  {
    // XXX when do we get here? --AV
    /* looks like this is not needed --AV
    llpkt = check_and_cast<LLInterfacePkt*> (msg);
    assert(llpkt);
    datagram = check_and_cast<IPv6Datagram*> (llpkt->data().dgram);
    */

    // must already contain link layer control info
    assert(check_and_cast<LL6ControlInfo*>(datagram->controlInfo()));

    //Need to dup datagram as Omnet++ only allows you to send a message
    //twice (from routing to frag to this module).  Got error dialog in TCL
    //about mesage is owned by route.core.locals and trying to send that
    //message from this module is not allowed. Is this example of garbage
    //collector kicking in?

// XXX well, actually: you sent over the pointer of the message from
// route.core, but the message object stayed there!!
// How should poor OMNeT++ know you wanted the message itself to travel
// as well...? --AV
/*
    datagram = datagram->dup();
    delete llpkt->data().dgram;
    llpkt->data().dgram = datagram;
*/
  }

  if (datagram->inputPort() != -1)
    ctrIP6OutForwDatagrams++;

  if (datagram->destAddress().isMulticast())
    ctrIP6OutMcastPkts++;

  bool directionOut = true;
  IPv6Utils::printRoutingInfo(forwardMod->routingInfoDisplay, datagram, rt->nodeName(), directionOut);

  //XXX return llpkt;
}

/* XXX remove --AV
void IPv6OutputCore::activity()
{
  static const string addrReslnIn("addrReslnIn");
  static const string neighbourDiscoveryIn("neighbourDiscoveryIn");

    cMessage *dfmsg = 0;
    IPv6Datagram* datagram = 0;
    LLInterfacePkt* llpkt = 0;
    cMessage* msg = 0;

    while(true)
    {
      msg = receive();

      if (addrReslnIn == msg->arrivalGate()->name() ||
          neighbourDiscoveryIn == msg->arrivalGate()->name())
      {
        //Perhaps these should be sent to the multicast module with ifIdx and
        //let multicast handle how to talk to link layer instead of replicating
        //that here too.

        //Receive the IPv6Datagram directly as don't know what the target
        //LL addr is and want to specify which iface to send on

        datagram = check_and_cast<IPv6Datagram*> (msg);
#ifdef TESTIPv6
        ICMPv6Message* icmpMsg = check_and_cast<ICMPv6Message*> (datagram->encapsulatedMsg());
        assert(icmpMsg != 0);

        assert(icmpMsg->type() >= 0 && icmpMsg->type() < 138);
        //Dout( dc::debug, "type is "<<(int)icmpMsg->type());
#endif //TESTIPv6

        LLInterfaceInfo info = { datagram,
                                 IPv6Multicast::multicastLLAddr(datagram->destAddress())};

        llpkt = new LLInterfacePkt(info);
        llpkt->setName(info.dgram->name());
      }
      else
      {

        llpkt = check_and_cast<LLInterfacePkt*> (msg);
        datagram = check_and_cast<IPv6Datagram*> (llpkt->data().dgram);

        //Need to dup datagram as Omnet++ only allows you to send a message
        //twice (from routing to frag to this module).  Got error dialog in TCL
        //about mesage is owned by route.core.locals and trying to send that
        //message from this module is not allowed. Is this example of garbage
        //collector kicking in?
        datagram = datagram->dup();
        delete llpkt->data().dgram;
        llpkt->data().dgram = datagram;

      }

      if (datagram->inputPort() != -1)
        ctrIP6OutForwDatagrams++;

      if (datagram->destAddress().isMulticast())
        ctrIP6OutMcastPkts++;

      // pass Datagram through netfilter if it exists
      if (hasHook)
      {
        send(datagram, "netfilterOut");
        dfmsg = receiveNewOn("netfilterIn");
        if (dfmsg->kind() == DISCARD_PACKET)
        {
          delete dfmsg;
          delete llpkt;

          continue;
        }
        datagram = check_and_cast<IPv6Datagram *>(dfmsg);
      }

      wait(delay);

      // TODO: TEMPARORY FIX FOR ENQUEUE
      llpkt->setKind(datagram->kind());
      //llpkt->data().dgram->setOwner(llpkt);
      send(llpkt, "queueOut");



    }
}
*/
void IPv6OutputCore::finish()
{
  recordScalar("IP6OutForwDatagrams", ctrIP6OutForwDatagrams);
  recordScalar("IP6OutMcastPkts", ctrIP6OutMcastPkts);
}
