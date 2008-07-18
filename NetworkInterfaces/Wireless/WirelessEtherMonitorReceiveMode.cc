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
    @file WirelessEtherMonitorReceiveMode.cc
    @brief Source file for WEMonitorReceiveMode

    @author    Steve Woon
          Eric Wu
*/

#include <sys.h>
#include "debug.h"
#include <iomanip>

#include "WirelessEtherMonitorReceiveMode.h"
#include "WirelessEtherState.h"
#include "WirelessEtherModule.h"
#include "WirelessEtherSignal_m.h"
#include "WirelessEtherFrame_m.h"
#include "WirelessEtherFrameBody_m.h"

#include "WirelessEtherStateIdle.h"
#include "WirelessEtherStateSend.h"
#include "WirelessEtherStateBackoff.h"
#include "WirelessEtherStateReceive.h"
#include "WirelessEtherStateAwaitACK.h"
#include "WirelessEtherStateBackoffReceive.h"
#include "WirelessEtherStateBackoff.h"

WEMonitorReceiveMode *WEMonitorReceiveMode::_instance = 0;

WEMonitorReceiveMode *WEMonitorReceiveMode::instance()
{
    if (_instance == 0)
        _instance = new WEMonitorReceiveMode;

    return _instance;
}

void WEMonitorReceiveMode::decodeFrame(WirelessEtherModule *mod, WESignalData *signal)
{
    // need to print or store contents of Frame.
    // pass frame up to higher layer
    WirelessEtherBasicFrame *frame = check_and_cast<WirelessEtherBasicFrame *>(signal->encapsulatedMsg());

    FrameControl frameControl = frame->getFrameControl();

    wEV << currentTime() << " " << mod->fullPath() << "Frame Control: "<<frameControl.subtype<<endl << "\n";

    mod->sendMonitorFrameToUpperLayer(signal);

    static_cast<WirelessEtherStateReceive *>(mod->currentState())->changeNextState(mod);
}
