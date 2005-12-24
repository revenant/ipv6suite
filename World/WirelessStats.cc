//
// Copyright (C) 2002 CTIE, Monash University
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

#include "sys.h"
#include "debug.h"

#include <sstream>

#include "WirelessStats.h"
#include "WorldProcessor.h"
#include "opp_utils.h"

#ifdef USE_MOBILITY
#include "WirelessEtherModule.h"
#include "WirelessAccessPoint.h"
#include "wirelessethernet.h"
#endif //USE_MOBILITY


Define_Module( WirelessStats );


void WirelessStats::initialize()
{
    // XXX A bit of a hack
    //BASE_SPEED is in wirelessEthernet.h/cc unit
//    BASE_SPEED = int(par("wlan_speed").doubleValue() * 1000 * 1000);
    Dout(dc::notice, " 802.11b wlan is at rate of "<<par("wlan_speed").longValue()<<" bps");

    balanceIndexVec.setName("balanceIndex");

    // Timer to update statistics
    scheduleAt(simTime()+1, new cMessage("updateStats"));
}

void WirelessStats::handleMessage(cMessage* msg)
{
  updateStats();

  // allow sim to quit if nothing of interest is happening.
  if (!simulation.msgQueue.empty())
    scheduleAt(simTime()+1, msg);
}

void WirelessStats::finish()
{
}

void WirelessStats::updateStats()
{
  cModule *wpmod = simulation.moduleByPath("worldProcessor");
  WorldProcessor *wp = check_and_cast<WorldProcessor*>(wpmod);
  const ModuleList& modList = wp->entities();

  double balanceIndex =0, loadSum=0, loadSquaredSum=0, n=0, usedBW;

  for (size_t i = 0; i < modList.size(); i++)
  {
    //Get the interface's link layer
    cModule* interface = modList[i]->containerModule()->parentModule()->parentModule(); // XXX Ugh!! hardcoded model structure! --AV
    cModule* phylayer = interface->gate("wlin")->toGate()->ownerModule(); // XXX if not connected ==> crash! --AV
    cModule* linkLayer =  phylayer->gate("linkOut")->toGate()->ownerModule(); // XXX if not connected ==> crash! --AV

    if(linkLayer != NULL)
    {
      if (std::string(linkLayer->par("NWIName")) == "WirelessAccessPoint")
      {
        WirelessAccessPoint* a = static_cast<WirelessAccessPoint*>(linkLayer->submodule("networkInterface"));
        usedBW = (par("wlan_speed").longValue()/1000000)-a->getEstAvailBW();
        loadSum += usedBW;
        loadSquaredSum += usedBW*usedBW;
        n++;
      }
    }
  }
  balanceIndex = (n*loadSquaredSum > 0) ? (loadSum*loadSum)/(n*loadSquaredSum):0;
  balanceIndexVec.record(balanceIndex);
}

