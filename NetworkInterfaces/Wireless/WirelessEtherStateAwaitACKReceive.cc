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
	@file WirelessEtherStateAwaitACKReceive.cc
	@brief Header file for WirelessEtherStateAwaitACKReceive
    
    Super class of wireless Ethernet State

	@author Steve Woon
*/

#include "sys.h"
#include "debug.h"

#include <iostream>
#include <iomanip>

#include "WirelessEtherStateAwaitACKReceive.h"
#include "WirelessEtherModule.h"
#include "WirelessEtherStateAwaitACK.h"
#include "WirelessEtherSignal.h"
#include "WirelessEtherFrame_m.h"
#include "WirelessEtherFrameBody_m.h"
#include "WirelessEtherStateBackoff.h"
#include "WirelessEtherStateIdle.h"
#include "WirelessAccessPoint.h"

WirelessEtherStateAwaitACKReceive* WirelessEtherStateAwaitACKReceive::_instance = 0;

WirelessEtherStateAwaitACKReceive* WirelessEtherStateAwaitACKReceive::instance()
{
  if (_instance == 0)
    _instance = new WirelessEtherStateAwaitACKReceive;
  
  return _instance;
}

std::auto_ptr<cMessage> WirelessEtherStateAwaitACKReceive::processSignal(WirelessEtherModule* mod, std::auto_ptr<cMessage> msg)
{
  return WirelessEtherState::processSignal(mod, msg);
}

std::auto_ptr<WESignalIdle> WirelessEtherStateAwaitACKReceive::processIdle(WirelessEtherModule* mod, std::auto_ptr<WESignalIdle> idle)
{
  mod->decNoOfRxFrames();
  
  // If there is no input Frame, means collision has occured
  if ( mod->inputFrame )
  {
    if(mod->frameSource == idle->sourceName())
    {
      WirelessEtherBasicFrame* frame =
        static_cast<WirelessEtherBasicFrame*>(mod->inputFrame->data());

      if ( frame->getFrameControl().subtype == ST_ACK )
      {
        mod->decodeFrame(mod->inputFrame);
      }

      if ( mod->ackReceived )
      {
        mod->changeState(WirelessEtherStateIdle::instance());
        static_cast<WirelessEtherStateIdle*>(mod->currentState())->chkOutputBuffer(mod);
      }
      else
      {
        Dout(dc::wireless_ethernet|flush_cf, "MAC LAYER: " << std::fixed << std::showpoint << std::setprecision(12) << mod->simTime() << " sec, " << mod->fullPath() << ": frame received is not an ACK");
      
        cTimerMessage* awaitAckTmr = mod->getTmrMessage(WIRELESS_SELF_AWAITACK);
        // Stop the ACK timeout and start retransmission now
        if ( awaitAckTmr->isScheduled())
        {
          mod->getTmrMessage(WIRELESS_SELF_AWAITACK)->cancel();
          mod->changeState(WirelessEtherStateAwaitACK::instance());
          static_cast<WirelessEtherStateAwaitACK*>(mod->currentState())->endAwaitACK(mod);
        }
      }
    }
    else
    {
      Dout(dc::wireless_ethernet|flush_cf, "MAC LAYER: " << std::fixed << std::showpoint << std::setprecision(12) << mod->simTime() << " sec, " << mod->fullPath() << ": (idle non matching) collision detected in ACK");

      cTimerMessage* awaitAckTmr = mod->getTmrMessage(WIRELESS_SELF_AWAITACK);
    
      // Stop the ACK timeout
      if ( awaitAckTmr->isScheduled())
      {
        mod->getTmrMessage(WIRELESS_SELF_AWAITACK)->cancel();
      }
    }
    delete mod->inputFrame;
    mod->inputFrame = 0;
  }
  else
  {
    Dout(dc::wireless_ethernet|flush_cf, "MAC LAYER: " << std::fixed << std::showpoint << std::setprecision(12) << mod->simTime() << " sec, " << mod->fullPath() << ": collision detected in ACK");
      
    cTimerMessage* awaitAckTmr = mod->getTmrMessage(WIRELESS_SELF_AWAITACK);
    
    // Stop the ACK timeout
    if ( awaitAckTmr->isScheduled())
    {
      mod->getTmrMessage(WIRELESS_SELF_AWAITACK)->cancel();
    }
    // Start retransmission if all frames received
    if(mod->getNoOfRxFrames() == 0)
    {
      mod->changeState(WirelessEtherStateAwaitACK::instance());
      static_cast<WirelessEtherStateAwaitACK*>(mod->currentState())->endAwaitACK(mod);
    }
  }

  return idle;
}

std::auto_ptr<WESignalData> WirelessEtherStateAwaitACKReceive::processData(WirelessEtherModule* mod, std::auto_ptr<WESignalData> data)
{
  mod->incNoOfRxFrames();

  // Receiving data in this state means collision has occured
  delete mod->inputFrame;
  mod->inputFrame = 0;

  Dout(dc::wireless_ethernet|flush_cf, "MAC LAYER: " << std::fixed << std::showpoint << std::setprecision(12) << mod->simTime() << " sec, " << mod->fullPath() << ": there is collision during receive ACK!!!");

  return data;
}
