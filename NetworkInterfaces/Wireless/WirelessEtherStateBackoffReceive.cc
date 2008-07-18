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
    @file WirelessEtherStateBackoffReceive.cc
    @brief Header file for WirelessEtherStateBackoffReceive

    Super class of wireless Ethernet State

    @author Greg Daley
            Eric Wu
*/


#include "sys.h"
#include "debug.h"

#include <iostream>
#include <iomanip>

#include "WirelessEtherStateBackoffReceive.h"
#include "WirelessEtherModule.h"
#include "WirelessEtherSignal_m.h"
#include "WirelessEtherStateBackoff.h"
#include "WirelessEtherStateIdle.h"
#include "WirelessEtherAScanReceiveMode.h"
#include "WEQueue.h"

WirelessEtherStateBackoffReceive *WirelessEtherStateBackoffReceive::_instance = 0;

WirelessEtherStateBackoffReceive *WirelessEtherStateBackoffReceive::instance()
{
    if (_instance == 0)
        _instance = new WirelessEtherStateBackoffReceive;

    return _instance;
}

std::auto_ptr<cMessage> WirelessEtherStateBackoffReceive::processSignal(WirelessEtherModule *mod,
                                                                           std::auto_ptr<cMessage> msg)
{
    return WirelessEtherStateReceive::processSignal(mod, msg);
}

std::auto_ptr<WESignalIdle> WirelessEtherStateBackoffReceive::processIdle(WirelessEtherModule *mod,
                                                                             std::auto_ptr<WESignalIdle> idle)
{
    return WirelessEtherStateReceive::processIdle(mod, idle);
}

std::auto_ptr<WESignalData> WirelessEtherStateBackoffReceive::processData(WirelessEtherModule *mod,
                                                                             std::auto_ptr<WESignalData> data)
{
    return WirelessEtherStateReceive::processData(mod, data);
}

void WirelessEtherStateBackoffReceive::changeNextState(WirelessEtherModule *mod)
{
    mod->outputQueue->endBusyCount(mod->simTime());
    mod->outputQueue->startIdleCount(mod->simTime());

    // If frame received causes a handover, go to idle state and ditch current planned tx
    if (mod->prbEnergyScanNotifier && mod->prbEnergyScanNotifier->isScheduled())
    {
        wEV << currentTime() << " sec, " << mod->fullPath() << ": handover triggered! go back to idle state\n";

        mod->changeState(WirelessEtherStateIdle::instance());
        return;
    }

    // Resume backoff
    assert(!mod->backoffTimer->isScheduled());
    mod->scheduleAt(mod->simTime() + mod->outputQueue->getTimeToSend(), mod->backoffTimer);

    wEV << currentTime() << " sec, " << mod->fullPath() << ": resume backing off and scheduled to cease backoff in " << mod->outputQueue->getTimeToSend() << " seconds\n";

    mod->outputQueue->startingContention(mod->simTime());
    mod->changeState(WirelessEtherStateBackoff::instance());
}
