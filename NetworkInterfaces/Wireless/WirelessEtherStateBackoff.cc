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
    @file WirelessEtherStateBackoff.cc
    @brief Header file for WirelessEtherStateBackoff

    Super class of wireless Ethernet State

    @author Greg Daley
            Eric Wu
*/

#include "sys.h" // Dout
#include "debug.h" // Dout
#include <iostream>
#include <iomanip>

#include "WirelessEtherStateBackoff.h"

#include "cTimerMessageCB.h"

#include "WirelessEtherModule.h"
#include "WirelessEtherSignal_m.h"
#include "WirelessEtherFrame_m.h"
#include "WirelessEtherFrameBody_m.h"
#include "WirelessEtherStateBackoffReceive.h"
#include "WirelessEtherStateBackoff.h"
#include "WirelessEtherStateSend.h"

#include "opp_utils.h"

WirelessEtherStateBackoff* WirelessEtherStateBackoff::_instance = 0;

WirelessEtherStateBackoff* WirelessEtherStateBackoff::instance()
{
  if (_instance == 0)
    _instance = new WirelessEtherStateBackoff;

  return _instance;
}

std::auto_ptr<cMessage> WirelessEtherStateBackoff::processSignal(WirelessEtherModule* mod, std::auto_ptr<cMessage> msg)
{
  return WirelessEtherState::processSignal(mod, msg);
}

std::auto_ptr<WESignalIdle> WirelessEtherStateBackoff::processIdle(WirelessEtherModule* mod, std::auto_ptr<WESignalIdle> idle)
{
  mod->decNoOfRxFrames();
  return idle;
}

std::auto_ptr<WESignalData> WirelessEtherStateBackoff::processData(WirelessEtherModule* mod, std::auto_ptr<WESignalData> data)
{
  mod->incNoOfRxFrames();
  mod->frameSource = data->sourceName();

  cTimerMessage* a = mod->getTmrMessage(WIRELESS_SELF_AWAITMAC);

  // timer to await for MAC to be free should have already been created
  // now we want to stop the timer and wait for the medium to be idle
  assert(a);

  //check if output frame is a probe req/resp and fast active scan is enabled
  WESignalData* outData = *(mod->outputBuffer.begin());
  assert(outData); // check if the frame is ok
  if( ((WEBASICFRAME_IN(outData)->getFrameControl().subtype == ST_PROBEREQUEST)||(WEBASICFRAME_IN(outData)->getFrameControl().subtype == ST_PROBERESPONSE))&& mod->fastActiveScan())
  {
    if ( SIFS < a->elapsedTime() )
    {
      int slotsElapsed = (int)((a->elapsedTime()-SIFS)/SLOTTIME);
      mod->backoffTime = mod->backoffTime - (slotsElapsed * SLOTTIME);
    }
    else
      mod->backoffTime = mod->backoffTime + a->elapsedTime();
  }
  else
  {
    // the time for awaiting MAC has "eaten" the DIFS
    if ( DIFS < a->elapsedTime() )
    {
      int slotsElapsed = (int)((a->elapsedTime()-DIFS)/SLOTTIME);
      mod->backoffTime = mod->backoffTime - (slotsElapsed * SLOTTIME);
      mod->totalBackoffTime.sampleTotal += a->elapsedTime()-(slotsElapsed*SLOTTIME);
    }
    // the time for awaiting MAC has NOT "eaten" the DIFS
    else
    {
      mod->backoffTime = mod->backoffTime + a->elapsedTime();
      mod->totalBackoffTime.sampleTotal += a->elapsedTime();
    }
  }
  a->cancel();

  assert(!mod->inputFrame);
  mod->inputFrame = (WESignalData*) data.get()->dup(); //XXX eliminate dup()

  mod->waitStartTime = mod->simTime();

  mod->changeState(WirelessEtherStateBackoffReceive::instance());

  return data;
}

void WirelessEtherStateBackoff::readyToSend(WirelessEtherModule* mod)
{
  assert(mod->outputBuffer.size());

  WESignalData* data = *(mod->outputBuffer.begin());
  mod->sendFrame(data);

  // Keeping track of number of attempted tx
  WirelessEtherBasicFrame* frame = check_and_cast<WirelessEtherBasicFrame*>
    (data->encapsulatedMsg());
  assert(frame);

  if (frame->getFrameControl().subtype == ST_DATA)
  {
    mod->noOfAttemptedTx++;
  }

  mod->changeState(WirelessEtherStateSend::instance());

  // TODO: supported rates NOT IMPLEMENTED YET.. therefore bandwidth
  // is 1Mbps

  double d = (double)WEBASICFRAME_IN(data)->length()*8;
  simtime_t transmTime = d / BASE_SPEED;


  // GD Hack: assume not (toDS=1, FromDS=1).

  short st = WEBASICFRAME_IN(data)->getFrameControl().subtype;
  if(!(( st == ST_CTS) || (  st == ST_ACK )))  {
  Dout(dc::wireless_ethernet|flush_cf, "MAC LAYER: (WIRELESS) "
       << mod->fullPath() << " \n"
       << " ---------------------------------------------------- \n"
       << " sending a frame to : " << WEBASICFRAME_IN(data)->getAddress1() <<" WLAN Tx(2): "<<((WirelessEtherRTSFrame *)(data->encapsulatedMsg()))->getAddress2()
       << " will finish at " << std::fixed << std::showpoint << std::setprecision(12) << mod->simTime() + transmTime
       << "\n ---------------------------------------------------- \n");
  }
  else{
  Dout(dc::wireless_ethernet|flush_cf, "MAC LAYER: (WIRELESS) "
       << mod->fullPath() << " \n"
       << " ---------------------------------------------------- \n"
       << " sending a frame to : " << WEBASICFRAME_IN(data)->getAddress1()
       << " will finish at " << std::fixed << std::showpoint << std::setprecision(12) << mod->simTime() + transmTime
       << "\n ---------------------------------------------------- \n");
  }

  OPP_Global::ContextSwitcher switchContext(mod);

  cTimerMessage* tmrMessage = mod->getTmrMessage(WE_TRANSMIT_SENDDATA);

  if (!tmrMessage)
  {
    Loki::cTimerMessageCB<void, TYPELIST_1(WirelessEtherModule*)>* a;

    a  = new Loki::cTimerMessageCB<void, TYPELIST_1(WirelessEtherModule*)>
      (WE_TRANSMIT_SENDDATA, mod, static_cast<WirelessEtherStateSend*>
       (mod->currentState()), &WirelessEtherStateSend::endSendingData,
       "endSendingData");

    Loki::Field<0> (a->args) = mod;

    mod->addTmrMessage(a);
    tmrMessage = a;
  }

  mod->backoffTime = 0;

  tmrMessage->reschedule(mod->simTime() +  transmTime);
}
