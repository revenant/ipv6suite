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
    @file WirelessEtherStateBackoff.h
    @brief Header file for WirelessEtherStateBackoff

    Super class of wireless Ethernet State

    @author Eric Wu
            Steve Woon
            Greg Daley
*/

#ifndef __WIRELESS_ETHER_STATE_BACKOFF_H__
#define __WIRELESS_ETHER_STATE_BACKOFF_H__

#include <memory> //auto_ptr

#include <omnetpp.h>
#include "WEthernet.h"
#include "WirelessEtherState.h"

class WirelessEtherModule;
class WirelessEtherStateIdle;

class WESignalIdle;
class WESignalData;

class WirelessEtherStateBackoff : public WirelessEtherState
{
  friend class WirelessEtherModule;

public:
  static WirelessEtherStateBackoff* instance();

  virtual std::auto_ptr<cMessage> processSignal(WirelessEtherModule* mod, std::auto_ptr<cMessage> msg);

protected:
  WirelessEtherStateBackoff(void) {}

  virtual std::auto_ptr<WESignalIdle> processIdle(WirelessEtherModule* mod, std::auto_ptr<WESignalIdle> idle);
  virtual std::auto_ptr<WESignalData> processData(WirelessEtherModule* mod, std::auto_ptr<WESignalData> data);

  void readyToSend(WirelessEtherModule* mod);

  static WirelessEtherStateBackoff* _instance;
};
#endif
