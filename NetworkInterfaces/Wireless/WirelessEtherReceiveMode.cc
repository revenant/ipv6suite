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

	@author	Steve Woon
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
#include "WirelessEtherSignal.h"
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
  WirelessEtherBasicFrame* frame = signal->data();
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
  if(changeState == true)
  {
    static_cast<WirelessEtherStateReceive*>(mod->currentState())->
      changeNextState(mod);
  }
}

void WEReceiveMode::finishFrameTx(WirelessEtherModule* mod)
{
  WESignalData* signal = *(mod->outputBuffer.begin());
  assert(signal->data());
  
  WirelessEtherBasicFrame* frame = static_cast<WirelessEtherBasicFrame*>
    (signal->data());
  assert(frame);

  // Update statistics if data frame transmitted.
  if (frame->getFrameControl().subtype == ST_DATA)
  {
    mod->noOfSuccessfulTx++;
    mod->throughput.sampleTotal += (double)(frame->encapsulatedMsg()->length());
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

void WEReceiveMode::sendAck(WirelessEtherModule* mod, 
                            WESignalData* ack)
{
  //mod->changeState(WirelessEtherStateReceive::instance());
  mod->sendFrame(ack);

  // Schedule an event to indicate end of Ack transmission
  cTimerMessage* a = mod->getTmrMessage(WIRELESS_SELF_ENDSENDACK);
  if (!a)
  {
    Loki::cTimerMessageCB<void, TYPELIST_1(WirelessEtherModule*)>* tmr;
      
    tmr = new Loki::cTimerMessageCB<void, TYPELIST_1(WirelessEtherModule*)>
      (WIRELESS_SELF_ENDSENDACK, mod, 
       static_cast<WirelessEtherStateReceive*>(mod->currentState()), 
       &WirelessEtherStateReceive::endSendingAck, "endSendingAck");
      
    Loki::Field<0> (tmr->args) = mod;
    mod->addTmrMessage(tmr);
    a = tmr;
  }

  double d = (double)ack->data()->length()*8;
  simtime_t transmTime = d / BASE_SPEED;

  delete ack;

  assert(!a->isScheduled());
  a->reschedule(mod->simTime() + transmTime);

  Dout(dc::wireless_ethernet|flush_cf, "MAC LAYER: " << std::fixed << std::showpoint << setprecision(12)<< mod->simTime() 
       << " sec, " << mod->fullPath() << ": " 
       << "schedule to finish sending ack at " 
       << std::fixed << std::showpoint << std::setprecision(12) << mod->simTime() +  transmTime << " sec");
}
