// -*- C++ -*-
// Copyright (C) 2001, 2003 Monash University, Melbourne, Australia
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
    @file WirelessAccessPoint.h
    @brief Header file for WirelessAccessPoint

    simple implementation of wireless access point module

    @author Eric Wu
*/

#ifndef __WIRELESSACCESSPOINT__
#define __WIRELESSACCESSPOINT__

#include "config.h"

#include <omnetpp.h>
#include "ExpiryEntryList.h"
#include "WirelessEtherModule.h"

class WirelessEtherInterface
{
  public:

  // Needed to use ExpiryEntryList
  double expiryTime(void) const { return expire; }
  MACAddress6 identifier(void) { return address; }

  MACAddress6 address;

  ReceiveMode receiveMode;

    // consecutive failed transmission (dropped frames)
    int consecFailedTrans;

  // this is updated
  double expire;

  // enumerations can be found in WirelessEtherFrameBody.msg
  ReasonCode reasonCode;

  // enumerations can be found in WirelessEtherFrameBody.msg
  StatusCode statusCode;
};

bool operator==(const WirelessEtherInterface& lhs,
                const WirelessEtherInterface& rhs);

bool operator!=(const WirelessEtherInterface& lhs,
                const WirelessEtherInterface& rhs);

bool operator<(const WirelessEtherInterface& lhs,
                             const WirelessEtherInterface& rhs);

class WESignalData;
class WirelessEtherBridge;
class FrameBody;
class MACAddress6;
class WirelessEtherBridge;

extern const WirelessEtherInterface UNSPECIFIED_WIRELESS_ETH_IFACE;

class WirelessAccessPoint : public WirelessEtherModule
{
  friend class WEReceiveMode;
  friend class WEAPReceiveMode;
  friend class WirelessEtherBridge;
  friend class WirelessEtherStateIdle;
  friend class WirelessEtherStateAwaitACK;

public:
  Module_Class_Members(WirelessAccessPoint, WirelessEtherModule, 0);

  virtual void initialize(int stage);
  virtual void handleMessage(cMessage* msg);
  virtual void finish(void);
  virtual int  numInitStages() const  {return 2;}

  virtual void idleNetworkInterface(void);

  // frames from bridge module
  virtual void receiveData(std::auto_ptr<cMessage> msg);
  virtual FrameBody* createFrameBody(WirelessEtherBasicFrame* f);

  WirelessEtherInterface findIfaceByMAC(MACAddress6 mac);
  void updateConsecutiveFailedCount();
  double getEstAvailBW(void) { return estAvailBW; }



private:
  void addIface(MACAddress6 mac, ReceiveMode receiveMode);
  void setIfaceStatus(MACAddress6 mac, StatusCode);
  void sendBeacon(void);

private:
  ExpiryEntryList<WirelessEtherInterface> *ifaces;

  int consecFailedTransLimit; //consecutive failed tranmission limit b4 entry is removed
  double beaconPeriod;
  double authWaitEntryTimeout;
  double authEntryTimeout;
  double assEntryTimeout;
  WirelessEtherBridge* bridge;

  // Storing values for statistics
  virtual void updateStats(void);
  TimeAverageReading usedBW; // average stored in bytes/sec
  double estAvailBW; //Mbit/sec

  cOutVector *estAvailBWVec;

};

#endif // __WIRELESSACCESSPOINT__
