// -*- C++ -*-
// Copyright (C) 2001, 2004 Monash University, Melbourne, Australia
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
    @file WirelessEtherModule.h
    @brief Header file for WirelessEtherModule

    Simple implementation of wireless Ethernet module

    @author Eric Wu
*/

#ifndef __WIRELESSETHERMODULE__
#define __WIRELESSETHERMODULE__

#include "config.h"

#include <memory> //auto_ptr

#ifndef BOOST_CAST_HPP
#include <boost/cast.hpp>
#endif //BOOST_CAST_HPP

#ifndef BOOST_SHARED_PTR_HPP
#include <boost/shared_ptr.hpp>
#endif //BOOST_SHARED_PTR_HPP

#include <string>
#include <vector> // for supported rates

#include "cTimerMessageCB.h"

#include <omnetpp.h>
#include <list>

#include "LinkLayerModule.h"
#include "wirelessethernet.h"
#include "MACAddress6.h"

#include "WirelessEtherFrame_m.h"
#include "WirelessEtherFrameBody_m.h"
#include "WirelessExternalSignal_m.h"

#include "AveragingList.h"

class WEReceiveMode;
class WEAScanReceiveMode;
class WEPScanReceiveMode;
class WEAuthenticationReceiveMode;
class WEAssociationReceiveMode;
class WEDataReceiveMode;
class WEMonitorReceiveMode;

class WirelessEtherState;
class WESignal;
class WESignalData;
class WirelessEtherFrame;

class WirelessAccessPoint;
class MobilityHandler;
class WorldProcessor;


// support rates of the wireless network interface
typedef std::vector<SupportedRatesElement> SupportedRates;

// output queue for which frames are prepared to send to network
//typedef std::list<WESignalData*> OutputBuffer;

namespace XMLConfiguration
{
  class IPv6XMLParser;
  class IPv6XMLWrapManager;
  class XMLOmnetParser;
}

enum TrigVals{
  LinkUP = 0,
  LinkDOWN = 1,
  NumTrigVals = 2
};

typedef Loki::cTimerMessageCB<void, TYPELIST_1(double)> L2DelayTmr;

class WirelessEtherModule : public LinkLayerModule
{
  friend class XMLConfiguration::IPv6XMLParser;
  friend class XMLConfiguration::IPv6XMLWrapManager;
  friend class XMLConfiguration::XMLOmnetParser;
  friend class WEReceiveMode;
  friend class WEAScanReceiveMode;
  friend class WEPScanReceiveMode;
  friend class WEAuthenticationReceiveMode;
  friend class WEAssociationReceiveMode;
  friend class WEDataReceiveMode;
  friend class WEMonitorReceiveMode;
  friend class WEAPReceiveMode;
  friend class WirelessEtherStateAwaitACKReceive;
  friend class WirelessEtherStateAwaitACK;
  friend class WirelessEtherStateBackoff;
  friend class WirelessEtherStateReceive;
  friend class WirelessEtherStateSend;
  friend class WirelessEtherStateIdle;

public:
  Module_Class_Members(WirelessEtherModule, LinkLayerModule, 0);

  virtual void initialize(int stage);
  virtual void handleMessage(cMessage* msg);
  virtual void finish();
  virtual int numInitStages(void) const { return 2; }

  void receiveSignal(std::auto_ptr<cMessage> msg);

  // receive packet from other layer besides physical layer e.g. upper
  // layer or peer layer
  virtual void receiveData(std::auto_ptr<cMessage> msg);

  //void setLayer2Trigger(TFunctorBaseA<cTimerMessage>* cb);
  //virtual void setLayer2Trigger(cTimerMessage* trig);
  virtual void setLayer2Trigger( cTimerMessage* trig, enum TrigVals v=LinkUP);

  //cTimerMessage* getLayer2Trigger(void){ return l2Trigger[0]; }
  cTimerMessage* getLayer2Trigger(enum TrigVals v=LinkUP){ return l2Trigger[v]; }

  void setLayer2DelayRecorder( L2DelayTmr*  recorder ) {l2DelayRecorder = recorder;}
   L2DelayTmr* getLayer2DelayRecorder(void){ return l2DelayRecorder; }

  void setLayer2LinkDownRecorder( L2DelayTmr*  recorder ) {l2LinkDownRecorder = recorder;}
  L2DelayTmr* getLayer2LinkDownRecorder(void){ return l2LinkDownRecorder; }

  // reset all current CSMA/CA-related values back to initial state
  void reset(void);

  // attributes

  int getChannel(void){ return channel; }
  double getPower(void) { return txpower; } // mW
  double getThreshPower(void) { return threshpower; } //mW
  double getHOThreshPower(void) { return hothreshpower; } //mW

  bool isAP() { return apMode; }

  virtual void idleNetworkInterface(void);

  long procDelay(void) { return procdelay; }

  const unsigned int* macAddress(void);
  std::string macAddressString(void);

    std::list<WirelessEtherBasicFrame*> offlineOutputBuffer;
  std::list<WESignalData*> outputBuffer;
  WESignalData* inputFrame;

  // list to store signal strength readings
  AveragingList *signalStrength;

  // input gate of the Output Queue for incoming packet from other layer or peer L2 modules
  virtual int outputQueueInGate() { return findGate("netwIn"); }

  // output gate of the Input Queue to other layer or peer L2 modules
  virtual int inputQueueOutGate() { return findGate("netwOut"); }

  // CSMA-CA

  // check if the sending frame is a broadcast frame, if it is then go
  // back to idle
  bool handleSendingBcastFrame(void);

  int contentionWindowPower(void) { return contWindowPwr; }
  void incContentionWindowPower(void)
    {
      assert(contWindowPwr >= 5);
      if ( contWindowPwr < 10 )
        contWindowPwr++;
    }
  void resetContentionWindowPower(void) { contWindowPwr = 5; }

  double backoffTime;

  void sendFrame(WESignal* msg);
  void sendEndOfFrame();

  WirelessEtherState* currentState() const { return _currentState; }
  WEReceiveMode* currentReceiveMode() const { return _currentReceiveMode; }
  void changeState(WirelessEtherState* state) { _currentState = state; }

  void incrementRetry(void) { retry++; }
  unsigned int getRetry(void) const { return retry; }
  void resetRetry(void) { retry = 0; }
  unsigned int  getMaxRetry(void) const { return maxRetry; }

  void scanNextChannel(void);
  void sendSuccessSignal(void);
  void sendStatsSignal(void);
  void startMonitorMode(void);
  void restartScanning(void);

  // check if the frame belongs to me
  virtual bool isFrameForMe(WirelessEtherBasicFrame*);

  virtual void sendToUpperLayer(WirelessEtherBasicFrame*);

  void sendMonitorFrameToUpperLayer(WESignalData*);
  // related to Mobile MLDv2
#if MLDV2
  void sendGQtoUpperLayer();
#endif
  // self timer mssages

  void addTmrMessage(cTimerMessage* msg) { tmrs.push_back(msg); }
  cTimerMessage* getTmrMessage(const int& messageID);

  void cancelAllTmrMessages(void);

  void decodeFrame(WESignalData* signal);

  void incrementSequenceNumber(void)
  {
    sequenceNumber = static_cast<unsigned short>((sequenceNumber+1)%4096);
  }

  bool fastActiveScan(void) { return fastActScan; }

  bool scanShortCircuit(void) { return scanShortCirc; }

  double wirelessRange() const
  {
    if (!_wirelessRange)
    {
      _wirelessRange = pow((double)10, (double)((-threshpower+(10*log10(txpower))-40)/(10*pLExp)));
    }
    return _wirelessRange;
  }

  void incNoOfRxFrames(void) { noOfRxFrames++; }

  void decNoOfRxFrames(void)
  {
    // noOfRxFrames is already 0, means that module has only received
    // the end of a frame (half of frame), in which case set inputFrame = 0
    // to prevent decoding it.
    if(noOfRxFrames)
      noOfRxFrames--;
    else
      inputFrame=0;
  }

  void resetNoOfRxFrames(void) { noOfRxFrames = 0; }

  int getNoOfRxFrames(void) const { return noOfRxFrames; }

  void makeOfflineBufferAvailable(void);

#if L2FUZZYHO // (Layer 2 fuzzy logic handover)
    double calculateHOValue(double rxpower, double ap_avail_bw, double bw_req);
#endif // L2FUZZYHO

protected:

  void baseInit(int stage);

  class TimeAverageReading
    {
        public:
            double sampleTotal;
            double sampleTime;
            double average;
    };

  // ----------
  // general attributes
  // ----------

  MACAddress6 address;
  bool apMode;
  bool adhocMode; // parsed from XML
  std::string ssid; // parsed from XML
  SupportedRates rates; // Mbps; parsed from XML (NOT IMPLEMENTED YET!)
  double pLExp; // path loss exponent
  double pLStdDev; // dB; standard deviation of Gauss. dist. for path loss model
  double txpower; // mW; parsed from XML
  double threshpower; // dBm; threshold power; parsed from XML
  double hothreshpower; // dBm; handover threshold power; parsed from XML
  double probeEnergyTimeout; // parsed from XML
  double probeResponseTimeout; // parsed from XML
  double authenticationTimeout; // parsed from XML
  double associationTimeout; // parsed from XML
  unsigned int maxRetry; // parsed from XML
  bool fastActScan; // parsed from XML
  bool scanShortCirc; // parsed from XML
  bool crossTalk;  // parsed from XML
  bool shadowing; // parsed from XML
  std::string chanNotToScan; // parsed from XML
  double bWRequirements; // parsed from XML : rating from 0-1
  bool statsVec; // parsed from XML
  bool activeScan; // parsed from XML
  double channelScanTime; //parsed from XML

  bool ackReceived;
  int channel;
  unsigned int retry;

  long procdelay; // ms

  int noOfRxFrames; //used for collision detection
  std::string frameSource; // name of module its receiving a frame from

  // ---------
  // specific implementation wise attributes
  // ---------

  //sequence number
  unsigned short sequenceNumber;

  MobilityHandler* _mobCore;
  WorldProcessor* wproc; // aware of all mobile nodes

  WESignalData* tempOutputFrame; // frame in process of sending

  std::list<cTimerMessage*> tmrs;

  typedef struct destInfo
  {
    WirelessEtherModule* mod; // module to send to
    int index;                // node interface index
    int channel;
    double rxPower;
  }
  DestInfo;

  /**
     @brief store modules which need to be sent the end of a frame

     Using shared_ptr so whenever we do idelDest.clear() all the pointed to
     entities are automatically deleted.

     @todo DestInfo does not really need to be a pointer does it? (Johnny)
  */
  typedef std::list<boost::shared_ptr<DestInfo> > IdleDest;
  IdleDest idleDest;
  typedef IdleDest::iterator IDIT;

  int contWindowPwr;

  // --------------------------
  // specific 802.11 operations
  // --------------------------

  WirelessEtherState* _currentState; // CSMA-CA state
  WEReceiveMode* _currentReceiveMode; // Receive mode

  ///@name statistics variables
  //@{
  //Storing values for statistics
  Loki::cTimerMessageCB<void>* updateStatsNotifier;
public:
  virtual void updateStats(void);
protected:
  TimeAverageReading throughput; // average stored in bytes/sec
  double noOfFailedTx;
  double noOfSuccessfulTx;
  double errorPercentage; // 0-1
  TimeAverageReading totalWaitTime;
  TimeAverageReading totalBackoffTime;
  double waitStartTime;

  cOutVector* throughputVec;
  cOutVector* errorPercentageVec;
  cOutVector* noOfFailedTxVec;
  cOutVector* totalBackoffTimeVec;
  cOutVector* totalWaitTimeVec;

  unsigned int noOfDiscardedFrames;
  //double totalBackoffTime;
  double totalBytesTransmitted;
  unsigned int totalBytesReceived;
  double beginCollectionTime;
  double endCollectionTime;
  //@}

  double getRxPower(int distance); //mW

  // -----
  // debug
  // -----

  void printSelfMsg(const cMessage* msg);

  // L2 Trigger
  cTimerMessage* l2Trigger[NumTrigVals];

  // record L2 handover message for higher layer
  L2DelayTmr* l2DelayRecorder;

  L2DelayTmr* l2LinkDownRecorder;

  // generate frame
  WirelessEtherBasicFrame* createFrame(FrameType frameType,
                                       SubType subType,
                                       MACAddress6 source,
                                       MACAddress6 destination = MACAddress6(WE_BROADCAST_ADDRESS));

  //create Frame body
  virtual FrameBody* createFrameBody(WirelessEtherBasicFrame* f);

  mutable double _wirelessRange;

private:

  cOutVector* l2HODelay;
  simtime_t linkdownTime;

  // maximum number of signal strength readings
  unsigned int sSMaxSample; //obtained from XML

  struct APInfo
  {
    MACAddress6 address;
    int channel;
    double rxpower; // received power from AP
    double hOValue;    // value to resolve handover decisions
      bool associated;

    // parameters to help HO decision (not part of standard)
    double estAvailBW;
    double errorPercentage;
    double avgBackoffTime;
    double avgWaitTime;
  } associateAP;

  //  for scan purpose

  typedef std::list<APInfo> AccessPointList;
  typedef std::list<APInfo>::iterator AIT;

  // Used for specifying a handover target
  struct HOTarget
  {
    bool valid;
    APInfo target;
  } handoverTarget;

  ReceiveMode receiveMode;

  // temporaroy access point list during the scan process
  AccessPointList tempAPList;

  bool *channelToScan;

  // generate probe request
  WESignalData* generateProbeReq(void);

  // self check if the frame is a probe request
  bool isProbeReq(WESignalData* signal);

  // probe channel process
  void probeChannel(void);

  // passive scan process
  void passiveChannelScan(void);

  void authTimeoutHandler(void);

  void assTimeoutHandler(void);

  void initialiseChannelToScan(void);

  void insertToAPList(APInfo newEntry);

  bool highestPowerAPEntry(APInfo&);
  bool highestHOValueAPEntry(APInfo&);

  bool findAPEntry(APInfo&);
};

typedef std::list<WESignalData*>::iterator WIT;
typedef std::list<cTimerMessage*>::iterator TIT;

#endif //
