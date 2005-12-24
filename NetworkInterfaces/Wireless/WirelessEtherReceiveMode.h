// -*- C++ -*-// Copyright (C) 2001 Monash University, Melbourne, Australia
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
    @file WirelessEtherReceiveMode.h
    @brief Header file for WEReceiveMode

    Super class of Wireless Ethernet Receive Mode

    @author    Steve Woon
          Eric Wu
*/

#ifndef __WIRELESS_ETHER_RECEIVE_MODE_H__
#define __WIRELESS_ETHER_RECEIVE_MODE_H__


class WESignalData;
class WirelessEtherModule;
class WirelessAccessPoint;
class WirelessEtherBasicFrame;

class WEReceiveMode
{
public:
  virtual void decodeFrame(WirelessEtherModule* mod, WESignalData* signal);

  // handled by mobile station
  virtual void handleProbeResponse(WirelessEtherModule* mod, WESignalData* signal) {}
  virtual void handleAssociationResponse(WirelessEtherModule* mod, WESignalData* signal) {}
  virtual void handleReAssociationResponse(WirelessEtherModule* mod, WESignalData* signal) {}
  virtual void handleDisAssociation(WirelessEtherModule* mod, WESignalData* signal) {}
  virtual void handleBeacon(WirelessEtherModule* mod, WESignalData* signal) {}
  virtual void handleDeAuthentication(WirelessEtherModule* mod, WESignalData* signal) {}

  // handled by access point
  virtual void handleProbeRequest(WirelessAccessPoint* mod, WESignalData* signal) {}
  virtual void handleAssociationRequest(WirelessAccessPoint* mod, WESignalData* signal) {}
  virtual void handleReAssociationRequest(WirelessAccessPoint* mod, WESignalData* signal) {}

  // handled by both
  virtual void handleData(WirelessEtherModule* mod, WESignalData* signal) {}
  virtual void handleAck(WirelessEtherModule* mod, WESignalData* signal) {}
  virtual void handleAuthentication(WirelessEtherModule* mod, WESignalData* signal) {}

  static WEReceiveMode* instance();

  void finishFrameTx(WirelessEtherModule* mod);
protected:
  bool changeState;
  static WEReceiveMode* _instance;
  WEReceiveMode(void) {}

  void scheduleAck(WirelessEtherModule* mod, WirelessEtherBasicFrame* ack);
};


#endif // __WIRELESS_ETHER_RECEIVE_MODE_H__
