// Copyright (C) 2001, 2004 Monash University, Melbourne, Australia
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
    @file EtherModuleAP.cc
    @brief Definition file for EtherModuleAP

    simple implementation of ethernet interface in AP

    @author Eric Wu
*/

#include <sys.h> // Dout
#include "debug.h" // Dout

#include "EtherModuleAP.h"

#include <string>



#include "opp_utils.h"
#include "ethernet.h"
#include "PHYSimple.h"
#include "EtherSignal.h"
#include "EtherFrame.h"
#include "EtherStateIdle.h"

#include "MACAddress.h"

#ifdef USE_MOBILITY
#include "WirelessEtherBridge.h"
#include <memory>
#endif

Define_Module_Like( EtherModuleAP, NetworkInterface );

void EtherModuleAP::initialize(int stage)
{
  if (stage == 0)
  {
    frameCollided = false;

    LinkLayerModule::initialize();
    setIface_name(PR_ETHERNET);
    iface_type = PR_ETHERNET;

    changeState(EtherStateIdle::instance());

    backoffRemainingTime = 0;

    _myAddr.set(MAC_ADDRESS_UNSPECIFIED_STRUCT);

    inputFrame = 0;
    retry = 0;
    interframeGap = 0;

    inGate = findGate("physicalIn");
    outGate = findGate("physicalOut");
  }
  else if (stage == 1)
  {
    // needed for AP bridge
    cMessage* protocolNotifier = new cMessage("PROTOCOL_NOTIFIER");
    protocolNotifier->setKind(MK_PACKET);
    send(protocolNotifier, inputQueueOutGate());
  }
}

void EtherModuleAP::handleMessage(cMessage* msg)
{
  if ((MAC_address)_myAddr == MAC_ADDRESS_UNSPECIFIED_STRUCT)
  {
    // needed for AP bridge
    if ( std::string(msg->name()) == "WIRELESS_AP_NOTIFY_MAC")
    {
      _myAddr.set(static_cast<cPar*>(msg->parList().get(0))->stringValue());

      idleNetworkInterface();
    }

    delete msg;
    return;
  }

  EtherModule::handleMessage(msg);
}

void EtherModuleAP::finish(void)
{}

bool EtherModuleAP::receiveData(std::auto_ptr<cMessage> msg)
{
  // the frame should have already been created in the bridge module
  EtherSignalData* frame =
    boost::polymorphic_downcast<EtherSignalData *>(msg.get()->decapsulate());
  assert(frame);

  frame->setSrcModPathName(fullPath());

  outputBuffer.push_back(frame);

  if ( _currentState == EtherStateIdle::instance())
  {
    cTimerMessage* tmrMessage = getTmrMessage(SELF_INTERFRAMEGAP);
    if (!(tmrMessage && tmrMessage->isScheduled()))
      static_cast<EtherStateIdle*>(_currentState)->chkOutputBuffer(this);
  }

  return true;
}

bool EtherModuleAP::sendData(EtherFrame* frame)
{
  assert(frame);

  send(frame->dup(), inputQueueOutGate());

  return true;
}

void EtherModuleAP::addMacEntry(std::string addr)
{
  NeighbourMacList::iterator it;

  for (it = ngbrMacList.begin(); it != ngbrMacList.end(); it++)
  {
    if ( (*it) == addr )
      break;
  }

  if ( it == ngbrMacList.end() )
    ngbrMacList.push_back(addr);
}
