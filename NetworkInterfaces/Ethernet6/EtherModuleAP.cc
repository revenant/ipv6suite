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
#include "EtherSignal_m.h"
#include "EtherFrame6.h"
#include "EtherStateIdle.h"
#include "cTimerMessage.h"
#include "MACAddress6.h"

#ifdef USE_MOBILITY
#include "WirelessEtherBridge.h"
#include <memory>
#endif

Define_Module( EtherModuleAP);

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

    inGate = findGate("physIn");
    outGate = findGate("physOut");

    //Initialise Variables for Statistics
    statsUpdatePeriod = 1;
    RxDataBWStat = 0;
    TxDataBWStat = 0;
    noOfRxStat = 0;
    noOfTxStat = 0;

    RxDataBWVec = new cOutVector("RxDataBWVec");
    TxDataBWVec = new cOutVector("TxDataBWVec");
    noOfRxVec = new cOutVector("noOfRxVec");
    noOfTxVec = new cOutVector("noOfTxVec");

    statsVec = par("recordStatisticVector").boolValue();

    if ( statsVec )
    {
      InstRxFrameSizeVec = new cOutVector("InstRxFrameSizeVec");
      InstTxFrameSizeVec = new cOutVector("InstTxFrameSizeVec");
    }
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
    if ( std::string(msg->name()) == "WE_AP_NOTIFY_MAC")
    {
      _myAddr.set(static_cast<cPar*>(msg->parList().get(0))->stringValue());

      idleNetworkInterface();
    }

    delete msg;
    return;
  }

  EtherModule::handleMessage(msg);
}

void EtherModuleAP::finish()
{}

bool EtherModuleAP::receiveData(std::auto_ptr<cMessage> msg)
{
  // the frame should have already been created in the bridge module
  EtherSignalData* frame =
    check_and_cast<EtherSignalData *>(msg.get()->decapsulate());
  assert(frame);

  frame->setSrcModPathName(fullPath().c_str());

  outputBuffer.push_back(frame);

  if ( _currentState == EtherStateIdle::instance())
  {
    cTimerMessage* tmrMessage = getTmrMessage(SELF_INTERFRAMEGAP);
    if (!(tmrMessage && tmrMessage->isScheduled()))
      static_cast<EtherStateIdle*>(_currentState)->chkOutputBuffer(this);
  }

  return true;
}

bool EtherModuleAP::sendData(EtherFrame6* frame)
{
  assert(frame);

  send(frame->dup(), inputQueueOutGate());
  delete frame;

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
