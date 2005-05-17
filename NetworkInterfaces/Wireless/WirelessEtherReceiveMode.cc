// -*- C++ -*-
// Copyright (C) 2001 Monash University, Melbourne, Australia
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
    @file WirelessEtherReceiveMode.cc
    @brief Source file for WEReceiveMode

    Super class of wireless Ethernet Receive Mode

    @author    Steve Woon
          Eric Wu
*/
#include "sys.h" // Dout
#include "debug.h" // Dout

#include <iomanip>


#include "cTTimerMessageCB.h"

#include "WirelessEtherReceiveMode.h"
#include "WirelessAccessPoint.h"

#include "WirelessEtherState.h"
#include "WirelessEtherModule.h"
#include "WirelessEtherSignal_m.h"
#include "WirelessEtherFrame_m.h"
#include "WirelessEtherFrameBody_m.h"

#include "WirelessEtherStateIdle.h"
#include "WirelessEtherStateSend.h"
#include "WirelessEtherStateBackoff.h"
#include "WirelessEtherStateReceive.h"
#include "WirelessEtherStateAwaitACK.h"
#include "WirelessEtherStateBackoffReceive.h"
#include "WirelessEtherStateBackoff.h"

#include "WirelessEtherAScanReceiveMode.h"

WEReceiveMode* WEReceiveMode::_instance = 0;

WEReceiveMode* WEReceiveMode::instance()
{
  if (_instance == 0)
    _instance = new WEReceiveMode;

  return _instance;
}

void WEReceiveMode::decodeFrame(WirelessEtherModule* mod, WESignalData* signal)
{
  WirelessEtherBasicFrame* frame = check_and_cast<WirelessEtherBasicFrame*>(signal->encapsulatedMsg());
  assert(frame);

  FrameControl frameControl = frame->getFrameControl();

  //mod->changeState(WirelessEtherStateIdle::instance());
  changeState = true;


  switch(frameControl.subtype)
  {
    case ST_BEACON:
      handleBeacon(mod, signal);
      break;
    case ST_PROBEREQUEST:
      handleProbeRequest(static_cast<WirelessAccessPoint*>(mod), signal);
      break;
    case ST_PROBERESPONSE:
      handleProbeResponse(mod, signal);
      break;
    case ST_ASSOCIATIONREQUEST:
      handleAssociationRequest(static_cast<WirelessAccessPoint*>(mod), signal);
      break;
    case ST_ASSOCIATIONRESPONSE:
      handleAssociationResponse(mod, signal);
      break;
    case ST_REASSOCIATIONREQUEST:
      handleReAssociationRequest(static_cast<WirelessAccessPoint*>(mod), signal);
      break;
    case ST_REASSOCIATIONRESPONSE:
      handleReAssociationResponse(mod, signal);
      break;
    case ST_DISASSOCIATION:
      handleDisAssociation(mod, signal);
      break;
    case ST_DATA:
      handleData(mod, signal);
      break;
    case ST_ACK:
      handleAck(mod, signal);
      break;
    case ST_AUTHENTICATION:
      handleAuthentication(mod, signal);
      break;
    case ST_DEAUTHENTICATION:
      handleDeAuthentication(mod, signal);
      break;
  }
  //Frames which do not need to be ACK can change states immediately
  if(changeState == true)
  {
    static_cast<WirelessEtherStateReceive*>(mod->currentState())->
      changeNextState(mod);
  }
}

void WEReceiveMode::finishFrameTx(WirelessEtherModule* mod)
{
  WESignalData* signal = *(mod->outputBuffer.begin());
  assert(signal->encapsulatedMsg());

  WirelessEtherBasicFrame* frame = static_cast<WirelessEtherBasicFrame*>
    (signal->encapsulatedMsg());
  assert(frame);

  // Update statistics if data frame transmitted.
  if (frame->getFrameControl().subtype == ST_DATA)
  {
    mod->noOfTxStat++;
    mod->TxDataBWStat += (double)frame->encapsulatedMsg()->length()/1000000;
    mod->TxFrameSizeStat->collect((double)frame->encapsulatedMsg()->length()/8);
    mod->avgTxFrameSizeStat->collect(frame->encapsulatedMsg()->length()/8);
    mod->TxAccessTimeStat->collect(mod->simTime()-mod->dataReadyTimeStamp);
    mod->avgTxAccessTimeStat->collect(mod->simTime()-mod->dataReadyTimeStamp);
    if(mod->statsVec)
      mod->InstTxFrameSizeVec->record(frame->encapsulatedMsg()->length()/8);
  }

  delete *(mod->outputBuffer.begin());
  mod->outputBuffer.pop_front();
  mod->resetRetry();
  mod->resetContentionWindowPower();
  mod->backoffTime = 0;
  mod->incrementSequenceNumber();
  mod->idleNetworkInterface();
  mod->ackReceived = true;
}

void WEReceiveMode::scheduleAck(WirelessEtherModule* mod, WESignalData* ack)
{
  // Schedule to send an ACK
  cTimerMessage* a = mod->getTmrMessage(WIRELESS_SELF_SCHEDULEACK);
  
  Loki::cTimerMessageCB<void, 
    TYPELIST_2(WirelessEtherModule*,WESignalData*)>* tmr;
  
  if (!a)
  {
    tmr = new Loki::cTimerMessageCB<void, 
      TYPELIST_2(WirelessEtherModule*,WESignalData*)>
      (WIRELESS_SELF_SCHEDULEACK, mod, 
       static_cast<WirelessEtherStateReceive*>(mod->currentState()), 
       &WirelessEtherStateReceive::sendAck, "sendAck");
      
    Loki::Field<0> (tmr->args) = mod;
    mod->addTmrMessage(tmr);
    a = tmr;
  }
  else
    tmr = static_cast<Loki::cTimerMessageCB<void, TYPELIST_2(WirelessEtherModule*,WESignalData*)>*>(a);

  Loki::Field<1> (tmr->args) = ack;

  assert(!a->isScheduled());
  a->reschedule(mod->simTime() + SIFS);
}
