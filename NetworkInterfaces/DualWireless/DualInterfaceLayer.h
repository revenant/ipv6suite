#if 0

CURRENTLY OUT OF ORDER

//
// Copyright (C) 2004 CTIE, Monash University 
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

/**
	@file DualInterfaceLayer.h
	@brief Dual NIC
	@author Steve Woon
	@date Ask Steve

*/

#ifndef DUAL_INTERFACE_LAYER_H
#define DUAL_INTERFACE_LAYER_H

#include <omnetpp.h>
#include <list>

#include "cTimerMessageCB.h"
#include "WirelessEtherSignal.h"
#include "WirelessEtherFrame_m.h"

#include "ExpiryEntryList.h"
#include "AveragingList.h"

class IPDatagram;

/*
  @class AccessPointEntry
 This will allow the DualInterface to keep track of surrouding APs
 */
class AccessPointEntry
{
  public:
  
  AccessPointEntry(int listSize = 1);

  // Needed to use ExpiryEntryList
  double expiryTime(void) const { return expire; }
  MACAddress identifier(void) { return address; }

  MACAddress address;  
  int DSChannel;
  // this is updated 
  double expire;
  AveragingList signalStrength;
  AveragingList estAvailBW;
  AveragingList errorPercentage;
  AveragingList avgBackoffTime;
  AveragingList avgWaitTime;  
};

bool operator==(const AccessPointEntry& lhs,
                const AccessPointEntry& rhs);

bool lessThan(AccessPointEntry&, AccessPointEntry&);

/**
 @class DualInterfaceLayer

 @brief Receive input from all interfaces and forward to IP processing module
        Manages which LL interface connects to the network and responsible for handover
 */
 
class DualInterfaceLayer: public cSimpleModule
{
public:
  Module_Class_Members(DualInterfaceLayer, cSimpleModule, 0);

  static const double handoverWaitTime = 5;
  static const double monitorChannelTime = 0.2;
  static const double obtainStatsTime = 1;
  
  virtual void initialize(int);
  virtual void handleMessage(cMessage*);
  virtual void finish();
  virtual int numInitStages(void) const { return 2; }

  // Functions called within handleMessage
  void handleLinkLayerMessage(std::auto_ptr<cMessage>);
  void handleNetworkLayerMessage(std::auto_ptr<cMessage>);
  void handleSignallingMessage(std::auto_ptr<cMessage>);

  void setMonitoringInterface(int);
  void requestConnectionStats(int);
  void monitorNextChannel(void);
  void obtainStats(void);

  bool handoverRequired(void);
  void handoverDecision(void);
  void processMonitorFrames(WESignalData*);
  
private:

  // buffer messages from upper layer
  std::list<cMessage*> buffer;

  // keep track of which interface is associated and which is monitoring
  int connectedLL;
  int monitoringLL;

  // details of current association
  double connectedSignalStrength;
  double connectedErrorPercentage;
  double connectedAvgBackoffTime;
  double connectedAvgWaitTime;
  
  cOutVector* errorPercentageVec;
  cOutVector* avgBackoffTimeVec;
  cOutVector* avgWaitTimeVec;

  // expiry time of ap entries
  double APEntryLifetime;

  // signal strength threshold when handover will be initiated
  double HOThreshPower;

  // channel monitored on monitoring interface
  int monitoringChannel;
  
  Loki::cTimerMessageCB<void>* handoverWaitTimer;
  Loki::cTimerMessageCB<void>* monitorChannelTimer;
  Loki::cTimerMessageCB<void,TYPELIST_1(int)>* settingMonitorMode;
  Loki::cTimerMessageCB<void>* obtainStatsTimer;

  // Keep track of surroudning APs
  ExpiryEntryList<AccessPointEntry> *apList;
  AccessPointEntry associatedAP;
};

#endif

#endif