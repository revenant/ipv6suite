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
    @file WirelessEtherState.cc
    @brief Header file for WirelessEtherState

    Super class of wireless Ethernet State

    @author Greg Daley
            Eric Wu
*/

#include "sys.h"
#include "debug.h"

#include <iostream>
#include <iomanip>

#include "WirelessEtherState.h"
#include "WirelessEtherModule.h"
#include "WirelessEtherSignal_m.h"
#include "WirelessEtherFrame_m.h"
#include "WirelessEtherFrameBody_m.h"

#include "WirelessEtherStateIdle.h"
#include "WirelessEtherStateSend.h"
#include "WirelessEtherStateBackoff.h"
#include "WirelessEtherStateReceive.h"
#include "WirelessEtherStateBackoffReceive.h"
#include "WirelessEtherStateAwaitACK.h"
#include "WirelessEtherStateAwaitACKReceive.h"
#include "WirelessEtherStateBackoff.h"

WirelessEtherState::~WirelessEtherState(){}
std::auto_ptr<cMessage> WirelessEtherState::processSignal(WirelessEtherModule *mod,
                                                             std::auto_ptr<cMessage> msg)
{
    // Message from Upper layer
    if (msg.get()->arrivedOn(mod->outputQueueInGate()))
    {
        mod->receiveData(msg);
        return msg;
    }
    // Message from external signalling
    if (strcmp(msg.get()->arrivalGate()->name(), "extSignalIn") == 0)
    {
        wEV << currentTime() << " sec, " << mod->fullPath() << ": external signal received\n";

        mod->receiveSignal(msg);
        return msg;
    }

    WESignal *signal = check_and_cast<WESignal *>(msg.get());

    if (signal->channelNum() != mod->getChannel())
        return msg;

    // debug
    printMsg(mod, signal);
    ///////

    if (dynamic_cast<WESignalData *>(signal))
        msg.reset((processData(mod, auto_downcast < WESignalData > (msg))).release());
    else if (dynamic_cast<WESignalIdle *>(signal))
        msg.reset((processIdle(mod, auto_downcast < WESignalIdle > (msg))).release());
    else
        assert(false);
    return msg;
}

std::auto_ptr<WESignalIdle> WirelessEtherState::processIdle(WirelessEtherModule *mod,
                                                               std::auto_ptr<WESignalIdle> idle)
{
    return idle;
}

std::auto_ptr<WESignalData> WirelessEtherState::processData(WirelessEtherModule *mod,
                                                               std::auto_ptr<WESignalData> data)
{
    return data;
}

// debug message
void WirelessEtherState::printMsg(WirelessEtherModule *mod, WESignal * signal)
{
    std::string s = signal->className();

    if (mod->currentState() == WirelessEtherStateIdle::instance())
        wEV << currentTime() << " sec, " << mod->fullPath() << ": " << s.c_str() << " in WIRELESS_IDLE state\n";

    else if (mod->currentState() == WirelessEtherStateSend::instance())
        wEV << currentTime() << " sec, " << mod->fullPath() << ": " << s.c_str() << " in WIRELESS_SEND state\n";

    else if (mod->currentState() == WirelessEtherStateBackoff::instance())
        wEV << currentTime() << " sec, " << mod->fullPath() << ": " << s.c_str() << " in WIRELESS_BACKOFF state\n";

    else if (mod->currentState() == WirelessEtherStateAwaitACK::instance())
        wEV << currentTime() << " sec, " << mod->fullPath() << ": " << s.c_str() << " in WIRELESS_AWAITACK state\n";

    else if (mod->currentState() == WirelessEtherStateAwaitACKReceive::instance())
        wEV << currentTime() << " sec, " << mod->fullPath() << ": " << s.c_str() << " in WIRELESS_AWAITACKRECEIVE state\n";

    else if (mod->currentState() == WirelessEtherStateBackoffReceive::instance())
        wEV << currentTime() << " sec, " << mod->fullPath() << ": " << s.c_str() << " in WIRELESS_BACKOFFRECEIVE state\n";

    else if (mod->currentState() == WirelessEtherStateReceive::instance())
        wEV << currentTime() << " sec, " << mod->fullPath() << ": " << s.c_str() << " in WIRELESS_RECEIVE state\n";

    else
        assert(false);
}
