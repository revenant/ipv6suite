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
    @file WirelessEtherStateIdle.cc
    @brief Header file for WirelessEtherStateIdle

    Super class of wireless Ethernet State

    @author Greg Daley
            Eric Wu
*/

#include "sys.h" // Dout
#include "debug.h" // Dout

#include <iostream>
#include <iomanip>

#include "WirelessEtherStateIdle.h"
#include "WirelessEtherModule.h"
#include "WirelessAccessPoint.h"
#include "WirelessEtherSignal.h"
#include "WirelessEtherFrame_m.h"
#include "WirelessEtherFrameBody_m.h"

#include "WirelessEtherStateBackoff.h"
#include "WirelessEtherStateReceive.h"
#include "WirelessEtherStateBackoff.h"

#include "cTimerMessageCB.h"
#include "opp_akaroa.h"

WirelessEtherStateIdle* WirelessEtherStateIdle::_instance = 0;

WirelessEtherStateIdle* WirelessEtherStateIdle::instance()
{
  if (_instance == 0)
    _instance = new WirelessEtherStateIdle;

  return _instance;
}

std::auto_ptr<cMessage> WirelessEtherStateIdle::processSignal(WirelessEtherModule* mod, std::auto_ptr<cMessage> msg)
{
  return WirelessEtherState::processSignal(mod, msg);
}

std::auto_ptr<WESignalIdle> WirelessEtherStateIdle::processIdle(WirelessEtherModule* mod, std::auto_ptr<WESignalIdle> idle)
{
  mod->decNoOfRxFrames();
  return idle;
}

std::auto_ptr<WESignalData> WirelessEtherStateIdle::processData(WirelessEtherModule* mod, std::auto_ptr<WESignalData> data)
{
  mod->incNoOfRxFrames();
  mod->frameSource = data->sourceName();

  // Probe wait time extensions only apply to mobile nodes
  if(!mod->isAP())
  {
    if(mod->activeScan)
    {
      cTimerMessage* tmrEnergy = mod->getTmrMessage(TMR_PRBENERGYSCAN);
      assert(tmrEnergy);
      cTimerMessage* tmrResp = mod->getTmrMessage(TMR_PRBRESPSCAN);
      assert(tmrResp);

      if(tmrEnergy->isScheduled())
      {
        assert(!tmrResp->isScheduled());

        tmrEnergy->cancel();

        // Extend probe wait time since there is traffic in the channel
        double nextSchedTime = mod->simTime() + mod->probeResponseTimeout;

        mod->scheduleAt(nextSchedTime, tmrResp);
      }
    }
  }

  // check that we are not in backoff state
  assert(!mod->getTmrMessage(WIRELESS_SELF_AWAITMAC) ||
         !mod->getTmrMessage(WIRELESS_SELF_AWAITMAC)->isScheduled());

  // make sure the inputFrame is empty to store new frame
  assert(!mod->inputFrame);

  // has to duplicate the data because the auto_ptr will delete the
  // instance afterwards
  mod->inputFrame = data.get()->dup();

  mod->waitStartTime = mod->simTime();

  // entering receive state and waiting to finish receiving the frame
  mod->changeState(WirelessEtherStateReceive::instance());

  return data;
}

void WirelessEtherStateIdle::chkOutputBuffer(WirelessEtherModule* mod)
{
    // SW HACK: When a MS's entry expires in the AP, there could still be
    // frames destined for that MS in the AP's buffer. This removes data
    // destined for MSs no longer associated with the AP. It will prevent
    // the channel being used uneccesarily.
    if(mod->isAP())
    {
        while(mod->outputBuffer.size())
        {
            WESignalData* a = *(mod->outputBuffer.begin());

            WirelessEtherBasicFrame* outputFrame =
              static_cast<WirelessEtherBasicFrame*>(a->data());

            FrameControl frameControl = outputFrame->getFrameControl();

            // Only remove data frames
            if(frameControl.subtype == ST_DATA)
            {
                WirelessAccessPoint* ap =
                    boost::polymorphic_downcast<WirelessAccessPoint*>(mod);

                WirelessEtherInterface dest = ap->findIfaceByMAC(outputFrame->getAddress1());

                // Remove if its a unicast address not in the list of associated MS
                if(    (outputFrame->getAddress1() == MACAddress(ETH_BROADCAST_ADDRESS)) ||
                        (dest != UNSPECIFIED_WIRELESS_ETH_IFACE)    )
                {
                    ap->usedBW.sampleTotal += outputFrame->length();
                    break;
                }
                else
                    mod->outputBuffer.pop_front();
            }
            else
                break;
        }
    }

  if (mod->outputBuffer.size())
  {
    // switch to backoff state and schedule backoff time for minimum
    // contention window

    mod->changeState(WirelessEtherStateBackoff::instance());

    cTimerMessage* a = mod->getTmrMessage(WIRELESS_SELF_AWAITMAC);
    if (!a)
    {
      Loki::cTimerMessageCB<void, TYPELIST_1(WirelessEtherModule*)>* tmr;

      tmr = new Loki::cTimerMessageCB<void, TYPELIST_1(WirelessEtherModule*)>
        (WIRELESS_SELF_AWAITMAC, mod,
         static_cast<WirelessEtherStateBackoff*>(mod->currentState()),
         &WirelessEtherStateBackoff::readyToSend, "readyToSend");

      Loki::Field<0> (tmr->args) = mod;
      mod->addTmrMessage(tmr);
      a = tmr;
    }

    int numSlots;

    //check if output frame is a probe req/resp and fast active scan is enabled
    WESignalData* outData = *(mod->outputBuffer.begin());
    assert(outData); // check if the frame is ok
    if( ((outData->data()->getFrameControl().subtype == ST_PROBEREQUEST)||(outData->data()->getFrameControl().subtype == ST_PROBERESPONSE))&& mod->fastActiveScan())
    {
      numSlots = intuniform(0,CW_MIN);
      mod->backoffTime = numSlots * SLOTTIME + SIFS;
    }
    else
    {
      int cw = (1 << mod->contentionWindowPower()) - 1;
      numSlots = intuniform(0, cw);
      mod->backoffTime = numSlots * SLOTTIME + DIFS;
    }

    if(outData->data()->getFrameControl().subtype == ST_DATA)
    {
      mod->totalBackoffTime.sampleTotal += mod->backoffTime;
    }

    double nextSchedTime = mod->simTime() + mod->backoffTime;

    Dout(dc::wireless_ethernet|flush_cf, "MAC LAYER: " << std::fixed << std::showpoint << std::setprecision(12) << mod->simTime() << " sec, " << mod->fullPath() << ": start backing off and scheduled to cease backoff in " << mod->backoffTime << " seconds");

    assert(!a->isScheduled());
    a->reschedule(nextSchedTime);
  }
}
