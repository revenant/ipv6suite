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

#include "IPv6PPPInterface.h"
#include <string>


#include "PPP6Frame.h"
#include "opp_utils.h"
#include "IPv6InterfaceData.h"
#include "Messages.h"
//#include "IPDatagram.h"
//#ifdef CWDEBUG
//#include "IPv6Datagram.h"
//#endif //CWDEBUG

#ifdef TESTIPv6
#undef NDEBUG
#include <cassert>
//#include "IPv6Datagram.h"
#endif

#include "LLInterfacePkt.h"


Define_Module_Like( IPv6PPPInterface, NetworkInterface6);

void IPv6PPPInterface::initialize()
{
  LinkLayerModule::initialize();

  setIface_name(PR_PPP);
  iface_type = PR_PPP;

  interfaceID[0] = OPP_Global::generateInterfaceId();
  interfaceID[1] = OPP_Global::generateInterfaceId();

  waitTmr = new cMessage("IPv6PPPInterfaceWait");
  curMessage = 0;
}

unsigned int IPv6PPPInterface::lowInterfaceId()
{
  return interfaceID[1];
}

unsigned int IPv6PPPInterface::highInterfaceId()
{
  return interfaceID[0];
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
      cerr<<fullPath()<<" "<<simTime()<<" received new mesage "
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

  if (!strcmp(msg->arrivalGate()->name(), "ipOutputQueueIn"))
  {
    PPP6Frame* outFrame = receiveFromUpperLayer(msg);
    send(outFrame, "physicalOut");
    cMessage *nwiIdleMsg = new cMessage();
    nwiIdleMsg->setKind(NWI_IDLE);
    send(nwiIdleMsg, "ipOutputQueueOut");
    return;
  }

  ++cntReceivedPackets;
  //from Network peer
  std::auto_ptr<PPP6Frame> recFrame(check_and_cast<PPP6Frame*>(msg));
  assert(recFrame.get());

/* XXX why's this check? we won't support anything but IP? --AV
  if (recFrame->protocol() != PPP_PROT_IP)
  {
    ev << "\n+ PPPLink of " << fullPath()
       << " receive error: Not IP protocol.\n";
    Dout(dc::debug, fullPath()<<" "<<msg->arrivalTime()
         <<" PPPLink receive error: unknown protocol received (should be PPP_PROT_IP)");
    return;
  }
*/

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

  LLInterfacePkt* recPkt = check_and_cast<LLInterfacePkt*>(msg);

  //Hack dest LL addr required when we use PPP in access point otherwise packets
  //don't know which MN to go to. If we used broadcast then all MN would process
  //packet at upper layers which is not efficient and is not realistic.
  outFrame->destAddr = recPkt->data().destLLAddr;
  Debug( assert(check_and_cast<IPv6Datagram*>(recPkt->data().dgram)!=0); );

/* XXX next two lines replaced with one --AV
  cMessage *dupMsg = recPkt->data().dgram->dup();
  delete recPkt->data().dgram;
*/
  cMessage *dupMsg = recPkt->data().dgram;

  dupMsg->setLength(dupMsg->length() * 8); // convert from bytes to bits   XXX ??? --AV
  outFrame->encapsulate(dupMsg);
  outFrame->setProtocol(PPP_PROT_IP); //XXX wrong! value from InterfacePkt? --AV
  outFrame->setName(dupMsg->name());
  delete recPkt;
  return outFrame;
  //wait(delay);
}

void IPv6PPPInterface::sendToUpperLayer(PPP6Frame* recFrame)
{
/* XXX why force IPDatagram? can be anything --AV
 IPDatagram *ipdatagram =
    check_and_cast<IPDatagram*> (recFrame->decapsulate());
  ipdatagram->setLength(ipdatagram->length() / 8); // convert from bits back to bytes
  assert(ipdatagram);
  //wait(delay);
  send(ipdatagram, "ipInputQueueOut");
*/
  cMessage *packet = recFrame->decapsulate();
  packet->setLength(packet->length() / 8); // convert from bits back to bytes  XXX why???? --AV
  send(packet, "ipInputQueueOut");
}

/* XXX activity() code below was apparently not used at all, removed --AV
*/
