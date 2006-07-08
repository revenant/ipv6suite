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

#include <memory> //auto_ptr

#include <boost/shared_ptr.hpp>
#include <string>
#include <vector> // for supported rates

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
class WEQueue;
class WEQoSQueue;

class WirelessAccessPoint;
class MobilityHandler;
class WorldProcessor;
class InterfaceEntry;
class cTimerMessage;

// support rates of the wireless network interface
typedef std::vector<SupportedRatesElement> SupportedRates;

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
    friend class WirelessEtherStateBackoffReceive;
    friend class WirelessEtherStateReceive;
    friend class WirelessEtherStateSend;
    friend class WirelessEtherStateIdle;
    friend class WEQoSQueue;

public:
    Module_Class_Members(WirelessEtherModule, LinkLayerModule, 0);

    ~WirelessEtherModule();
    virtual void initialize(int stage);
    virtual void handleMessage(cMessage* msg);
    virtual void finish();
    virtual int numInitStages(void) const { return 2; }

    void readConfiguration();

    // adds interface entry into InterfaceTable
    InterfaceEntry *registerInterface();

    void receiveSignal(std::auto_ptr<cMessage> msg);

    // receive packet from other layer besides physical layer e.g. upper
    // layer or peer layer
    virtual void receiveData(std::auto_ptr<cMessage> msg);

    virtual void setLayer2Trigger( cTimerMessage* trig, enum TrigVals v=LinkUP);

    cTimerMessage* getLayer2Trigger(enum TrigVals v=LinkUP){ return l2Trigger[v]; }

    // reset all current CSMA/CA-related values back to initial state
    void reset(void);

    // attributes
    int getDataRate(void){ return dataRate; }

    const double successOhDurationBE() {return _successOhDurationBE;}
    const double successOhDurationVI() {return _successOhDurationVI;};
    const double successOhDurationVO() {return _successOhDurationVO;};

    int getChannel(void){ return channel; }
    double getPower(void) { return txpower; } // mW
    double getThreshPower(void) { return threshpower; } // dBm
    double getHOThreshPower(void) { return hothreshpower; } // dBm

    bool isAP() { return apMode; }

    long procDelay(void) { return procdelay; }

    std::string macAddressString(void);

    bool linkUpTrigger() { return _linkUpTrigger; }

    WEQueue* outputQueue;
    std::list<WirelessEtherBasicFrame*> offlineOutputBuffer;

    WESignalData* inputFrame;

    // list to store signal strength readings
    AveragingList *signalStrength; // dBm

    // input gate of the Output Queue for incoming packet from other layer or peer L2 modules
    virtual int outputQueueInGate() { return findGate("netwIn"); }

    // output gate of the Input Queue to other layer or peer L2 modules
    virtual int inputQueueOutGate() { return findGate("netwOut"); }

    // CSMA-CA

    // check if the sending frame is a broadcast frame, if it is then go
    // back to idle
    bool handleSendingBcastFrame(void);
    void sendFrame(WirelessEtherBasicFrame*);
    void sendEndOfFrame();

    WirelessEtherState* currentState() const { return _currentState; }
    WEReceiveMode* currentReceiveMode() const { return _currentReceiveMode; }
    void changeState(WirelessEtherState* state) { _currentState = state; }
    void changeReceiveMode(WEReceiveMode* mode);

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

    void cancelAllTmrMessages(void);

    void decodeFrame(WESignalData* signal);

    void incrementSequenceNumber(void)
    {
      sequenceNumber = static_cast<unsigned short>((sequenceNumber+1)%4096);
    }

    bool fastActiveScan(void) { return fastActScan; }

    bool scanShortCircuit(void) { return scanShortCirc; }

    // returns range in meters, relative to threspower, not HOthrespower
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

    //
    // configuration
    //
    MACAddress6 address;
    int dataRate;
    bool apMode;
    bool adhocMode;
    std::string ssid;
    SupportedRates rates; // Mbps (NOT IMPLEMENTED YET!)
    double pLExp; // path loss exponent
    double pLStdDev; // dB; standard deviation of Gauss. dist. for path loss model
    double txpower; // mW
    double threshpower; // dBm; threshold power
    double hothreshpower; // dBm; handover threshold power
    double probeEnergyTimeout;
    double probeResponseTimeout;
    double authenticationTimeout;
    double associationTimeout;
    unsigned int maxRetry;
    bool fastActScan;
    bool scanShortCirc;
    bool crossTalk;
    bool shadowing;
    std::string chanNotToScan;
    double bWRequirements;  // rating from 0-1
    bool statsVec;
    bool activeScan;
    double channelScanTime;
    unsigned int bufferSize;
    bool regInterface;
    double errorRate;

    double _successOhDurationBE;
    double _successOhDurationVI;
    double _successOhDurationVO;

    //
    // state information and statistics
    //
    bool ackReceived;
    int channel;

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

    // timers
    cMessage *awaitAckTimer;    // WIRELESS_SELF_AWAITACK
    cMessage *backoffTimer;    // WIRELESS_SELF_AWAITMAC
    cMessage *sendAckTimer; // WIRELESS_SELF_SCHEDULEACK
    cMessage *endSendAckTimer;  // WIRELESS_SELF_ENDSENDACK
    cMessage *endSendDataTimer;    // WE_TRANSMIT_SENDDATA
    cMessage *updateStatsTimer;  // TMR_STATS

    cMessage *powerUpBeaconNotifier;
    cMessage *prbEnergyScanNotifier;
    cMessage *prbRespScanNotifier;
    cMessage *channelScanNotifier;
    cMessage *authTimeoutNotifier;
    cMessage *assTimeoutNotifier;

    typedef struct destInfo
    {
      WirelessEtherModule* mod; // module to send to
      int index;                // node interface index
      int channel;
      double rxPower;
    }
    DestInfo;

    // SWOON HACK: To find achievable throughput
    double probTxInSlot;
    int appType;

    /**
       @brief store modules which need to be sent the end of a frame

       Using shared_ptr so whenever we do idelDest.clear() all the pointed to
       entities are automatically deleted.

       @todo DestInfo does not really need to be a pointer does it? (Johnny)
    */
    typedef std::list<boost::shared_ptr<DestInfo> > IdleDest;
    IdleDest idleDest;
    typedef IdleDest::iterator IDIT;

    // --------------------------
    // specific 802.11 operations
    // --------------------------

    WirelessEtherState* _currentState; // CSMA-CA state
    WEReceiveMode* _currentReceiveMode; // Receive mode

    ///@name statistics variables
    //@{
  public:
    virtual void updateStats(void);
  protected:

     /********* STATISTICAL VARIABLES (START) **********/
      double samplingStartTime;  //seconds
    double statsUpdatePeriod; //seconds
    double dataReadyTimeStamp; //seconds
    double totalDisconnectedTime; //seconds
    //statistics which will be resetted every update period
    double RxDataBWStat;        //received data bandwidth (Mbit)
    double TxDataBWStat;        //transmitted data bandwidth (Mbit)
    double noOfRxStat;          //number of received data frames (frames)
    double noOfTxStat;          //number of transmitted data frames (frames)
    double noOfFailedTxStat;    //number of dropped transmission (frames)
    double txSuccess;
    cStdDev* RxFrameSizeStat;   //average received frame size (bytes/frame)
    cStdDev* TxFrameSizeStat;   //average transmitted frame size (bytes/frame)
    cStdDev* TxAccessTimeStat;  //average time to successfully tx a frame (sec/successful attempt)
    cStdDev* backoffSlotsStat;  //average backoff time per backoff (slots/tx attempt)
    cStdDev* CWStat;            //average contention window per backoff (slots/tx attempt)

    //average of above statistics over the whole simulation
    cStdDev* avgRxDataBWStat;        //(Mbit/s)
    cStdDev* avgTxDataBWStat;        //(Mbit/s)
    cStdDev* avgNoOfRxStat;          //(frames/s)
    cStdDev* avgNoOfTxStat;          //(frames/s)
    cStdDev* avgNoOfFailedTxStat;    //(frames/s)
    cStdDev* avgRxFrameSizeStat;     //(bytes/frame)
    cStdDev* avgTxFrameSizeStat;     //(bytes/frame)
    cStdDev* avgTxFrameRateStat;
    cStdDev* avgTxAccessTimeStat;    //(sec/successful attempt)
    cStdDev* avgBackoffSlotsStat;    //(slots/tx attempt)
    cStdDev* avgCWStat;              //(slots/tx attempt)

    cStdDev* avgOutBuffSizeBEStat;     //current buffer size(frames)
    cStdDev* avgOutBuffSizeVIStat;     //current buffer size(frames)
    cStdDev* avgOutBuffSizeVOStat;     //current buffer size(frames)

    //vector of statistics every second
    cOutVector* RxDataBWVec;        //(Mbit/s)
    cOutVector* TxDataBWVec;        //(Mbit/s)
    cOutVector* noOfRxVec;          //(frames/s)
    cOutVector* noOfTxVec;          //(frames/s)
    cOutVector* noOfFailedTxVec;    //(frames/s)
    cOutVector* RxFrameSizeVec;     //(bytes/frame)
    cOutVector* TxFrameSizeVec;     //(bytes/frame)
    cOutVector* TxFrameRateVec;
    cOutVector* TxAccessTimeVec;    //(sec/successful attempt)
    cOutVector* backoffSlotsVec;    //(slots/tx attempt)
    cOutVector* CWVec;              //(slots/tx attempt)

    cOutVector* outBuffSizeBEVec;     //current buffer size(frames)
    cOutVector* outBuffSizeVIVec;     //current buffer size(frames)
    cOutVector* outBuffSizeVOVec;     //current buffer size(frames)
    cOutVector* probTxInSlotBEVec;
    cOutVector* probTxInSlotVIVec;
    cOutVector* probTxInSlotVOVec;
    cOutVector* lambdaBEVec;
    cOutVector* lambdaVIVec;
    cOutVector* lambdaVOVec;
    cOutVector* noOfCollisionBEVec;
    cOutVector* noOfCollisionVIVec;
    cOutVector* noOfCollisionVOVec;
    cOutVector* noOfAttemptedBEVec;
    cOutVector* noOfAttemptedVIVec;
    cOutVector* noOfAttemptedVOVec;
    cOutVector* avgTxFrameSizeBEVec;
    cOutVector* avgTxFrameSizeVIVec;
    cOutVector* avgTxFrameSizeVOVec;

    //vector of instantaneaous readings
    cOutVector* InstRxFrameSizeVec; //(bytes) successful ones
    cOutVector* InstTxFrameSizeVec; //(bytes) successful ones

    /********* STATISTICAL VARIABLES (END) **********/

    unsigned int noOfDiscardedFrames;
    //double totalBackoffTime;
    double totalBytesTransmitted;
    unsigned int totalBytesReceived;
    double beginCollectionTime;
    double endCollectionTime;
    //@}

    // Distance is in meters, returned power in dBm
    double getRxPower(int distance); // dBm

    // -----
    // debug
    // -----

    void printSelfMsg(const cMessage* msg);

    // L2 Trigger
    cTimerMessage* l2Trigger[NumTrigVals];

    // generate frame
    WirelessEtherBasicFrame* createFrame(FrameType frameType,
                                         SubType subType,
                                         MACAddress6 source,
                                         int frameAppType,
                                         MACAddress6 destination = MACAddress6(WE_BROADCAST_ADDRESS));

    //create Frame body
    virtual FrameBody* createFrameBody(WirelessEtherBasicFrame* f);

    mutable double _wirelessRange;

    void reschedule(cMessage *msg, simtime_t t);

  private:

    bool _linkUpTrigger;
    bool _linkDownTrigger;
    bool noAuth;

  // used to calculate scalar totalDisconnectedTime
    simtime_t linkdownTime;

    // maximum number of signal strength readings
    unsigned int sSMaxSample; //obtained from XML

    struct APInfo
    {
      MACAddress6 address;
      int channel;
      int receivedSequence;
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

    // temporaroy access point list during the scan process
    AccessPointList tempAPList;

    bool *channelToScan;

    // send beacon -- only makes sense in an AP
    virtual void sendBeacon();

    // generate probe request
    WirelessEtherBasicFrame* generateProbeReq(void);

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


#endif
