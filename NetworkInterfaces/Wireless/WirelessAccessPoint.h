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



#include <omnetpp.h>
#include <vector>
#include "WirelessEtherModule.h"

class WirelessEtherInterface
{
  public:
  WirelessEtherInterface();

  // Needed to use ExpiryEntryList
  double expiryTime(void) const { return expire; }
  MACAddress6 identifier(void) { return address; }

  MACAddress6 address;

  ReceiveMode receiveMode;

  // consecutive failed transmission (dropped frames)
  int consecFailedTrans;
  int currentSequence;

  // this is updated
  double expire;

  // enumerations can be found in WirelessEtherFrameBody.msg
  ReasonCode reasonCode;

  // enumerations can be found in WirelessEtherFrameBody.msg
  StatusCode statusCode;
};

// SWOON HACK: To find achievable throughput
typedef struct mobileStats
{
    std::string name;
    int appType;
    double lastUpdateTime;
    double probTxInSlot;
    cOutVector* achievableThroughput;
    cOutVector* probOfTxInSlotVec;
    cStdDev* avgAchievableThroughput;
    double avgFrameSize;
    double probSuccessfulTx;
    double probSuccessfulTxVI;
    double probSuccessfulTxVO;
} MobileStats;

bool operator==(const WirelessEtherInterface& lhs,
                const WirelessEtherInterface& rhs);

bool operator!=(const WirelessEtherInterface& lhs,
                const WirelessEtherInterface& rhs);

bool operator<(const WirelessEtherInterface& lhs,
                             const WirelessEtherInterface& rhs);

std::ostream& operator<<(std::ostream& os, const WirelessEtherInterface& wif);

class WESignalData;
class WirelessEtherBridge;
class FrameBody;
class MACAddress6;
class WirelessEtherBridge;

extern const WirelessEtherInterface UNSPECIFIED_WIRELESS_ETH_IFACE;
template<typename T> class ExpiryEntryListSignal;

class WirelessAccessPoint : public WirelessEtherModule
{
  friend class WEReceiveMode;
  friend class WEAPReceiveMode;
  friend class WirelessEtherBridge;
  friend class WirelessEtherStateIdle;
  friend class WirelessEtherStateAwaitACK;

public:
  Module_Class_Members(WirelessAccessPoint, WirelessEtherModule, 0);

  ~WirelessAccessPoint();
  virtual void initialize(int stage);
  virtual void handleMessage(cMessage* msg);
  virtual void finish(void);
  virtual int  numInitStages() const  {return 2;}

  // frames from bridge module
  virtual void receiveData(std::auto_ptr<cMessage> msg);
  virtual FrameBody* createFrameBody(WirelessEtherBasicFrame* f);

  WirelessEtherInterface findIfaceByMAC(MACAddress6 mac);
  void updateConsecutiveFailedCount(WirelessEtherBasicFrame*);
  double getEstAvailBW(void) { return estAvailBW; }

private:
  void addIface(MACAddress6 mac, ReceiveMode receiveMode, int sequenceNo);
  void setIfaceStatus(MACAddress6 mac, StatusCode);
  virtual void sendBeacon(void);
  // SWOON HACK: To find achievable throughput
  void estimateThroughput(double&, double&, double&);
  void updateMStats(std::string, int, double, double);

private:
  typedef ExpiryEntryListSignal<std::vector<WirelessEtherInterface> > WIL;
  WIL*ifaces;

  // SWOON HACK: To find achievable throughput
  std::list<MobileStats*> mStats;

  int consecFailedTransLimit; //consecutive failed tranmission limit b4 entry is removed
  double beaconPeriod;
  double authWaitEntryTimeout;
  double authEntryTimeout;
  double assEntryTimeout;
  WirelessEtherBridge* bridge;

  // Storing values for statistics
  virtual void updateStats(void);
  TimeAverageReading usedBW; // average stored in bytes/sec
  double RxDataBWBE;
  double RxDataBWVI;
  double RxDataBWVO;
  double TxDataBWBE;
  double TxDataBWVI;
  double TxDataBWVO;
  double collDurationBE;
  double durationBE;
  double durationVI;
  double durationVO;
  double durationDataBE;
  double durationDataVI;
  double durationDataVO;
  double currentCollDurationBE;
  double currentDurationBE;
  double currentDurationVI;
  double currentDurationVO;
  double estAvailBW; //Mbit/sec
  double newNoOfVI;
  double newNoOfVO;

  cOutVector *estAvailBWVec;
  cOutVector *avgCollDurationVec;
  cOutVector *achievableThroughputBEVec;
  cOutVector *achievableThroughputVIVec;
  cOutVector *achievableThroughputVOVec;
  cOutVector *achievableTpTotalVec;
  cOutVector *collisionVec;
  cOutVector *idleVec;
  cOutVector *probOfTxInSlotBEVec;
  cOutVector *probOfTxInSlotVIVec;
  cOutVector *probOfTxInSlotVOVec;
  cOutVector *noOfVIVec;
  cOutVector *noOfVOVec;

  cStdDev* predUsedBWVIStat;
  cStdDev* predUsedBWVOStat;
  cStdDev* predCollBWVIStat;
  cStdDev* predCollBWVOStat;

  cStdDev* noOfVIStat;
  cStdDev* noOfVOStat;
  cStdDev* idleBWStat;
  cStdDev* collisionBWStat;
  cStdDev* successBWStat;
  cStdDev* dataTpBWStat;
  cStdDev* avgTxDataBWBE;
  cStdDev* avgTxDataBWVI;
  cStdDev* avgTxDataBWVO;
  cStdDev* avgDataBWBE;
  cStdDev* avgDataBWVI;
  cStdDev* avgDataBWVO;
  cStdDev* avgAchievableThroughputBE;
  cStdDev* avgAchievableThroughputVI;
  cStdDev* avgAchievableThroughputVO;
  cStdDev* avgCollDurationStat;
  cStdDev* usedBWStat;
  cStdDev* frameSizeTxStat;     //average frame size tx
  cStdDev* frameSizeRxStat;     //average frame size rx
  cStdDev* avgFrameSizeTxStat;  //average frame size tx for whole sim
  cStdDev* avgFrameSizeRxStat;  //average frame size rx for whole sim
  cOutVector* frameSizeTxVec;   //vector of frame size tx each second
  cOutVector* frameSizeRxVec;   //vector of frame size rx each second

};

#endif // __WIRELESSACCESSPOINT__

