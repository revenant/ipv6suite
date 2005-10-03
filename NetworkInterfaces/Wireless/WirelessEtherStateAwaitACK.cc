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
#include "WEQueue.h"

WirelessEtherStateAwaitACK *WirelessEtherStateAwaitACK::_instance = 0;

WirelessEtherStateAwaitACK *WirelessEtherStateAwaitACK::instance()
{
    if (_instance == 0)
        _instance = new WirelessEtherStateAwaitACK;

    return _instance;
}

std::auto_ptr<cMessage> WirelessEtherStateAwaitACK::processSignal(WirelessEtherModule *mod,
                                                                     std::auto_ptr<cMessage> msg)
{
    return WirelessEtherState::processSignal(mod, msg);
}

std::auto_ptr<WESignalIdle> WirelessEtherStateAwaitACK::processIdle(WirelessEtherModule *mod,
                                                                       std::auto_ptr<WESignalIdle> idle)
{
    mod->decNoOfRxFrames();
    return idle;
}

std::auto_ptr<WESignalData> WirelessEtherStateAwaitACK::processData(WirelessEtherModule *mod,
                                                                       std::auto_ptr<WESignalData> data)
{
    mod->incNoOfRxFrames();
    mod->frameSource = data->sourceName();

    mod->outputQueue->endIdleCount(mod->simTime());
    mod->outputQueue->startBusyCount(mod->simTime());

    // check that we are not in backoff state
    assert(!mod->backoffTimer->isScheduled());

    // make sure the inputFrame is empty to store new frame
    assert(!mod->inputFrame);

    // XXX has to duplicate the data because the auto_ptr will delete the
    // instance afterwards.
    mod->inputFrame = check_and_cast<WESignalData *>(data.get()->dup());

    // entering receive state and waiting to finish receiving the ACK frame
    mod->changeState(WirelessEtherStateAwaitACKReceive::instance());

    return data;
}

// Prepare for the next retransmission when ACK is not received
void WirelessEtherStateAwaitACK::endAwaitACK(WirelessEtherModule *mod)
{
    // Just in case there is a frame in input when ACK timeout expires
    if (mod->inputFrame != 0)
    {
        delete mod->inputFrame;
        mod->inputFrame = 0;
    }

    // If all the frames havent been fully received when awaitack expired
    // go back to AwaitAckReceive to receive all frames
    if (mod->getNoOfRxFrames() > 0)
    {
        mod->changeState(WirelessEtherStateAwaitACKReceive::instance());
        return;
    }

    // Get a copy of the frame sent, since it may be deleted when initiateRetry
    // is called.
    WirelessEtherBasicFrame *frame =
        dynamic_cast<WirelessEtherBasicFrame *>(mod->outputQueue->getReadyFrame()->dup());

    // SWOON HACK: To find achievable throughput
    if (mod->isAP())
    {
        WirelessAccessPoint *apMod = check_and_cast<WirelessAccessPoint *>(mod);
        if (apMod->outputQueue->getQueueType() == AC_VO)
            apMod->avgCollDurationStat->collect((double) frame->length() / BASE_SPEED + collOhDurationVO);
        else if (mod->outputQueue->getQueueType() == AC_VI)
            apMod->avgCollDurationStat->collect((double) frame->length() / BASE_SPEED + collOhDurationVI);
        else
        {
            apMod->collDurationBE += (double) frame->length() / BASE_SPEED + collOhDurationBE;
            apMod->avgCollDurationStat->collect((double) frame->length() / BASE_SPEED + collOhDurationBE);
        }

    }


    // If maximum retry has been reached, dont attempt to retrasmit again, go on to next frame
    if (!mod->outputQueue->initiateRetry(mod->simTime()))
    {
        // If frame sent was a DATA frame, collect stats
        if (frame->getFrameControl().subtype == ST_DATA)
        {
            mod->noOfFailedTxStat++;
            mod->TxAccessTimeStat->collect(mod->simTime() - mod->dataReadyTimeStamp);
            mod->avgTxAccessTimeStat->collect(mod->simTime() - mod->dataReadyTimeStamp);
        }
        wEV << currentTime() << " sec, " << mod->fullPath() << ": " << "maximum retry triggered.. discard frame\n";

        // Update the consecutive failed transmission count if its an AP
        if (mod->isAP())
        {
            WirelessAccessPoint *ap = check_and_cast<WirelessAccessPoint *>(mod);
            ap->updateConsecutiveFailedCount(frame);
        }
        // TODO: change receive mod if it is not a access point?

        mod->changeState(WirelessEtherStateIdle::instance());
        static_cast<WirelessEtherStateIdle *>(mod->currentState())->chkOutputBuffer(mod);
    }
    else                        // retry sending same frame
    {
        wEV << currentTime() << " sec, " << mod->fullPath() << ": " << "next retry: " << mod->outputQueue->getRetry() << "\n";

        // Get the frame being transmitted
        WirelessEtherBasicFrame *txFrame = mod->outputQueue->getReadyFrame();

        // If its a DATA frame, collect stats
        if (txFrame->getFrameControl().subtype == ST_DATA)
        {
            mod->CWStat->collect(mod->outputQueue->getCW());
            mod->avgCWStat->collect(mod->outputQueue->getCW());
            mod->backoffSlotsStat->collect(mod->outputQueue->getBackoffSlots());
            mod->avgBackoffSlotsStat->collect(mod->outputQueue->getBackoffSlots());
        }

        mod->outputQueue->startingContention(mod->simTime());
        mod->changeState(WirelessEtherStateBackoff::instance());

        // We go to Backoff state instead of Backoff because we know that
        // there is nothing in the medium as all MS's cease sending the
        // frames (i.e. not IDLE will be received) whereas in backoff, we
        // are awaiting the medium to be free by the reception of the IDLE
        assert(!mod->backoffTimer->isScheduled());
        double nextSchedTime = mod->simTime() + mod->outputQueue->getTimeToSend();

        wEV << currentTime() << " sec, " << mod->fullPath() << ": start backing off and scheduled to cease backoff in " << mod->outputQueue->getTimeToSend() << " seconds\n";
        mod->reschedule(mod->backoffTimer, nextSchedTime);

    }
    delete frame;
}
