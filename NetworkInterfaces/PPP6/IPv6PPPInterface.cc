//
// Copyright (C) 2001, 2004 Johnny Lai
// Monash University, Melbourne, Australia
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
    @file IPv6PPPInterface.cc
    @brief Implementation of PPPModule


*/

#include "sys.h"
#include "debug.h"

#include <omnetpp.h>
#include <memory> //auto_ptr
#include <iostream>
#include <assert.h>

#include "IPv6PPPInterface.h"
#include <string>
#include "PPP6Frame.h"
#include "opp_utils.h"
#include "InterfaceTableAccess.h"

#ifdef TESTIPv6
#undef NDEBUG
#include <cassert>
#endif

#include "LL6ControlInfo_m.h"


Define_Module_Like( IPv6PPPInterface, NetworkInterface6);

void IPv6PPPInterface::initialize()
{
  LinkLayerModule::initialize();

  setIface_name(PR_PPP);
  iface_type = PR_PPP;

  registerInterface();

  waitTmr = new cMessage("IPv6PPPInterfaceWait");
  curMessage = 0;
}


InterfaceEntry *IPv6PPPInterface::registerInterface()
{
  InterfaceEntry *e = new InterfaceEntry();

/*
  // interface name: NetworkInterface module's name without special characters ([])
  char *interfaceName = new char[strlen(parentModule()->fullName())+1];
  char *d=interfaceName;
  for (const char *s=parentModule()->fullName(); *s; s++)
      if (isalnum(*s))
          *d++ = *s;
  *d = '\0';

  e->setName(interfaceName);
  delete [] interfaceName;
*/
  std::string tmp = std::string("ppp")+OPP_Global::ltostr(parentModule()->index());
  e->setName(tmp.c_str()); // XXX HACK -- change back to above code!

  // port: index of gate where our "netwIn" is connected (in IP)
  int outputPort = parentModule()->gate("netwIn")->fromGate()->index();
  e->setOutputPort(outputPort);

  e->_linkMod = this; // XXX remove _linkMod on the long term!! --AV

  // generate a link-layer address to be used as interface token for IPv6
  InterfaceToken token(0, simulation.getUniqueNumber(), 64);
  e->setInterfaceToken(token);

  // and convert it to a string, for llAddrStr
  char buf[32];
  sprintf(buf, "%8.8dx:%8.8dx", token.normal(), token.low());
  e->setLLAddrStr(buf);

  // MTU: typical values are 576 (Internet de facto), 1500 (Ethernet-friendly),
  // 4000 (on some point-to-point links), 4470 (Cisco routers default, FDDI compatible)
  e->setMtu(4470);

  // capabilities
  e->setMulticast(true);
  e->setPointToPoint(true);

  // add
  InterfaceTable *ift = InterfaceTableAccess().get();
  ift->addInterface(e);

  printf("DBG: %s as '%s' on %d\n", fullPath().c_str(), e->name(), outputPort);

  return e;
}

/*
  @note Implementation note

  This activity to handleMessage conversion is correct in the sense that the
  output packets are sent at the same time as their activity counterparts
  however the creation times of those messages is not the same as the activity
  since they are created after the wait and not before. Since we have not used
  the creation of message times in any output variable calculations this is a
  valid conversion.

  To maintain invariance in these creation times requires moving the
  isSelfMessage into the subbranch if (!waitTmr.isScheduled() && curMessage ==
  0) of !isSelfMessage and additional flags (well can actually use the
  ipdatagram pointer for this) to show whether packet arrived from network or
  upper layer so that in the selfMessage branch you could send the correct one.
  That approach was not chosen because if support for queued packets then would
  require the action of creating the packets to be duplicated in the
  isSelfMessage branch as well as the subbranch if (!waitTmr->isScheduled() &&
  curMessage == 0) of !isSelfMessage branch.
 */
void IPv6PPPInterface::handleMessage(cMessage* theMsg)
{
  if (!theMsg->isSelfMessage())
  {
    if (!waitTmr->isScheduled() && curMessage == 0)
    {
      scheduleAt(delay + simTime(), waitTmr);
      curMessage = theMsg;
      return;
    }
    else if (waitTmr->isScheduled())
    {
      std::cerr<<fullPath()<<" "<<simTime()<<" received new mesage "
          <<" when message was scheduled at waitTmr="<<waitTmr->arrivalTime()
          <<endl;
      Dout(dc::custom, fullPath()<<" "<<simTime()<<" received new mesage "
           <<" when message was scheduled at waitTmr="<<waitTmr->arrivalTime());
      delete theMsg;
      return;
    }
    assert(false);
    delete theMsg;
    return;
  }

  cMessage* msg = curMessage;
  curMessage = 0;

  if (!strcmp(msg->arrivalGate()->name(), "netwIn"))
  {
    PPP6Frame* outFrame = receiveFromUpperLayer(msg);
    send(outFrame, "physOut");
    //XXX cMessage *nwiIdleMsg = new cMessage();
    //XXX nwiIdleMsg->setKind(NWI_IDLE);
    //XXX send(nwiIdleMsg, "ipOutputQueueOut");
    return;
  }

  ++cntReceivedPackets;
  //from Network peer
  std::auto_ptr<PPP6Frame> recFrame(check_and_cast<PPP6Frame*>(msg));
  assert(recFrame.get());

  if (recFrame->hasBitError())
  {
    ev << "\n+ PPPLink of " << fullPath()
       << " receive error: Error in transmission.\n";
    Dout(dc::debug, fullPath()<<" "<<msg->arrivalTime()
         <<" PPPLink receive error: bit error in transmission.");
    return;
  }

  sendToUpperLayer(recFrame.get());
}

PPP6Frame* IPv6PPPInterface::receiveFromUpperLayer(cMessage* msg) const
{
  //from network layer

  // encapsulate IP datagram in PPP frame
  PPP6Frame* outFrame = new PPP6Frame();

  //cMessage *dupMsg = recPkt->data().dgram;
  LL6ControlInfo *ctrlInfo = check_and_cast<LL6ControlInfo*>(msg->removeControlInfo());
  outFrame->destAddr = ctrlInfo->getDestLLAddr();
  delete ctrlInfo;

  outFrame->encapsulate(msg);
  outFrame->setProtocol(PPP_PROT_IP); //XXX why hardcode? value from ctrl info? --AV
  outFrame->setName(msg->name());
  return outFrame;
}

void IPv6PPPInterface::sendToUpperLayer(PPP6Frame* recFrame)
{
  cMessage *packet = recFrame->decapsulate();
  send(packet, "netwOut");
}

