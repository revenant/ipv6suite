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

#include "sys.h"
#include "debug.h"

#include <iostream>
#include <iomanip>

#include "WirelessEtherStateIdle.h"
#include "WirelessEtherModule.h"
#include "WirelessAccessPoint.h"
#include "WirelessEtherSignal_m.h"
#include "WirelessEtherFrame_m.h"
#include "WirelessEtherFrameBody_m.h"

#include "WirelessEtherStateBackoff.h"
#include "WirelessEtherStateReceive.h"
#include "WirelessEtherStateBackoff.h"
#include "WEQueue.h"


WirelessEtherStateIdle *WirelessEtherStateIdle::_instance = 0;

WirelessEtherStateIdle *WirelessEtherStateIdle::instance()
{
    if (_instance == 0)
        _instance = new WirelessEtherStateIdle;

    return _instance;
}

std::auto_ptr<cMessage> WirelessEtherStateIdle::processSignal(WirelessEtherModule *mod,
                                                                 std::auto_ptr<cMessage> msg)
{
    return WirelessEtherState::processSignal(mod, msg);
}

std::auto_ptr<WESignalIdle> WirelessEtherStateIdle::processIdle(WirelessEtherModule *mod,
                                                                   std::auto_ptr<WESignalIdle> idle)
{
    mod->decNoOfRxFrames();
    return idle;
}

std::auto_ptr<WESignalData> WirelessEtherStateIdle::processData(WirelessEtherModule *mod,
                                                                   std::auto_ptr<WESignalData> data)
{
    mod->incNoOfRxFrames();
    mod->frameSource = data->sourceName();

    // Probe wait time extensions only apply to mobile nodes
    if (!mod->isAP())
    {
        if (mod->activeScan)
        {
            if (mod->prbEnergyScanNotifier->isScheduled())
            {
                assert(!mod->prbRespScanNotifier->isScheduled());

                mod->cancelEvent(mod->prbEnergyScanNotifier);

                // Extend probe wait time since there is traffic in the channel
                double nextSchedTime = mod->simTime() + mod->probeResponseTimeout;

                mod->scheduleAt(nextSchedTime, mod->prbRespScanNotifier);
            }
        }
    }

    // check that we are not in backoff state
    assert(!mod->backoffTimer->isScheduled());

    // make sure the inputFrame is empty to store new frame
    assert(!mod->inputFrame);

    // XXX has to duplicate the data because the auto_ptr will delete the
    // instance afterwards
    mod->inputFrame = (WESignalData *) data->dup();

    mod->outputQueue->startIdleReceive(mod->simTime());

    mod->outputQueue->endIdleCount(mod->simTime());
    mod->outputQueue->startBusyCount(mod->simTime());

    // entering receive state and waiting to finish receiving the frame
    mod->changeState(WirelessEtherStateReceive::instance());

    return data;
}

void WirelessEtherStateIdle::chkOutputBuffer(WirelessEtherModule *mod)
{
    // SW HACK: When a MS's entry expires in the AP, there could still be
    // frames destined for that MS in the AP's buffer. This removes data
    // destined for MSs no longer associated with the AP. It will prevent
    // the channel being used uneccesarily.
    if (mod->isAP())
    {
        while (mod->outputQueue->size())
        {
            WirelessEtherBasicFrame *outputFrame = mod->outputQueue->getReadyFrame();
            FrameControl frameControl = outputFrame->getFrameControl();

            // Only remove data frames
            if (frameControl.subtype == ST_DATA)
            {
                WirelessAccessPoint *ap = check_and_cast<WirelessAccessPoint *>(mod);
                WirelessEtherInterface dest = ap->findIfaceByMAC(outputFrame->getAddress1());

                // Remove if its a unicast address not in the list of associated MS
                if ((outputFrame->getAddress1() == MACAddress6(WE_BROADCAST_ADDRESS)) ||
                    (dest != UNSPECIFIED_WIRELESS_ETH_IFACE))
                    break;
                else
                    mod->outputQueue->prepareNextFrame(mod->simTime());
            }
            else
                break;
        }
    }

    // Attempt to send if there is something in the queue
    if (mod->outputQueue->size())
    {
        WirelessEtherBasicFrame *frame = mod->outputQueue->getReadyFrame();
        assert(frame);          // check if the frame is ok

        // Update stats if its a DATA frame
        if (frame->getFrameControl().subtype == ST_DATA)
        {
            mod->dataReadyTimeStamp = mod->simTime();
            mod->CWStat->collect(mod->outputQueue->getCW());
            mod->avgCWStat->collect(mod->outputQueue->getCW());
            mod->backoffSlotsStat->collect(mod->outputQueue->getBackoffSlots());
            mod->avgBackoffSlotsStat->collect(mod->outputQueue->getBackoffSlots());
        }

        // set timer for backoff
        double nextSchedTime = mod->simTime() + mod->outputQueue->getTimeToSend();
        assert(!mod->backoffTimer->isScheduled());
        mod->reschedule(mod->backoffTimer, nextSchedTime);

        mod->outputQueue->startingContention(mod->simTime());
        mod->changeState(WirelessEtherStateBackoff::instance());

        wEV << currentTime() << " sec, " << mod->fullPath() << ": start backing off and scheduled to cease backoff in " << mod->outputQueue->getTimeToSend() << " seconds"<< " cw: "<<mod->outputQueue->getCW()<<" numSlots: "<<mod->outputQueue->getBackoffSlots() << "\n";
    }
}
