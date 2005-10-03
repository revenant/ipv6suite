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

#include "sys.h"
#include "debug.h"
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
#include "WEQueue.h"

#include "opp_utils.h"

WirelessEtherStateBackoff *WirelessEtherStateBackoff::_instance = 0;

WirelessEtherStateBackoff *WirelessEtherStateBackoff::instance()
{
    if (_instance == 0)
        _instance = new WirelessEtherStateBackoff;

    return _instance;
}

std::auto_ptr<cMessage> WirelessEtherStateBackoff::processSignal(WirelessEtherModule *mod,
                                                                    std::auto_ptr<cMessage> msg)
{
    return WirelessEtherState::processSignal(mod, msg);
}

std::auto_ptr<WESignalIdle> WirelessEtherStateBackoff::processIdle(WirelessEtherModule *mod,
                                                                      std::auto_ptr<WESignalIdle> idle)
{
    mod->decNoOfRxFrames();
    return idle;
}

std::auto_ptr<WESignalData> WirelessEtherStateBackoff::processData(WirelessEtherModule *mod,
                                                                      std::auto_ptr<WESignalData> data)
{
    mod->incNoOfRxFrames();
    mod->frameSource = data->sourceName();

    // timer to await for MAC to be free should have already been created
    // now we want to stop the timer and wait for the medium to be idle
    assert(mod->backoffTimer->isScheduled());

    // double probSameSlot = 1-(a->remainingTime()/(SLOTTIME));
    // There is a chance the backoff time will expire on the same time slot as
    // when an incoming frame is received
    // if( (a->remainingTime() >= SLOTTIME)||(uniform(0,1) > probSameSlot) )
    // {
    simtime_t remainingTime = mod->simTime() - mod->backoffTimer->arrivalTime();
    if (remainingTime >= RXTXTURNAROUND)
    {
        // inform queue of interruption
        mod->outputQueue->contentionInterrupted(mod->simTime());

        // pause backoff countdown
        mod->cancelEvent(mod->backoffTimer);

        // get the frame just received
        assert(!mod->inputFrame);
        mod->inputFrame = (WESignalData *) data.get()->dup();   // XXX eliminate dup()

        mod->outputQueue->endIdleCount(mod->simTime());
        mod->outputQueue->startBusyCount(mod->simTime());

        mod->changeState(WirelessEtherStateBackoffReceive::instance());
    }
    // else
    // mod->txFailedReasonCount[1]++;


    return data;
}

void WirelessEtherStateBackoff::readyToSend(WirelessEtherModule *mod)
{
    // Get frame to be sent
    WirelessEtherBasicFrame *frame = mod->outputQueue->getReadyFrame();
    assert(frame);

    mod->outputQueue->endIdleCount(mod->simTime());

    // Place a lock on the frame being sent, apply mainly
    // to QoS queues and update other queue elapsedTime
    mod->outputQueue->sendingReadyFrame(mod->simTime());

    // Calculate tx time
    double d = (double) frame->length();
    simtime_t transmTime = d / BASE_SPEED;

    // tx time should not be smaller than a slot
    assert(transmTime > SLOTTIME);

    // GD Hack: assume not (toDS=1, FromDS=1).
    short st = frame->getFrameControl().subtype;
    if (!((st == ST_CTS) || (st == ST_ACK)))
    {
        wEV  << mod->fullPath() << " \n"
             << " ---------------------------------------------------- \n"
             << " sending a frame to : " << frame->getAddress1() << " WLAN Tx(2): "
             << dynamic_cast<WirelessEtherRTSFrame *>(frame)->getAddress2()
             << " will finish at " << formatTime(mod->simTime() + transmTime)
             << "\n ---------------------------------------------------- \n";
    }
    else
    {
        wEV  << mod->fullPath() << " \n"
             << " ---------------------------------------------------- \n"
             << " sending a frame to : " << frame->getAddress1()
             << " will finish at " << formatTime(mod->simTime() + transmTime)
             << "\n ---------------------------------------------------- \n";
    }

    OPP_Global::ContextSwitcher switchContext(mod);

    // send frame
    mod->sendFrame(frame);
    mod->changeState(WirelessEtherStateSend::instance());

    mod->reschedule(mod->endSendDataTimer, mod->simTime() + transmTime);
}
