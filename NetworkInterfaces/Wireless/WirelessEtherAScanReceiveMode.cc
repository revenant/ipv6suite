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
  @file WirelessEtherAScanReceiveMode.cc
  @brief Source file for WEAScanReceiveMode

  @author    Steve Woon
  Eric Wu
*/

#include "sys.h" // Dout
#include "debug.h" // Dout
#include "config.h"

#include "WirelessEtherAScanReceiveMode.h"
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

WEAScanReceiveMode* WEAScanReceiveMode::_instance = 0;

WEAScanReceiveMode* WEAScanReceiveMode::instance()
{
  if (_instance == 0)
    _instance = new WEAScanReceiveMode;

  return _instance;
}

void WEAScanReceiveMode::handleProbeResponse(WirelessEtherModule* mod, WESignalData *signal)
{
  WirelessEtherManagementFrame* probeResponse =
      static_cast<WirelessEtherManagementFrame*>(signal->encapsulatedMsg());

  if (probeResponse->getAddress1() == MACAddress6(mod->macAddressString().c_str()))
  {
      ProbeResponseFrameBody* probeResponseBody =
      static_cast<ProbeResponseFrameBody*>(probeResponse->decapsulate());

    assert(probeResponseBody);
#if L2FUZZYHO // (Layer 2 fuzzy logic handover)
    double avail_bw = (probeResponseBody->getHandoverParameters()).estAvailBW;
    double value = mod->calculateHOValue(signal->power(),avail_bw,mod->bWRequirements);

    Dout(dc::wireless_ethernet|flush_cf, "MAC LAYER: (WIRELESS) "
         << mod->fullPath() << "\n"
         << " ----------------------------------------------- \n"
         << " Probe Response received by: " << mod->macAddressString() << "\n"
         << " from " << probeResponse->getAddress2() << " \n"
         << " SSID: " << probeResponseBody->getSSID() << "\n"
         << " channel: " << probeResponseBody->getDSChannel() << "\n"
         << " rxpower: " << signal->power() << "\n"
         << " bw req: "<< mod->bWRequirements << "\n"
         << " hoValue: " << value << "\n"
         << " ----------------------------------------------- \n");
#else
    Dout(dc::wireless_ethernet|flush_cf, "MAC LAYER: (WIRELESS) "
         << mod->fullPath() << "\n"
         << " ----------------------------------------------- \n"
         << " Probe Response received by: " << mod->macAddressString() << "\n"
         << " from " << probeResponse->getAddress2() << " \n"
         << " SSID: " << probeResponseBody->getSSID() << "\n"
         << " channel: " << probeResponseBody->getDSChannel() << "\n"
         << " rxpower: " << signal->power() << "\n"
         << " ----------------------------------------------- \n");
#endif // L2FUZZYHO
    for ( size_t i = 0; i < probeResponseBody->getSupportedRatesArraySize(); i++ )
      Dout(dc::wireless_ethernet|flush_cf,
           " Supported Rates: \n "
           << " [" << i << "] \t rate: "
           << probeResponseBody->getSupportedRates(i).rate << " \n"
           << " \t supported?" << probeResponseBody->getSupportedRates(i).supported
           << "\n");

      WirelessEtherModule::APInfo apInfo =
      {
          probeResponse->getAddress2(),
        probeResponseBody->getDSChannel(),
        signal->power(),
#if L2FUZZYHO // (Layer 2 fuzzy logic handover)
            value,
#else
            0,
#endif // L2FUZZYHO
        false,
        (probeResponseBody->getHandoverParameters()).estAvailBW,
        (probeResponseBody->getHandoverParameters()).avgErrorRate,
        (probeResponseBody->getHandoverParameters()).avgBackoffTime,
        (probeResponseBody->getHandoverParameters()).avgWaitTime
      };
      mod->insertToAPList(apInfo);

    // send ACK
    WirelessEtherBasicFrame* ack = mod->
      createFrame(FT_CONTROL, ST_ACK, MACAddress6(mod->macAddressString().c_str()),
                  probeResponse->getAddress2());
    WESignalData* ackSignal = encapsulateIntoWESignalData(ack);
    sendAck(mod, ackSignal);
    //delete ack;
    //GD:   Can we do early return here??
    //Enough APs?? Signal Strength, SSID
    //Prematch criteria??
    changeState = false;

    delete probeResponseBody;
  } // endif
}
