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
    @file WirelessEtherStateAwaitACK.cc
    @brief Header file for WirelessEtherStateAwaitACK

    Super class of wireless Ethernet State

    @author Greg Daley
            Eric Wu
*/

#include "sys.h"
#include "debug.h"

#include <iostream>
#include <iomanip>


#include "WirelessEtherStateAwaitACK.h"
#include "WirelessEtherStateAwaitACKReceive.h"
#include "WirelessEtherModule.h"
#include "WirelessEtherSignal_m.h"
#include "WirelessEtherFrame_m.h"
#include "WirelessEtherFrameBody_m.h"
#include "WirelessEtherStateBackoff.h"
#include "WirelessEtherStateIdle.h"
#include "WirelessAccessPoint.h"
#include "opp_akaroa.h"

WirelessEtherStateAwaitACK* WirelessEtherStateAwaitACK::_instance = 0;

WirelessEtherStateAwaitACK* WirelessEtherStateAwaitACK::instance()
{
  if (_instance == 0)
    _instance = new WirelessEtherStateAwaitACK;

  return _instance;
}

std::auto_ptr<cMessage> WirelessEtherStateAwaitACK::processSignal(WirelessEtherModule* mod, std::auto_ptr<cMessage> msg)
{
  return WirelessEtherState::processSignal(mod, msg);
}

std::auto_ptr<WESignalIdle> WirelessEtherStateAwaitACK::processIdle(WirelessEtherModule* mod, std::auto_ptr<WESignalIdle> idle)
{
  mod->decNoOfRxFrames();
  return idle;
}

std::auto_ptr<WESignalData> WirelessEtherStateAwaitACK::processData(WirelessEtherModule* mod, std::auto_ptr<WESignalData> data)
{
  mod->incNoOfRxFrames();
  mod->frameSource = data->sourceName();

  // check that we are not in backoff state
  assert(!mod->getTmrMessage(WIRELESS_SELF_AWAITMAC) ||
         !mod->getTmrMessage(WIRELESS_SELF_AWAITMAC)->isScheduled());

  // make sure the inputFrame is empty to store new frame
  assert(!mod->inputFrame);

  // XXX has to duplicate the data because the auto_ptr will delete the
  // instance afterwards
  mod->inputFrame = check_and_cast<WESignalData*>(data.get()->dup());

  // entering receive state and waiting to finish receiving the ACK frame
  mod->changeState(WirelessEtherStateAwaitACKReceive::instance());

  return data;
}

// Prepare for the next retransmission when ACK is not received
void WirelessEtherStateAwaitACK::endAwaitACK(WirelessEtherModule* mod)
{
  assert(!mod->backoffTime);

  // Just in case there is a frame in input when ACK timeout expires
  if(mod->inputFrame != 0)
  {
    delete mod->inputFrame;
    mod->inputFrame = 0;
  }

  WESignalData* signal = *(mod->outputBuffer.begin());
  assert(signal->encapsulatedMsg());

  WirelessEtherBasicFrame* frame = static_cast<WirelessEtherBasicFrame*>
    (signal->encapsulatedMsg());
  assert(frame);

  // Statistic collection
  if (frame->getFrameControl().subtype == ST_DATA)
  {
    mod->noOfFailedTx++;
  }

  // If maximum retry has been reached, dont attempt to retrasmit again, go on to next frame
  if ( mod->getRetry() > mod->getMaxRetry() )
  {
    /*  if (frame->getFrameControl().subtype == ST_DATA)
    {
      mod->noOfFailedTx++;
      }*/
    Dout(dc::wireless_ethernet|flush_cf, "MAC LAYER: " << std::fixed << std::showpoint << std::setprecision(12) << mod->simTime() << " sec, " << mod->fullPath() << ": " << "maximum retry triggered.. discard frame");

        mod->resetRetry();

    assert(mod->outputBuffer.size());

        //Update the consecutive failed transmission count if its an AP
        if(mod->isAP())
        {
            WirelessAccessPoint* ap = check_and_cast<WirelessAccessPoint*>(mod);
            ap->updateConsecutiveFailedCount();
        }

    WESignalData* a = *(mod->outputBuffer.begin());

    delete a;
    mod->outputBuffer.pop_front();

    // TODO: change receive mod if it is not a access point?

    mod->idleNetworkInterface();

    mod->changeState(WirelessEtherStateIdle::instance());
    static_cast<WirelessEtherStateIdle*>(mod->currentState())->chkOutputBuffer(mod);

    return;
  }

  mod->incContentionWindowPower();
  mod->incrementRetry();

  Dout(dc::wireless_ethernet|flush_cf, "MAC LAYER: " << std::fixed << std::showpoint << std::setprecision(12) << mod->simTime() << " sec, " << mod->fullPath() << ": " << "next retry: " << mod->getRetry());

  // Couldnt receive any ACK re-assign the backoff period, retry++ and
  // send it again

  // This may be due to some mobile station who was unware of the
  // status of the medium or other awkard cirumstances. Therefore we
  // are entering collision state and re-assigning the backoff period

  //check if output frame is a probe req/resp and fast active scan is enabled
  WESignalData* outData = *(mod->outputBuffer.begin());
  assert(outData); // check if the frame is ok

  // To support fast Active scanning, where you use the smallest contention window and SIFS wait time
  WirelessEtherBasicFrame *outDataEncapFrame = check_and_cast<WirelessEtherBasicFrame*>(outData->encapsulatedMsg());
  if( ((outDataEncapFrame->getFrameControl().subtype == ST_PROBEREQUEST)||(outDataEncapFrame->getFrameControl().subtype == ST_PROBERESPONSE))&& mod->fastActiveScan())
  {
    mod->backoffTime = (int)intuniform(0, CW_MIN) * SLOTTIME + SIFS;
  }
  else
  {
    int cw = (1 << mod->contentionWindowPower()) - 1;
    mod->backoffTime = (int)intuniform(0, cw) * SLOTTIME + DIFS;
  }

  if(FIXME_FIXME_FIXME_OUTDATA_DATA->getFrameControl().subtype == ST_DATA)
  {
    mod->totalBackoffTime.sampleTotal += mod->backoffTime;
  }

  mod->changeState(WirelessEtherStateBackoff::instance());
  // set retry flag of the sending frame to true
  FIXME_FIXME_FIXME_OUTDATA_DATA->getFrameControl().retry = true;

  // We go to Backoff state instead of Backoff because we know that
  // there is nothing in the medium as all MS's cease sending the
  // frames (i.e. not IDLE will be received) whereas in backoff, we
  // are awaiting the medium to be free by the reception of the IDLE
  cTimerMessage* a = mod->getTmrMessage(WIRELESS_SELF_AWAITMAC);
  assert(a);
  assert(!a->isScheduled());

  double nextSchedTime = mod->simTime() + mod->backoffTime;

  Dout(dc::wireless_ethernet|flush_cf, "MAC LAYER: " << std::fixed << std::showpoint << std::setprecision(12) << mod->simTime() << " sec, " << mod->fullPath() << ": start backing off and scheduled to cease backoff in " << mod->backoffTime << " seconds");
  a->reschedule(nextSchedTime);
}
