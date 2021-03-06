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
#include "WirelessEtherSignal_m.h"
#include "WirelessEtherFrame_m.h"
#include "WirelessEtherFrameBody_m.h"
#include "WirelessEtherStateBackoff.h"
#include "WirelessEtherStateIdle.h"
#include "WirelessAccessPoint.h"
#include "WEQueue.h"

WirelessEtherStateAwaitACKReceive *WirelessEtherStateAwaitACKReceive::_instance = 0;

WirelessEtherStateAwaitACKReceive *WirelessEtherStateAwaitACKReceive::instance()
{
    if (_instance == 0)
        _instance = new WirelessEtherStateAwaitACKReceive;

    return _instance;
}

std::auto_ptr<cMessage> WirelessEtherStateAwaitACKReceive::processSignal(WirelessEtherModule *mod,
                                                                            std::auto_ptr<cMessage> msg)
{
    return WirelessEtherState::processSignal(mod, msg);
}

std::auto_ptr<WESignalIdle> WirelessEtherStateAwaitACKReceive::processIdle(WirelessEtherModule *mod,
                                                                              std::auto_ptr<WESignalIdle> idle)
{
    mod->decNoOfRxFrames();

    mod->outputQueue->endBusyCount(mod->simTime());
    mod->outputQueue->startIdleCount(mod->simTime());

    // If there is no input Frame, means collision has occured
    if (mod->inputFrame)
    {
        if (mod->frameSource == idle->sourceName())
        {
            WirelessEtherBasicFrame *frame =
                dynamic_cast<WirelessEtherBasicFrame *>(mod->inputFrame->encapsulatedMsg());

            if (frame->getFrameControl().subtype == ST_ACK)
            {
                mod->decodeFrame(mod->inputFrame);
            }

            if (mod->ackReceived)
            {
                mod->changeState(WirelessEtherStateIdle::instance());
                static_cast<WirelessEtherStateIdle *>(mod->currentState())->chkOutputBuffer(mod);
            }
            else
            {
                wEV << currentTime() << " sec, " << mod->fullPath() << ": frame received is not an ACK\n";

                // Stop the ACK timeout and start retransmission now
                cMessage *awaitAckTimer = mod->awaitAckTimer;
                if (awaitAckTimer->isScheduled())
                {
                    mod->cancelEvent(awaitAckTimer);
                    mod->changeState(WirelessEtherStateAwaitACK::instance());
                    static_cast<WirelessEtherStateAwaitACK *>(mod->currentState())->endAwaitACK(mod);
                }
            }
        }
        else
        {
            wEV << currentTime() << " sec, " << mod->fullPath() << ": (idle non matching) collision detected in ACK\n";
        }
        delete mod->inputFrame;
        mod->inputFrame = 0;
    }
    else
    {
        wEV << currentTime() << " sec, " << mod->fullPath() << ": collision detected in ACK\n";

        cMessage *awaitAckTimer = mod->awaitAckTimer;
        assert(awaitAckTimer);

        // Stop the ACK timeout
        if (!awaitAckTimer->isScheduled())
        {
            if (mod->getNoOfRxFrames() == 0)
            {
                mod->changeState(WirelessEtherStateAwaitACK::instance());
                static_cast<WirelessEtherStateAwaitACK *>(mod->currentState())->endAwaitACK(mod);
            }
        }
    }
    return idle;
}

std::auto_ptr<WESignalData> WirelessEtherStateAwaitACKReceive::processData(WirelessEtherModule *mod,
                                                                              std::auto_ptr<WESignalData> data)
{
    mod->incNoOfRxFrames();

    // Receiving data in this state means collision has occured
    delete mod->inputFrame;
    mod->inputFrame = 0;

  wEV << currentTime() << " sec, " << mod->fullPath() << ": there is collision during receive ACK!!!\n";

    return data;
}
