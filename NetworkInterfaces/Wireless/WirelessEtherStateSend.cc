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
    @file WirelessEtherStateSend.cc
    @brief Header file for WirelessEtherStateSend

    Super class of wireless Ethernet State

    @author Greg Daley
            Eric Wu
*/

#include "sys.h"
#include "debug.h"

#include <iostream>
#include <iomanip>

#include "WirelessEtherStateSend.h"
#include "WirelessEtherModule.h"
#include "WirelessEtherSignal_m.h"
#include "WirelessEtherFrame_m.h"
#include "WirelessEtherFrameBody_m.h"

#include "WirelessEtherStateIdle.h"
#include "WirelessEtherStateAwaitACK.h"
#include "WirelessEtherStateAwaitACKReceive.h"
#include "WirelessEtherStateBackoff.h"
#include "WEQueue.h"

#include "cTimerMessage.h"

WirelessEtherStateSend *WirelessEtherStateSend::_instance = 0;

WirelessEtherStateSend *WirelessEtherStateSend::instance()
{
    if (_instance == 0)
        _instance = new WirelessEtherStateSend;

    return _instance;
}

std::auto_ptr<cMessage> WirelessEtherStateSend::processSignal(WirelessEtherModule *mod,
                                                                 std::auto_ptr<cMessage> msg)
{
    return WirelessEtherState::processSignal(mod, msg);
}

std::auto_ptr<WESignalIdle> WirelessEtherStateSend::processIdle(WirelessEtherModule *mod,
                                                                   std::auto_ptr<WESignalIdle> idle)
{
    mod->decNoOfRxFrames();
    return idle;
}

std::auto_ptr<WESignalData> WirelessEtherStateSend::processData(WirelessEtherModule *mod,
                                                                   std::auto_ptr<WESignalData> data)
{
    if (WEBASICFRAME_IN(data)->getFrameControl().subtype == ST_ACK)
    {
        wEV << currentTime() << " sec, " << mod->fullPath() << ":" << "while sending, an ACK was received\n";
    }
    else
    {
        wEV << currentTime() << " sec, " << mod->fullPath() << ": " << "while sending, received data from: " << ((WirelessEtherRTSFrame *) (data->encapsulatedMsg()))->getAddress2() << "\n";
    }
    mod->incNoOfRxFrames();
    return data;
}

void WirelessEtherStateSend::endSendingData(WirelessEtherModule *mod)
{
    // Send the end of frames to all modules which were sent the start
    mod->sendEndOfFrame();

    mod->outputQueue->startIdleCount(mod->simTime());

    // Check if there is anything in the buffer to wait for ack. Could
    // have been cleared from initiating active scanning during
    // authentication or association timeout. However, this may mean that
    // it is able to send the end of frame, eventhought the channel has
    // been changed due to active scan restarting.
    if (mod->outputQueue->size() < 1)
        return;

    // State is changed to Idle if it is a broadcast frame.
    if (mod->handleSendingBcastFrame())
        return;

    mod->ackReceived = false;

    // TODO: maybe the bandwidth for ACK transmission is different
    if (mod->getNoOfRxFrames() > 0)
        mod->changeState(WirelessEtherStateAwaitACKReceive::instance());
    else
    {
        double ackTxTime = ACKLENGTH / BASE_SPEED;
        mod->reschedule(mod->awaitAckTimer, mod->simTime() + SIFS + SLOTTIME + ackTxTime /*+ SAP_DELAY */ );
        // mod->reschedule(mod->awaitAckTimer, mod->simTime() +  SIFS + ackTxTime /*+ SAP_DELAY*/);

        wEV << currentTime() << " sec, " << mod->fullPath() << ": " << "ends await ack at " << formatTime(mod->simTime() + SIFS + SLOTTIME + ackTxTime) << " sec\n";
        mod->changeState(WirelessEtherStateAwaitACK::instance());
    }
}
