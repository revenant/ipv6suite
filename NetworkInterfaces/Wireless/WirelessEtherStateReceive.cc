// -*- C++ -*-
// Copyright (C) 2001, 2005 Monash University, Melbourne, Australia
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
    @file WirelessEtherStateReceive.cc
    @brief Header file for WirelessEtherStateReceive

    Super class of wireless Ethernet State

    @author Greg Daley
            Eric Wu
            Steve Woon
*/

#include "sys.h" // Dout
#include "debug.h" // Dout

#include <iostream>
#include <iomanip>
#include <memory>

#include "WirelessEtherStateReceive.h"
#include "WirelessEtherStateBackoffReceive.h"
#include "WirelessEtherModule.h"
#include "WirelessEtherSignal_m.h"
#include "WirelessEtherStateIdle.h"


WirelessEtherStateReceive* WirelessEtherStateReceive::_instance = 0;

WirelessEtherStateReceive* WirelessEtherStateReceive::instance()
{
  if (_instance == 0)
    _instance = new WirelessEtherStateReceive;

  return _instance;
}

std::auto_ptr<cMessage> WirelessEtherStateReceive::processSignal(WirelessEtherModule* mod, std::auto_ptr<cMessage> msg)
{
  return WirelessEtherState::processSignal(mod, msg);
}

std::auto_ptr<WESignalIdle> WirelessEtherStateReceive::processIdle(WirelessEtherModule* mod, std::auto_ptr<WESignalIdle> idle)
{
  mod->decNoOfRxFrames();

  // If the input buffer points to nothing, this means that there was
  // collision detected by this ms/ap and the packet was discarded
  // silently. Therefore, we simply go to idle state when the sender
  // finishes sending the data (by the message, IDLE)
  if (mod->inputFrame == 0)
  {
      cTimerMessage* tmrMessage = mod->getTmrMessage(WIRELESS_SELF_ENDSENDACK);
      cTimerMessage* schedAck = mod->getTmrMessage(WIRELESS_SELF_SCHEDULEACK);
      // Check that all frames are fully received or ACK is fully sent before changing state
      if((mod->getNoOfRxFrames() == 0) && !(tmrMessage && tmrMessage->isScheduled()) && !(schedAck && schedAck->isScheduled()))
      {
        mod->totalWaitTime.sampleTotal += mod->simTime()-mod->waitStartTime;
        static_cast<WirelessEtherStateReceive*>(mod->currentState())->
          changeNextState(mod);
      }
      else
      {
        Dout(dc::wireless_ethernet|flush_cf, "MAC LAYER: " << std::fixed << std::showpoint << std::setprecision(12) << mod->simTime() << " sec, " << mod->fullPath() << ": will wait until all frames are received or ack is sent before moving onto the next state");
      }
      return idle;
  }

  if(mod->frameSource == idle->sourceName())
  {
    assert(mod->inputFrame);

    // the source MS finishes sending, we can send ACK to comfirm the
    // transmission process and process the data
    mod->decodeFrame(mod->inputFrame);
  }
  delete mod->inputFrame;
  mod->inputFrame = 0;

  return idle;
}

std::auto_ptr<WESignalData> WirelessEtherStateReceive::processData(WirelessEtherModule* mod, std::auto_ptr<WESignalData> data)
{
  //Increment number of frames being received. It will be greater than 1 hence there is a collision.
  mod->incNoOfRxFrames();

  // collision
  delete mod->inputFrame;
  mod->inputFrame = 0;

  Dout(dc::wireless_ethernet|flush_cf, "MAC LAYER: " << std::fixed << std::showpoint << std::setprecision(12) << mod->simTime() << " sec, " << mod->fullPath() << ": there is a collision!!!");

  return data;
}

void WirelessEtherStateReceive::changeNextState(WirelessEtherModule* mod)
{
  mod->changeState(WirelessEtherStateIdle::instance());
    static_cast<WirelessEtherStateIdle*>(mod->currentState())->chkOutputBuffer(mod);
}

void WirelessEtherStateReceive::sendAck(WirelessEtherModule* mod, 
                                        WESignalData* ack)
{
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
       << " sec, " << mod->fullPath() << ": Start Sending ACK");
}

void WirelessEtherStateReceive::endSendingAck(WirelessEtherModule* mod)
{
  mod->sendEndOfFrame();

  mod->idleNetworkInterface();

  Dout(dc::wireless_ethernet|flush_cf, "MAC LAYER: " << std::fixed << std::showpoint << setprecision(12)<< mod->simTime() 
       << " sec, " << mod->fullPath() << ": End Sending ACK");
  
  // Check that all frames are fully received before changing states
  if(mod->getNoOfRxFrames() == 0)
  {
    static_cast<WirelessEtherStateReceive*>(mod->currentState())->
      changeNextState(mod);
  }
  else
  {
    Dout(dc::wireless_ethernet|flush_cf, "MAC LAYER: " << std::fixed << std::showpoint << std::setprecision(12) << mod->simTime() << " sec, " << mod->fullPath() << ": will wait until all frames are received before moving onto the next state.");
  }
}
