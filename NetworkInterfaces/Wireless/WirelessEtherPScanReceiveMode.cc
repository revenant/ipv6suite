// -*- C++ -*-// Copyright (C) 2001 Monash University, Melbourne, Australia
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

/*    @file WirelessEtherPScanReceiveMode.cc
    @brief Source file for WEPScanReceiveMode
    @author    Steve Woon          Eric Wu
*/

#include <sys.h> // Dout#include "debug.h" // Dout


#include "WirelessEtherPScanReceiveMode.h"#include "WirelessEtherState.h"
#include "WirelessEtherModule.h"
#include "WirelessEtherSignal.h"
#include "WirelessEtherFrame_m.h"
#include "WirelessEtherFrameBody_m.h"

#include "WirelessEtherStateIdle.h"
#include "WirelessEtherStateSend.h"
#include "WirelessEtherStateBackoff.h"
#include "WirelessEtherStateReceive.h"
#include "WirelessEtherStateAwaitACK.h"
#include "WirelessEtherStateBackoffReceive.h"
#include "WirelessEtherStateBackoff.h"

WEPScanReceiveMode* WEPScanReceiveMode::_instance = 0;

WEPScanReceiveMode* WEPScanReceiveMode::instance()
{
  if (_instance == 0)
    _instance = new WEPScanReceiveMode;

  return _instance;
}

void WEPScanReceiveMode::handleBeacon(WirelessEtherModule* mod, WESignalData* signal)
{
    WirelessEtherManagementFrame* beacon =
         static_cast<WirelessEtherManagementFrame*>(signal->data());

    if(mod->isFrameForMe(static_cast<WirelessEtherBasicFrame*>(beacon)))
    {
      BeaconFrameBody* beaconBody =
        static_cast<BeaconFrameBody*>(beacon->decapsulate());

        assert(beaconBody);

        Dout(dc::wireless_ethernet|flush_cf, "MAC LAYER: (WIRELESS) "
         << mod->fullPath() << "\n"
         << " ----------------------------------------------- \n"
         << " Beacon received by: "
         << mod->macAddressString() << "\n"
         << " from " << beacon->getAddress2() << " \n"
         << " rx power: " << signal->power() << "\n"
         << " ----------------------------------------------- \n");

    //TODO: need to check supported rates
        WirelessEtherModule::APInfo apInfo =
          {
            beacon->getAddress2(),
            beaconBody->getDSChannel(),
            signal->power(),
            0,
            false,
            (beaconBody->getHandoverParameters()).estAvailBW,
            (beaconBody->getHandoverParameters()).avgErrorRate,
            (beaconBody->getHandoverParameters()).avgBackoffTime,
            (beaconBody->getHandoverParameters()).avgWaitTime
          };
        mod->insertToAPList(apInfo);

        delete beaconBody;
    } // endif
}

