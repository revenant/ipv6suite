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
    @file WirelessEtherStateReceive.h
    @brief Header file for WirelessEtherStateReceive

    Super class of wireless Ethernet State

    @author Greg Daley
            Eric Wu
*/

#ifndef __WIRELESS_ETHER_STATE_RECEIVE_H__
#define __WIRELESS_ETHER_STATE_RECEIVE_H__

#include <memory> //auto_ptr

#ifndef BOOST_CAST_HPP
#include <boost/cast.hpp>
#endif //BOOST_CAST_HPP

#include <omnetpp.h>
#include "WEthernet.h"
#include "WirelessEtherState.h"

class WirelessEtherModule;

class WESignalIdle;
class WESignalData;
class WESignalACK;

class WirelessEtherStateReceive : public WirelessEtherState
{
  friend class WirelessEtherStateIdle;

public:
  static WirelessEtherStateReceive* instance();

  virtual std::auto_ptr<cMessage> processSignal(WirelessEtherModule* mod, std::auto_ptr<cMessage> msg);

  void sendAck(WirelessEtherModule* mod, WESignalData* ack);

  void endSendingAck(WirelessEtherModule* mod);

  virtual void changeNextState(WirelessEtherModule* mod);
protected:
  WirelessEtherStateReceive(void) {}

  virtual std::auto_ptr<WESignalIdle> processIdle(WirelessEtherModule* mod, std::auto_ptr<WESignalIdle> idle);
  virtual std::auto_ptr<WESignalData> processData(WirelessEtherModule* mod, std::auto_ptr<WESignalData> data);

  static WirelessEtherStateReceive* _instance;
};
#endif
