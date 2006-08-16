//
// Copyright (C) 2001, 2002, 2004 Monash University, Melbourne, Australia
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
    @file WirelessEtherModule.cc
    @brief Definition file for WirelessEtherModule

    simple implementation of wireless Ethernet module

    @author Eric Wu
*/

#include "sys.h"
#include "debug.h"


#include <iostream>
#include <iomanip>
#include <cassert>
#include <cmath>                // std::pow
#include <math.h>
#include <sstream>
#include <string>

#include "cTimerMessage.h"
#include "TimerConstants.h"

#if MLDV2
#include "IPv6Datagram.h"
#endif

#include "opp_utils.h"

#include "WorldProcessor.h"
#include "MobileEntity.h"
#include "MobilityHandler.h"

#include "wirelessethernet.h"
#include "WEQueue.h"

#include "WirelessAccessPoint.h"
#include "WirelessEtherModule.h"
#include "WirelessEtherSignal_m.h"

#include "WirelessEtherStateIdle.h"
#include "WirelessEtherStateSend.h"
#include "WirelessEtherStateBackoff.h"
#include "WirelessEtherStateReceive.h"
#include "WirelessEtherStateAwaitACK.h"
#include "WirelessEtherStateAwaitACKReceive.h"
#include "WirelessEtherStateBackoffReceive.h"
#include "WirelessEtherStateBackoff.h"

#include "WirelessEtherReceiveMode.h"
#include "WirelessEtherAScanReceiveMode.h"
#include "WirelessEtherMonitorReceiveMode.h"
#include "WirelessEtherPScanReceiveMode.h"
#include "WirelessEtherAuthenticationReceiveMode.h"
#include "WirelessEtherAssociationReceiveMode.h"
#include "WirelessEtherDataReceiveMode.h"

#include "LL6ControlInfo_m.h"

#include "AveragingList.h"

#if MLDV2
#include "MLDv2Message.h"
#endif

#if L2FUZZYHO                   // (Layer 2 fuzzy logic handover)
#include"hodec.h"
#endif // L2FUZZYHO

#include "XML/XMLOmnetParser.h"
#include "InterfaceTableAccess.h"

#include "BRMsg_m.h"

Define_Module(WirelessEtherModule);

/*
WirelessEtherModule::WirelessEtherModule(const char *name, cModule *parent):
  cSimpleModule(name, parent, 0), channelToScan(new bool[MAX_CHANNEL + 1])
{
}
*/

WirelessEtherModule::~WirelessEtherModule()
{
    delete RxFrameSizeStat;
    delete TxFrameSizeStat;
    delete backoffSlotsStat;
    delete CWStat;

    delete avgRxDataBWStat;
    delete avgTxDataBWStat;
    delete avgNoOfRxStat;
    delete avgNoOfTxStat;
    delete avgNoOfFailedTxStat;
    delete avgRxFrameSizeStat;
    delete avgTxFrameSizeStat;
    delete avgTxFrameRateStat;
    delete avgTxAccessTimeStat;
    delete avgBackoffSlotsStat;
    delete avgCWStat;
    delete avgOutBuffSizeBEStat;
    delete avgOutBuffSizeVIStat;
    delete avgOutBuffSizeVOStat;

    delete RxDataBWVec;
    delete TxDataBWVec;
    delete noOfRxVec;
    delete noOfTxVec;
    delete noOfFailedTxVec;
    delete RxFrameSizeVec;
    delete TxFrameSizeVec;
    delete TxAccessTimeVec;
    delete backoffSlotsVec;
    delete CWVec;

    delete outBuffSizeBEVec;
    delete outBuffSizeVIVec;
    delete outBuffSizeVOVec;
    delete probTxInSlotBEVec;
    delete probTxInSlotVIVec;
    delete probTxInSlotVOVec;
    delete lambdaBEVec;
    delete lambdaVIVec;
    delete lambdaVOVec;
    delete noOfCollisionBEVec;
    delete noOfCollisionVIVec;
    delete noOfCollisionVOVec;
    delete noOfAttemptedBEVec;
    delete noOfAttemptedVIVec;
    delete noOfAttemptedVOVec;
    delete avgTxFrameSizeBEVec;
    delete avgTxFrameSizeVIVec;
    delete avgTxFrameSizeVOVec;

    if (InstRxFrameSizeVec != NULL)
        delete InstRxFrameSizeVec;
    if (InstTxFrameSizeVec != NULL)
        delete InstTxFrameSizeVec;

    delete outputQueue;
//    delete[]channelToScan;
//    delete signalStrength;
}

void WirelessEtherModule::baseInit(int stage)
{
    if (stage == 0)
    {
        changeState(WirelessEtherStateIdle::instance());
        sequenceNumber = 0;
        inputFrame = 0;
        _wirelessRange = 0;
        ackReceived = false;
        idleDest.clear();
        noOfRxFrames = 0;
        frameSource = "";
        // SWOON HACK: To find achievable throughput
        probTxInSlot = 0;
        appType = AC_BE;

        readConfiguration();

        beginCollectionTime = OPP_Global::findNetNodeModule(this)->par("beginCollectionTime").doubleValue();
        endCollectionTime = OPP_Global::findNetNodeModule(this)->par("endCollectionTime").doubleValue();

	noAuth = par("noAuth");

        // Most likely wouldn't want to register interface if using dual interface node
        if (regInterface)
            registerInterface();

        // Initialise timers
        awaitAckTimer = new cMessage("endAwaitAck", WIRELESS_SELF_AWAITACK);
        backoffTimer = new cMessage("backoff", WIRELESS_SELF_AWAITMAC);
        sendAckTimer = new cMessage("sendAck", WIRELESS_SELF_SCHEDULEACK);
        endSendAckTimer = new cMessage("endSendingAck", WIRELESS_SELF_ENDSENDACK);
        endSendDataTimer = new cMessage("endSendingData", WE_TRANSMIT_SENDDATA);
        updateStatsTimer = new cMessage("updateStats", TMR_STATS);

        powerUpBeaconNotifier = new cMessage("sendPowerUpBeacon", TMR_BEACON); // AP only
        prbEnergyScanNotifier = new cMessage("probeChannel", TMR_PRBENERGYSCAN);
        prbRespScanNotifier = new cMessage("probeChannel", TMR_PRBRESPSCAN);
        channelScanNotifier = new cMessage("passiveChannelScan", TMR_CHANNELSCAN);
        authTimeoutNotifier = new cMessage("authTimeout", TMR_AUTHTIMEOUT);
        assTimeoutNotifier = new cMessage("assTimeout", TMR_ASSTIMEOUT);
        // TODO: TMR_REMOVEENTRY

        // Initialise Variables for Statistics
        samplingStartTime = 100;
        statsUpdatePeriod = 1;
        dataReadyTimeStamp = 0;
        RxDataBWStat = 0;
        TxDataBWStat = 0;
        noOfRxStat = 0;
        noOfTxStat = 0;
        noOfFailedTxStat = 0;
        txSuccess = 0;

        RxFrameSizeStat = new cStdDev("RxFrameSizeStat");
        TxFrameSizeStat = new cStdDev("TxFrameSizeStat");
        TxAccessTimeStat = new cStdDev("TxAccessTimeStat");
        backoffSlotsStat = new cStdDev("backoffSlotsStat");
        CWStat = new cStdDev("CWStat");

        avgRxDataBWStat = new cStdDev("avgRxDataBWStat");
        avgTxDataBWStat = new cStdDev("avgTxDataBWStat");
        avgNoOfRxStat = new cStdDev("avgNoOfRxStat");
        avgNoOfTxStat = new cStdDev("avgNoOfTxStat");
        avgNoOfFailedTxStat = new cStdDev("avgNoOfFailedTxStat");
        avgRxFrameSizeStat = new cStdDev("avgRxFrameSizeStat");
        avgTxFrameSizeStat = new cStdDev("avgTxFrameSizeStat");
        avgTxFrameRateStat = new cStdDev("avgTxFrameRateStat");
        avgTxAccessTimeStat = new cStdDev("avgTxAccessTimeStat");
        avgBackoffSlotsStat = new cStdDev("avgBackoffSlotsStat");
        avgCWStat = new cStdDev("avgCWStat");
        avgOutBuffSizeBEStat = new cStdDev("avgOutBuffSizeBEStat");
        avgOutBuffSizeVIStat = new cStdDev("avgOutBuffSizeVIStat");
        avgOutBuffSizeVOStat = new cStdDev("avgOutBuffSizeVOStat");

        RxDataBWVec = new cOutVector("RxDataBWVec");
        TxDataBWVec = new cOutVector("TxDataBWVec");
        noOfRxVec = new cOutVector("noOfRxVec");
        noOfTxVec = new cOutVector("noOfTxVec");
        noOfFailedTxVec = new cOutVector("noOfFailedTxVec");
        RxFrameSizeVec = new cOutVector("RxFrameSizeVec");
        TxFrameSizeVec = new cOutVector("TxFrameSizeVec");
        TxFrameRateVec = new cOutVector("TxFrameRate");
        TxAccessTimeVec = new cOutVector("TxAccessTimeVec");
        backoffSlotsVec = new cOutVector("backoffSlots");
        CWVec = new cOutVector("CW");

        outBuffSizeBEVec = new cOutVector("outBuffSizeBEVec");
        outBuffSizeVIVec = new cOutVector("outBuffSizeVIVec");
        outBuffSizeVOVec = new cOutVector("outBuffSizeVOVec");
        probTxInSlotBEVec = new cOutVector("probTxInSlotBEVec");
        probTxInSlotVIVec = new cOutVector("probTxInSlotVIVec");
        probTxInSlotVOVec = new cOutVector("probTxInSlotVOVec");
        lambdaBEVec = new cOutVector("lambdaBEVec");
        lambdaVIVec = new cOutVector("lambdaVIVec");
        lambdaVOVec = new cOutVector("lambdaVOVec");
        noOfCollisionBEVec = new cOutVector("noOfCollisionBEVec");
        noOfCollisionVIVec = new cOutVector("noOfCollisionVIVec");
        noOfCollisionVOVec = new cOutVector("noOfCollisionVOVec");
        noOfAttemptedBEVec = new cOutVector("noOfAttemptedBEVec");
        noOfAttemptedVIVec = new cOutVector("noOfAttemptedVIVec");
        noOfAttemptedVOVec = new cOutVector("noOfAttemptedVOVec");
        avgTxFrameSizeBEVec = new cOutVector("avgTxFrameSizeBEVec");
        avgTxFrameSizeVIVec = new cOutVector("avgTxFrameSizeVIVec");
        avgTxFrameSizeVOVec = new cOutVector("avgTxFrameSizeVOVec");

        InstRxFrameSizeVec = statsVec ? new cOutVector("InstRxFrameSizeVec") : NULL;
        InstTxFrameSizeVec = statsVec ? new cOutVector("InstTxFrameSizeVec") : NULL;
    }
    else if (stage == 1)
    {
        outputQueue->initialise(this);

        cModule *mobMan = OPP_Global::findModuleByName(this, "mobilityHandler");
        assert(mobMan);
        _mobCore = static_cast<MobilityHandler *>(mobMan);

        // Timer to update statistics
        scheduleAt(simTime() + statsUpdatePeriod, updateStatsTimer);

        cModule *nodemod = OPP_Global::findNetNodeModule(this);
        nodemod->displayString().setTagArg("r",0,OPP_Global::dtostr(wirelessRange()).c_str());

        if ( isAP() )
          nodemod->displayString().setTagArg("r",2,"red");
        else
          nodemod->displayString().setTagArg("r",2,"blue");
    }
}

void WirelessEtherModule::initialize(int stage)
{
    if (stage == 0)
    {
        _linkUpTrigger = false;
	_linkDownTrigger = false;
        totalDisconnectedTime = 0;

        LinkLayerModule::initialize();
        cModule *mod = OPP_Global::findModuleByName(this, "worldProcessor");
        assert(mod);
        wproc = static_cast<WorldProcessor *>(mod);
        setIface_name(PR_WETHERNET);
        iface_type = PR_WETHERNET;

        procdelay = par("procdelay");
        apMode = false;

        channel = 0;
        WATCH(channel);

        associateAP.address = MAC_ADDRESS_UNSPECIFIED_STRUCT;
        associateAP.channel = INVALID_CHANNEL;
        associateAP.rxpower = INVALID_POWER;
        associateAP.associated = false;
        associateAP.receivedSequence = 0;
        associateAP.estAvailBW = 0;
        associateAP.errorPercentage = 0;
        associateAP.avgBackoffTime = 0;
        associateAP.avgWaitTime = 0;

        handoverTarget.valid = false;

        for (int i = 0; i < NumTrigVals; i++)
        {
            l2Trigger[i] = 0;
        }
    }
    else if (stage == 1)
    {
        if (!apMode)
	{
	  _linkUpTrigger = par("linkUpTrigger");
	  _linkDownTrigger = par("linkDownTrigger");
	}
        // list to store signal strength readings
        signalStrength = new AveragingList(sSMaxSample);

        initialiseChannelToScan();

        double randomStart = uniform(0, 1);
        // On power up, wireless ethernet interface is staarting to
        // perform active scanning..
        if (activeScan)
        {
            changeReceiveMode(WEAScanReceiveMode::instance());
            scheduleAt(simTime() + randomStart, prbEnergyScanNotifier);
        }
        else
        {
            changeReceiveMode(WEPScanReceiveMode::instance());
            scheduleAt(simTime() + randomStart, channelScanNotifier);
        }

        wEV  << fullPath() << "\n"
             << " ====================== \n"
             << " MAC ADDR: " << address.stringValue() << "\n"
             << " SSID: " << ssid.c_str() << "\n"
             << " PATH LOSS EXP: " << pLExp << "\n"
             << " STD DEV: " << pLStdDev << "dB\n"
             << " TXPOWER: " << txpower << "mW\n"
             << " THRESHOLDPOWER: " << threshpower << "dBm\n"
             << " HOTHRESHOLDPOWER: " << hothreshpower << "dBm\n"
             << " PROBEENERGY_TIMEOUT: " << probeEnergyTimeout << "\n"
             << " PROBERESPONSE_TIMEOUT: " << probeResponseTimeout << "\n"
             << " AUTHENTICATION_TIMEOUT: " << authenticationTimeout << " (TU)\n"
             << " ASSOCIATION_TIMEOUT: " << associationTimeout << " (TU)\n"
             << " MAXRETRY: " << maxRetry << "\n"
             << " FAST_ACTIVE_SCAN: " << fastActScan << "\n"
             << " CROSSTALK: " << crossTalk << "\n"
             << " SHADOWING: " << shadowing << "\n"
             << " CHANNELS TO AVOID: " << chanNotToScan << "\n"
             << " MAX SS SAMPLE COUNT: " << sSMaxSample << "\n";
    }
    baseInit(stage);
}

void WirelessEtherModule::finish()
{
    cModule *linkLayer = gate("extSignalOut")->toGate()->ownerModule();
    // if not dual layer node then print statistics
    if (!linkLayer->gate("extSignalOut")->isConnected())
    {
        recordScalar("avgTxFrameSizeStat", avgTxFrameSizeStat->mean());
        recordScalar("avgTxFrameRateStat", avgTxFrameRateStat->mean());
        recordScalar("avgRxDataBWStat", avgRxDataBWStat->mean());
        recordScalar("avgTxDataBWStat", avgTxDataBWStat->mean());
        recordScalar("avgNoOfFailedTxStat", avgNoOfFailedTxStat->mean());
        recordScalar("avgTxAccessTimeStat", avgTxAccessTimeStat->mean());
        recordScalar("avgBackoffSlots", avgBackoffSlotsStat->mean());
        recordScalar("avgCW", avgCWStat->mean());
        recordScalar("avgOutBuffSizeBEStat", avgOutBuffSizeBEStat->mean());
        recordScalar("avgOutBuffSizeVIStat", avgOutBuffSizeVIStat->mean());
        recordScalar("avgOutBuffSizeVOStat", avgOutBuffSizeVOStat->mean());
        recordScalar("totalDisconnectedTime", totalDisconnectedTime);
    }
}

void WirelessEtherModule::readConfiguration()
{
    // XXX this code was created from  XMLOmnetParser::parseWEInfo() --AV
    dataRate = par("dataRate");
    ssid = par("ssid").stringValue();
    pLExp = par("pathLossExponent");
    pLStdDev = par("pathLossStdDev");
    txpower = par("txPower");
    threshpower = par("thresholdPower");
    hothreshpower = par("hoThresholdPower");
    probeEnergyTimeout = par("probeEnergyTimeout");
    probeResponseTimeout = par("probeResponseTimeout");
    authenticationTimeout = par("authenticationTimeout");
    associationTimeout = par("associationTimeout");
    maxRetry = par("retry");
    fastActScan = par("fastActiveScan");
    scanShortCirc = par("scanShortCircuit");
    crossTalk = par("crossTalk");
    shadowing = par("shadowing");
    chanNotToScan = par("channelsNotToScan").stringValue();
    sSMaxSample = par("signalStrengthMaxSample");

    std::string addr = par("address").stringValue();

    _successOhDurationBE = SIFS + ACKLENGTH / dataRate + collOhDurationBE;
    _successOhDurationVI = SIFS + ACKLENGTH / dataRate + collOhDurationVI;
    _successOhDurationVO = SIFS + ACKLENGTH / dataRate + collOhDurationVO;

    if (!apMode)
    {
      if (!addr.empty())
        address.set(addr.c_str());
      // Only want to initialise the MN mac address once otherwise routing tables
      // are wrong if we generate another random number from the stream
      else if (address == MACAddress6())
      {
        MAC_address macAddr;
        macAddr.high = OPP_Global::generateInterfaceId() & 0xFFFFFF;
        macAddr.low = OPP_Global::generateInterfaceId() & 0xFFFFFF;
        address.set(macAddr);
      }

      par("address") = address;
    }

    bWRequirements = par("bandwidthRequirements");
    statsVec = par("recordStatisticVector").boolValue();
    activeScan = par("activeScan");
    channelScanTime = par("channelScanTime");
    bufferSize = par("bufferSize");
    regInterface = par("registerInterface");
    outputQueue = check_and_cast<WEQueue *>(createOne(par("queueType")));
    outputQueue->setMaxQueueSize(par("queueSize"));
    errorRate = par("errorRate");

    // TODO: parse supported rates
}

InterfaceEntry *WirelessEtherModule::registerInterface()
{
    InterfaceEntry *e = new InterfaceEntry();

/*
  // interface name: NetworkInterface module's name without special characters ([])
  char *interfaceName = new char[strlen(parentModule()->fullName())+1];
  char *d=interfaceName;
  for (const char *s=parentModule()->fullName(); *s; s++)
    if (isalnum(*s))
      *d++ = *s;
  *d = '\0';

  e->setName(interfaceName);
  delete [] interfaceName;
*/
    std::string tmp = std::string("wlan") + OPP_Global::ltostr(parentModule()->index());
    e->setName(tmp.c_str());    // XXX HACK -- change back to above code!

    e->_linkMod = this;         // XXX remove _linkMod on the long term!! --AV

    // port: index of gate where parent module's "netwIn" is connected (in IP)
    int outputPort = parentModule()->gate("netwIn")->fromGate()->index();
    e->setOutputPort(outputPort);

    // generate a link-layer address to be used as interface token for IPv6
    unsigned int iid[2];
    iid[0] = (address.intValue()[0] << 8) | 0xFF;
    iid[1] = address.intValue()[1] | 0xFE000000;
    InterfaceToken token(iid[1], iid[0], 64);
    e->setInterfaceToken(token);

    // MAC address as string
    e->setLLAddrStr(address.stringValue());

    // MTU is 1500 on Ethernet
    e->setMtu(1500);

    // capabilities
    e->setMulticast(true);
    e->setPointToPoint(false);

    // add
    InterfaceTable *ift = InterfaceTableAccess().get();
    ift->addInterface(e);

    return e;
}

void WirelessEtherModule::reschedule(cMessage *msg, simtime_t t)
{
    if (msg->isScheduled())
        cancelEvent(msg);
    scheduleAt(t, msg);
}


// Function to handle external signals
void WirelessEtherModule::receiveSignal(std::auto_ptr<cMessage> msg)
{
    WirelessExternalSignal *extSig = static_cast<WirelessExternalSignal *>(msg.get());

    // Monitor mode signal
    if (extSig->getType() == ST_MONITOR)
    {
        startMonitorMode();
    }
    // Channel specifier signal
    else if (extSig->getType() == ST_CHANNEL)
    {
        WirelessExternalSignalChannel *sigChan = static_cast<WirelessExternalSignalChannel *>(msg.get());
        channel = sigChan->getChan();

        wEV << currentTime() << " Switching to channel: " << channel << "\n";
        startMonitorMode();
    }
    // Request for stats reading from associated AP
    else if (extSig->getType() == ST_STATS_REQUEST)
    {
        sendStatsSignal();
    }
    // Request handover to a specified target
    else if (extSig->getType() == ST_HANDOVER)
    {
        // Keep record of the target and indicate its valid
        WirelessExternalSignalHandover *sigHO = static_cast<WirelessExternalSignalHandover *>(msg.get());
        handoverTarget.target.address = sigHO->getTarget();
        handoverTarget.valid = true;

        wEV << fullPath()<< " " << currentTime() << " Handover to target: " << handoverTarget.target.address << "\n";
        // Start active scan for handover
        restartScanning();
    }
    else
        assert(false);

    // need to handle association signal as well
}

void WirelessEtherModule::receiveData(std::auto_ptr<cMessage> msg)
{
    if (msg->length() > WE_MAX_PAYLOAD_BYTES * 8)
    {
        wEV << "message " << msg->name() << " too large (" 
	<<(msg->length() / 8) << " bytes), discarding limit is "
	<<WE_MAX_PAYLOAD_BYTES<<"\n";
        return;
    }

    // if (_currentReceiveMode != WEDataReceiveMode::instance())
    //  return;
    LL6ControlInfo *ctrlInfo = check_and_cast<LL6ControlInfo *>(msg->removeControlInfo());
    WirelessEtherBasicFrame *frame;
    // SWOON HACK: To find achievable throughput
    if (BRMsg * brMsg = dynamic_cast<BRMsg *>(msg.get()))
    {
        int ACType;
        if (brMsg->getType() == MT_DT)
            ACType = AC_BE;
        else if (brMsg->getType() == MT_VI)
            ACType = AC_VI;
        else if (brMsg->getType() == MT_VO)
            ACType = AC_VO;
        else
            assert(false);

        frame = createFrame(FT_DATA, ST_DATA, address, ACType, MACAddress6(ctrlInfo->getDestLLAddr()));
        appType = ACType;
    }
    else
    {
        frame = createFrame(FT_DATA, ST_DATA, address, AC_BE, MACAddress6(ctrlInfo->getDestLLAddr()));
    }
    delete ctrlInfo;
    assert(frame);

    frame->encapsulate(msg.get());
    frame->setName(msg->name());
    msg.release();              // XXX maybe get rid of auto_ptr here? --AV

    if (associateAP.associated)
    {
        outputQueue->insertFrame(frame, simTime());

        if (_currentState == WirelessEtherStateIdle::instance())
            static_cast<WirelessEtherStateIdle *>(_currentState)->chkOutputBuffer(this);
    }
    else
      // Note that since no association with an AP exists in DataReceiveMode,
      // there will be no address for the AP destination. Hence, this should
      // be filled in before sending.  It should be added in address1.
      offlineOutputBuffer.push_back(frame);
}

void WirelessEtherModule::handleMessage(cMessage *msg)
{
    assert(msg);

    if (!msg->isSelfMessage())
    {
        ++cntReceivedPackets;
        _currentState->processSignal(this, auto_ptr<cMessage> (msg));
    }
    else
    {
        printSelfMsg(msg); // FIXME change/remove

	// FIXME JOHNNY HACK to record mn bw because something is cancelling timer
	// assumes statsUpdatePeriod is 1 hence ceil fn
	if (statsVec && msg != updateStatsTimer &&
	    !updateStatsTimer->isScheduled())	  
	  scheduleAt(std::ceil(simTime()), updateStatsTimer);
	
        // FIXME TODO assert that state is really the one assumed
        if (msg==awaitAckTimer)
            WirelessEtherStateAwaitACK::instance()->endAwaitACK(this);
        else if (msg==backoffTimer)
            WirelessEtherStateBackoff::instance()->readyToSend(this);
        else if (msg==sendAckTimer)
            WirelessEtherStateReceive::instance()->sendAck(this, (WirelessEtherBasicFrame *)msg->contextPointer());
        else if (msg==endSendAckTimer)
            WirelessEtherStateReceive::instance()->endSendingAck(this);
        else if (msg==endSendDataTimer)
            WirelessEtherStateSend::instance()->endSendingData(this); // this is NO MISTAKE
        else if (msg==updateStatsTimer)
            updateStats();
        else if (msg==powerUpBeaconNotifier)
            sendBeacon(); // only implemented in AP
        else if (msg==prbEnergyScanNotifier)
            probeChannel();
        else if (msg==prbRespScanNotifier)
            probeChannel();
        else if (msg==channelScanNotifier)
            passiveChannelScan();
        else if (msg==authTimeoutNotifier)
            authTimeoutHandler();
        else if (msg==assTimeoutNotifier)
            assTimeoutHandler();
        else if (dynamic_cast<cTimerMessage *>(msg))
            static_cast<cTimerMessage *>(msg)->callFunc();
        else
            error("unrecognized timer (%s)%s", msg->className(), msg->name());
    }
}

void WirelessEtherModule::sendBeacon()
{
    error("sendBeacon() called but this is not an AP MAC");
}

/**
 * @todo can make this a virtual function of mobile interfaces etc. will need a
 * function in LinkLayerModule to tell which interfaces are mobile? or maybe all * interfaces can have connect and disconnect triggers.
 */
void WirelessEtherModule::setLayer2Trigger(cTimerMessage * trig, enum TrigVals v)
{
    if (l2Trigger[v])
        delete l2Trigger[v];

    if (v == LinkUP && !_linkUpTrigger)
    {
        delete trig;
        return;
    }

    if (v == LinkDOWN && !_linkDownTrigger)
    {
      delete trig;
      return;
    }

    l2Trigger[v] = trig;

    // dc::wireless_ethernet.precision(6);
    wEV << "Set Layer 2 Trigger: (WIRELESS) " << fullPath() << " #: " << v << "\n";

}

std::string WirelessEtherModule::macAddressString(void)
{
    return address.stringValue();
}

void WirelessEtherModule::sendFrame(WirelessEtherBasicFrame *frame)
{
    WESignalData *msg = encapsulateIntoWESignalData((cMessage *) frame->dup());
    // Make sure list of idles to send is cleared
    idleDest.clear();

    // Go up two levels to obtain just the unique module name
    std::string modName = parentModule()->parentModule()->fullPath();
    msg->setSourceName(modName.c_str());

    double r1, r2, r3, r4;
    int chanSep;

    // Find the max and min channel which crosstalk can occur
    int maxChan = (channel + 4 > MAX_CHANNEL) ? MAX_CHANNEL : channel + 4;
    int minChan = (channel - 4 < 1) ? 1 : channel - 4;

    // Find modules which can receive the frame
    ModuleList mods = wproc->findWirelessEtherModulesByChannelRange(minChan, maxChan);

    // Four random values which will be used to determine whether
    // data will cross over to adjacent channels

    r1 = uniform(0, 100);
    r2 = uniform(0, 100);
    r3 = uniform(0, 100);
    r4 = uniform(0, 100);

    // Go through each module and determine whether to transmit to them
    for (MLIT it = mods.begin(); it != mods.end(); it++)
    {
        Entity *e = (*it);
        cModule *interface = e->containerModule()->parentModule()->parentModule();
        size_t noOfInterface = interface->gate("wlin")->size();

        // Send to relevant interface in a node
        for (size_t i = 0; i < noOfInterface; i++)
        {
            cModule *phylayer = interface->gate("wlin", i)->toGate()->ownerModule();
            cModule *linkLayer = phylayer->gate("linkOut")->toGate()->ownerModule();
            assert(linkLayer);

            WirelessEtherModule *a =
                static_cast<WirelessEtherModule *>(linkLayer->submodule("networkInterface"));
            // Dont send to ourselves
            if (a == this)
                continue;

            assert(a->_mobCore);
            int distance = _mobCore->getEntity()->distance(a->_mobCore->getEntity());

            double rxPower = getRxPower(distance);

            // estimated receiving power must be greater than or equal to the
            // threshold power of the other end to in order to reach the
            // destination
            if ((rxPower >= a->getThreshPower()) && (a->channel != 0))
            {
                chanSep = abs(channel - a->channel);
                // Determine whether crosstalk will occur based on probabilities:
                // 73% for  1 channel separation
                // 27% for  2   "
                // 4% for   3   "
                // 0.5% for 4   "
                // These figures were obtained from the White Paper
                // "Channel Overlap Calculations for 802.11b Networks" by Mitchell Burton
                // of Cirond Technologies
                if ((chanSep == 0)
                    || (crossTalk
                        && ((chanSep == 1 && r1 < 73)
                            || (chanSep == 2 && r2 < 27)
                            || (chanSep == 3 && r3 < 4)
                            || (chanSep == 4 && r4 < .5))))
                {
                    msg->setPower(rxPower);
                    msg->setChannelNum(a->channel);
                    // propagation delay
                    double propDelay = procdelay/1e9;
		    if (!propDelay)
		      propDelay = distance / 3e8;


                    // Mark the module which need the end of the frame
                    // Need to also remember the Rx Power
                    boost::shared_ptr<DestInfo> dInfo(new DestInfo);
                    dInfo->mod = a;
                    dInfo->index = i;
                    dInfo->rxPower = rxPower;
                    dInfo->channel = a->channel;
                    idleDest.push_back(dInfo);

                    // wEV << currentTime() << " Sending SOF to: " << interface->fullPath() << " on index: "<< i << "\n";
                    sendDirect((cMessage *) msg->dup(), propDelay, interface, "wlin", i);
                }
            }
        }
    }
    mods.clear();
    delete msg;
}

void WirelessEtherModule::sendEndOfFrame()
{
    WESignalIdle *idle = new WESignalIdle;

    // Go up two levels to obtain just the unique module name
    std::string modName = parentModule()->parentModule()->fullPath();
    idle->setSourceName(modName.c_str());

    // Check if modules need the end of the frame
    if (!idleDest.empty())
    {
        assert(_mobCore);

        for (IDIT it = idleDest.begin(); it != idleDest.end(); it++)
        {
            // Check if the station is still in the same channel to receive the end of the frame
            if ((*it)->mod->channel == (*it)->channel)
            {
                int distance = _mobCore->getEntity()->distance((*it)->mod->_mobCore->getEntity());

                // use the same calculated rxPower as for start of frame
                double rxPower = (*it)->rxPower;

                // if the end of frame is suddenly under the receiving stations threshold,
                // there must be something wrong (Entity shouldnt be moving that quick!)
                // FIXME wrong assertion!!! node can move out of range if it was near the range boundary --Andras
                assert(rxPower >= (*it)->mod->getThreshPower());

                // set end of frame properties
                idle->setPower(rxPower);
                idle->setChannelNum((*it)->mod->channel);

                // propagation delay
                //double propDelay = distance / (3 * pow((double) 10, (double) 8));
		double propDelay = procdelay/1e9;
		if (!propDelay)
		  propDelay = distance / 3e8;

                cModule *interface = static_cast<cModule *>((*it)->mod->parentModule()->parentModule());

                // wEV << currentTime() << " Sending EOF to: " << interface->fullPath() << " on index: "<< (*it)->index << "\n";

                sendDirect((cMessage *) idle->dup(), propDelay, interface, "wlin", (*it)->index);
            }
        }
        idleDest.clear();
    }

    delete idle;
}

/**
   Distance is in meters, rxpwr in dBm
   @note limitation in propagation model as it is only 2D when MN moves over AP
   will obtain distance of zero. In real life we would be under the AP usually
   so there is still some distance
*/
double WirelessEtherModule::getRxPower(int distance)
{
    if (distance < 1)
        distance = 1;

    // Note that rxpwr is in dBm
    // apply log-normal shadowing path loss equation to find receive power
    double rxpwr = 10 * log10((double) txpower) - 40 - 10 * pLExp * log10((double) distance);
    if (shadowing)
        rxpwr += normal(0, pLStdDev);

    return rxpwr;
}

bool WirelessEtherModule::handleSendingBcastFrame(void)
{
    WirelessEtherBasicFrame *frame;
    frame = outputQueue->getReadyFrame();
    assert(frame);

    if (frame->getAddress1() == MACAddress6(WE_BROADCAST_ADDRESS))
    {
        if (frame->getFrameControl().subtype == ST_PROBEREQUEST)
        {
            // Start probe energy timeout when the probe frame is sent.
            // schedule the next scan message which will process the scan
            // after the SCAN_INTERVAL if nothing is received
            double nextSchedTime = simTime() + probeEnergyTimeout;

            assert(!prbEnergyScanNotifier->isScheduled());
            reschedule(prbEnergyScanNotifier, nextSchedTime);
        }

        assert(!outputQueue->getRetry());
        wEV << std::fixed << std::showpoint << setprecision(12)<< simTime() << " sec, " << fullPath() << " outputBuff count: " << outputQueue->size() << "\n";
        outputQueue->prepareNextFrame(simTime());

        // If there was a frame still oustanding to be received during the send state,
        // switch directly to receive state
        if (getNoOfRxFrames() > 0)
        {
            changeState(WirelessEtherStateReceive::instance());
        }
        else
        {
            changeState(WirelessEtherStateIdle::instance());
            static_cast<WirelessEtherStateIdle *>(_currentState)->chkOutputBuffer(this);
        }

        return true;
    }

    return false;
}

void WirelessEtherModule::scanNextChannel(void)
{
    assert(!apMode);

    reset();

    delete inputFrame;

    reschedule(prbEnergyScanNotifier, simTime() + SELF_SCHEDULE_DELAY);
}

void WirelessEtherModule::sendSuccessSignal(void)
{
    // XXX change these notifications to use the Blackboard --AV
    if (gate("extSignalOut") == NULL || gate("extSignalOut")->toGate() == NULL)
        return;
    cModule *linkLayer = gate("extSignalOut")->toGate()->ownerModule(); // XXX crashed if there was no extSignalOut[0] gate or it was not connected --AV

    // Check if linklayer external signalling channel is connected
    if (linkLayer->gate("extSignalOut")->isConnected())
    {
        WirelessExternalSignalConnectionStatus *extSig = new WirelessExternalSignalConnectionStatus;
        extSig->setType(ST_CONNECTION_STATUS);
        extSig->setState(S_ASSOCIATED);
        extSig->setConnectedAddress(associateAP.address);
        extSig->setConnectedChannel(associateAP.channel);
        extSig->setSignalStrength(associateAP.rxpower);
        extSig->setEstAvailBW(associateAP.estAvailBW);
        extSig->setAvgBackoffTime(associateAP.avgBackoffTime);
        extSig->setAvgWaitTime(associateAP.avgWaitTime);
        send(extSig, "extSignalOut", 0);
    }
}

void WirelessEtherModule::sendStatsSignal(void)
{
    cModule *linkLayer = gate("extSignalOut")->toGate()->ownerModule();

    // Check if linklayer external signalling channel is connected
    if (linkLayer->gate("extSignalOut")->isConnected())
    {
        WirelessExternalSignalStats *extSig = new WirelessExternalSignalStats;
        extSig->setType(ST_STATS);
        // Need to assign power. Keep track by monitoring received frames from associated AP.
        // If not associated, then put power as invalid or low.
        extSig->setSignalStrength(associateAP.rxpower);
        // extSig->setErrorPercentage(errorPercentage);
        // extSig->setAvgBackoffTime(totalBackoffTime.average);
        // extSig->setAvgWaitTime(totalWaitTime.average);
        send(extSig, "extSignalOut", 0);
    }
}

void WirelessEtherModule::startMonitorMode(void)
{
    associateAP.address = MAC_ADDRESS_UNSPECIFIED_STRUCT;
    associateAP.channel = INVALID_CHANNEL;
    associateAP.rxpower = INVALID_POWER;
    associateAP.associated = false;
    changeReceiveMode(WEMonitorReceiveMode::instance());
    changeState(WirelessEtherStateIdle::instance());

    // flush input and output buffer
    if (inputFrame)
    {
        delete inputFrame;
        inputFrame = 0;
    }

    std::list<WESignalData *>::iterator oit;

    outputQueue->reset();

    // cancel all timer message except for sending the end of frames
    cancelEvent(awaitAckTimer);
    cancelEvent(backoffTimer);
    cancelEvent(sendAckTimer);
    // cancelEvent(endSendAckTimer);
    // cancelEvent(endSendDataTimer);
    cancelEvent(updateStatsTimer);

    cancelEvent(powerUpBeaconNotifier);
    cancelEvent(prbEnergyScanNotifier);
    cancelEvent(prbRespScanNotifier);
    cancelEvent(channelScanNotifier);
    cancelEvent(authTimeoutNotifier);
    cancelEvent(assTimeoutNotifier);

    wEV << currentTime() << " " << fullPath() << " Monitor mode has been (re-)started, scanning channel: "<<channel << "\n";
}

void WirelessEtherModule::restartScanning(void)
{
    if (associateAP.associated)
    {
        if (gate("extSignalOut") != NULL && gate("extSignalOut")->toGate() != NULL)
        {
            cModule *linkLayer = gate("extSignalOut")->toGate()->ownerModule();

            // Check if linklayer external signalling channel is connected
            if (linkLayer->gate("extSignalOut")->isConnected())
            {
                WirelessExternalSignalConnectionStatus *extSig = new WirelessExternalSignalConnectionStatus;
                extSig->setType(ST_CONNECTION_STATUS);
                extSig->setState(S_DISCONNECTED);
                send(extSig, "extSignalOut", 0);
            }
        }
    }

    //record L2 down event by sending L2 down trigger
    linkdownTime = simTime();
    cMessage *linkDownTimeMsg = new cMessage;
    linkDownTimeMsg->setTimestamp();
    linkDownTimeMsg->setKind(LinkDOWN);
    sendDirect(linkDownTimeMsg,
	       0, OPP_Global::findModuleByName(this, "mobility"), "l2TriggerIn");

    if (ev.isGUI())
    {
      bubble("Active scan!");
      parentModule()->bubble("Active scan!");
      parentModule()->parentModule()->bubble("Active scan!");
    }

    associateAP.address = MAC_ADDRESS_UNSPECIFIED_STRUCT;
    associateAP.channel = INVALID_CHANNEL;
    associateAP.rxpower = INVALID_POWER;
    associateAP.associated = false;
    channel = 0;

    // flush input and output buffer

    if (inputFrame)
    {
        delete inputFrame;
        inputFrame = 0;
    }

    std::list<WESignalData *>::iterator oit;

    outputQueue->reset();

    double nextSchedTime = simTime();

    // cancel all timer message except for sending the end of frames
    // FIXME merge with prev method!!! it also has exactly this code
    cancelEvent(awaitAckTimer);
    cancelEvent(backoffTimer);
    cancelEvent(sendAckTimer);
    // cancelEvent(endSendAckTimer);
    // cancelEvent(endSendDataTimer);
    cancelEvent(updateStatsTimer);

    cancelEvent(powerUpBeaconNotifier);
    cancelEvent(prbEnergyScanNotifier);
    cancelEvent(prbRespScanNotifier);
    cancelEvent(channelScanNotifier);
    cancelEvent(authTimeoutNotifier);
    cancelEvent(assTimeoutNotifier);

    if (activeScan)
    {
        changeReceiveMode(WEAScanReceiveMode::instance());
        nextSchedTime += SELF_SCHEDULE_DELAY;
        assert(!prbEnergyScanNotifier->isScheduled());
        wEV << currentTime() << " " << fullPath() << " Scheduled time for active scan is: " << nextSchedTime << "\n";
        scheduleAt(nextSchedTime, prbEnergyScanNotifier);
    }
    else
    {
        changeReceiveMode(WEPScanReceiveMode::instance());
        nextSchedTime += SELF_SCHEDULE_DELAY;
        assert(!channelScanNotifier->isScheduled());
        wEV << currentTime() << " " << fullPath() << " Scheduled time for passive scan is: " << nextSchedTime << "\n";
        scheduleAt(nextSchedTime, channelScanNotifier);
    }

}

void WirelessEtherModule::reset(void)
{
    // cancel all timer messages
    cancelEvent(awaitAckTimer);
    cancelEvent(backoffTimer);
    cancelEvent(sendAckTimer);
    cancelEvent(endSendAckTimer);
    cancelEvent(endSendDataTimer);
    cancelEvent(updateStatsTimer);

    cancelEvent(powerUpBeaconNotifier);
    cancelEvent(prbEnergyScanNotifier);
    cancelEvent(prbRespScanNotifier);
    cancelEvent(channelScanNotifier);
    cancelEvent(authTimeoutNotifier);
    cancelEvent(assTimeoutNotifier);

    // change state to IDLE
    changeState(WirelessEtherStateIdle::instance());
}

bool WirelessEtherModule::isFrameForMe(WirelessEtherBasicFrame *chkFrame)
{
    return (chkFrame->getAddress1() == address ||
            chkFrame->getAddress1() == MACAddress6(WE_BROADCAST_ADDRESS));
}

// Find the AP with the highest signal strength
bool WirelessEtherModule::highestPowerAPEntry(APInfo & highest)
{
    if (!tempAPList.empty())
    {
        double highestPower = (tempAPList.begin())->rxpower;
        highest = *(tempAPList.begin());

        for (AIT it = tempAPList.begin(); it != tempAPList.end(); it++)
        {
            if ((*it).rxpower > highestPower)
            {
                highest = (*it);
                highestPower = (*it).rxpower;
            }
        }
        return true;
    }
    return false;
}

// Find the AP with the highest handover value
bool WirelessEtherModule::highestHOValueAPEntry(APInfo & highest)
{
    if (!tempAPList.empty())
    {
        double highestHOValue = (tempAPList.begin())->hOValue;
        highest = *(tempAPList.begin());

        for (AIT it = tempAPList.begin(); it != tempAPList.end(); it++)
        {
            if ((*it).hOValue > highestHOValue)
            {
                highest = (*it);
                highestHOValue = (*it).hOValue;
            }
        }
        return true;
    }
    return false;
}

// Find the AP with the matching MAC address
bool WirelessEtherModule::findAPEntry(APInfo & target)
{
    if (!tempAPList.empty())
    {
        for (AIT it = tempAPList.begin(); it != tempAPList.end(); it++)
        {
            if ((*it).address == target.address)
            {
                target = (*it);
                return true;
            }
        }
    }
    return false;
}

void WirelessEtherModule::probeChannel(void)
{
    if (inputFrame)
    {
        delete inputFrame;
        inputFrame = 0;
    }

    // reset all timers
    reset();

    assert(!apMode);
    assert(channel <= MAX_CHANNEL && channel >= 0);

    // find the next channel to scan
    int channelCopy = channel;
    channel = MAX_CHANNEL + 1;

    for (int i = channelCopy; i < MAX_CHANNEL; i++)
    {
        channelCopy++;
        if (channelToScan[channelCopy])
        {
            channel = channelCopy;
            break;
        }
    }

    // Once new channel to scan has been decided, dont care about packets still received,
    // so count can be resetted. Could cause miss count otherwise.
    resetNoOfRxFrames();

    // all frequency bands have been scanned and now select the best AP
    // if ( channel == (MAX_CHANNEL + 1) )
    if ((channel == (MAX_CHANNEL + 1)) || (scanShortCircuit() && tempAPList.size()))    // GD Hack
        // Hack Assumes that Any access point with sufficient power will be
        // Acceptable.  If not, need to jump back into scanning (if channels left)...
        // Steve: With hack, scanning will stop as long as one probe is received. It may
        //        not have sufficient power.
    {
        bool found = false;
        // reset the channel
        channel = 0;

        // look for a specified AP if the target is valid
        if (handoverTarget.valid)
        {
            found = findAPEntry(handoverTarget.target);
        }
        else
        {
            // search for AP with best signal strength if no target AP is specified
#if L2FUZZYHO                   // (Layer 2 fuzzy logic handover)
            found = highestHOValueAPEntry(handoverTarget.target);
#else
            found = highestPowerAPEntry(handoverTarget.target);
#endif // L2FUZZYHO
        }
        // only attempt to connect if a suitable AP is found
        if (found)
        {

            // Only attempt to connect if the signal strength is above handover threshold
            if (handoverTarget.target.rxpower > hothreshpower)
            {
                associateAP = handoverTarget.target;
#if MLDV2
                cout << "handoverTarget.target.rxpower > hothreshpower, at " << simTime() << endl;
                cout << "MAC:" << associateAP.address.stringValue() << endl;

                sendGQtoUpperLayer();
#endif // MLDV2
                assert(((MAC_address) associateAP.address != MAC_ADDRESS_UNSPECIFIED_STRUCT) ||
                      (associateAP.channel != INVALID_CHANNEL) ||
                      (associateAP.rxpower != INVALID_POWER));

                // clear tempAPList
                tempAPList.clear();
                wEV  << OPP_Global::nodeName(this) << " \n"
                     << " ---------------------------------------------------- \n"
                     << " active scan COMPLETE.. Access Point found: \n"
                     << " AP MAC: " << associateAP.address.stringValue() << "\n"
                     << " operating channel: " << associateAP.channel << "\n"
                     << " ---------------------------------------------------- \n";
		
                changeReceiveMode(WEAuthenticationReceiveMode::instance());
                assert(_currentState == WirelessEtherStateIdle::instance());

                channel = associateAP.channel;

		if (noAuth)
		{
		  changeReceiveMode(WEAssociationReceiveMode::instance());
		  WirelessEtherBasicFrame *assRequest = 
		    createFrame(FT_MANAGEMENT, ST_ASSOCIATIONREQUEST,
				     MACAddress6(macAddressString().
						 c_str()), AC_VO,
				     associateAP.address);
		  FrameBody *assRequestFrameBody = createFrameBody(assRequest);
		  assRequest->encapsulate(assRequestFrameBody);
		  outputQueue->insertFrame(assRequest, simTime());

		  // Start the association timeout timer
		  reschedule(assTimeoutNotifier, simTime() + (associationTimeout * TU));
		  return;
		}

                // Create authentication frame and send it
                WirelessEtherBasicFrame *authentication =
                    createFrame(FT_MANAGEMENT, ST_AUTHENTICATION, address, AC_VO,
                                associateAP.address);
                FrameBody *authFrameBody = createFrameBody(authentication);
                authentication->encapsulate(authFrameBody);
                outputQueue->insertFrame(authentication, simTime());

                // Start the authentication timeout timer
                reschedule(authTimeoutNotifier, simTime() + (authenticationTimeout * TU));

                assert(_currentState == WirelessEtherStateIdle::instance());

                static_cast<WirelessEtherStateIdle *>(_currentState)->chkOutputBuffer(this);

                return;
            }
        }

        // reset all current CSMA/CA values
        reset();
        outputQueue->reset();

        wEV << fullPath() << " No suitable access point was found, performing active scan again.\n";

        double nextSchedTime = simTime() + SELF_SCHEDULE_DELAY;
        assert(!prbEnergyScanNotifier->isScheduled());
        reschedule(prbEnergyScanNotifier, nextSchedTime);

        static_cast<WirelessEtherStateIdle *>(_currentState)->chkOutputBuffer(this);
        return;
    }

    wEV << currentTime() << " " << fullPath() << " scanning channel: " << channel << "\n";

    outputQueue->insertFrame(generateProbeReq(), simTime());

    static_cast<WirelessEtherStateIdle *>(_currentState)->chkOutputBuffer(this);
}

void WirelessEtherModule::passiveChannelScan(void)
{
    if (inputFrame)
    {
        delete inputFrame;
        inputFrame = 0;
    }

    // reset all current CSMA/CA values
    reset();

    assert(!apMode);
    assert(channel <= MAX_CHANNEL && channel >= 0);

    // find the next channel to scan
    int channelCopy = channel;
    channel = MAX_CHANNEL + 1;

    for (int i = channelCopy; i < MAX_CHANNEL; i++)
    {
        channelCopy++;
        if (channelToScan[channelCopy])
        {
            channel = channelCopy;
            break;
        }
    }

    // Once new channel to scan has been decided, dont care about packets still received,
    // so count can be resetted. Could cause miss count otherwise.
    resetNoOfRxFrames();

    // all frequency bands have been scanned and now select the best AP
    // if ( channel == (MAX_CHANNEL + 1) )
    if ((channel == (MAX_CHANNEL + 1)) || (scanShortCircuit() && tempAPList.size()))    // GD Hack
        // Hack Assumes that Any access point with sufficient power will be
        // Acceptable.  If not, need to jump back into scanning (if channels left)...
        // Steve: With hack, scanning will stop as long as one probe is received. It may
        //        not have sufficient power.
    {
        bool found = false;
        // reset the channel
        channel = 0;

        // look for a specified AP if the target is valid
        if (handoverTarget.valid)
        {
            found = findAPEntry(handoverTarget.target);
        }
        else
        {
            // search for AP with best signal strength if no target AP is specified
#if L2FUZZYHO                   // (Layer 2 fuzzy logic handover)
            found = highestHOValueAPEntry(handoverTarget.target);
#else
            found = highestPowerAPEntry(handoverTarget.target);
#endif // L2FUZZYHO
        }
        // only attempt to connect if a suitable AP is found
        if (found)
        {

            // Only attempt to connect if the signal strength is above handover threshold
            if (handoverTarget.target.rxpower > hothreshpower)
            {
                associateAP = handoverTarget.target;

                assert(((MAC_address) associateAP.address != MAC_ADDRESS_UNSPECIFIED_STRUCT) ||
                       (associateAP.channel != INVALID_CHANNEL) || (associateAP.rxpower != INVALID_POWER));

                // clear tempAPList
                tempAPList.clear();

                wEV  << OPP_Global::nodeName(this) << " \n"
                     << " ---------------------------------------------------- \n"
                     << " Passive scan COMPLETE.. Access Point found: \n"
                     << " AP MAC: " << associateAP.address.stringValue() << "\n"
                     << " operating channel: " << associateAP.channel << "\n"
                     << " ---------------------------------------------------- \n";

                changeReceiveMode(WEAuthenticationReceiveMode::instance());
                assert(_currentState == WirelessEtherStateIdle::instance());

                channel = associateAP.channel;

                // Create authentication frame and send it
                WirelessEtherBasicFrame *authentication =
                    createFrame(FT_MANAGEMENT, ST_AUTHENTICATION, address, AC_VO,
                                associateAP.address);
                FrameBody *authFrameBody = createFrameBody(authentication);
                authentication->encapsulate(authFrameBody);
                outputQueue->insertFrame(authentication, simTime());

                // Start the authentication timeout timer
                reschedule(authTimeoutNotifier, simTime() + (authenticationTimeout * TU));

                assert(_currentState == WirelessEtherStateIdle::instance());

                static_cast<WirelessEtherStateIdle *>(_currentState)->chkOutputBuffer(this);

                return;
            }
        }

        // reset all current CSMA/CA values
        reset();

        outputQueue->reset();

        wEV << fullPath() << " No suitable access point was found, performing passive scan again.\n";
        double nextSchedTime = simTime() + SELF_SCHEDULE_DELAY;
        assert(!channelScanNotifier->isScheduled());
        reschedule(channelScanNotifier, nextSchedTime);

        static_cast<WirelessEtherStateIdle *>(_currentState)->chkOutputBuffer(this);

        return;
    }

    wEV << currentTime() << " " << fullPath() << " passive scanning channel: " << channel << "\n";

    double nextSchedTime = simTime() + channelScanTime;
    assert(!channelScanNotifier->isScheduled());
    reschedule(channelScanNotifier, nextSchedTime);

    static_cast<WirelessEtherStateIdle *>(_currentState)->chkOutputBuffer(this);
}

void WirelessEtherModule::authTimeoutHandler(void)
{
    wEV << currentTime() << " " << fullPath() << " Authentication timed out by: " << address << "\n";
    restartScanning();
}

void WirelessEtherModule::assTimeoutHandler(void)
{
    wEV << currentTime() << " " << fullPath() << " Association timed out by: " << address << "\n";
    restartScanning();
}

void WirelessEtherModule::printSelfMsg(const cMessage *msg)
{
    const char *state;
    int messageID = msg->kind();

    if (currentState() == WirelessEtherStateIdle::instance())
        state = "IDLE";
    else if (currentState() == WirelessEtherStateSend::instance())
        state = "SEND";
    else if (currentState() == WirelessEtherStateBackoff::instance())
        state = "BACKOFF";
    else if (currentState() == WirelessEtherStateAwaitACK::instance())
        state = "AWAITACK";
    else if (currentState() == WirelessEtherStateAwaitACKReceive::instance())
        state = "AWAITACKRECEIVE";
    else if (currentState() == WirelessEtherStateBackoffReceive::instance())
        state = "BACKOFFRECEIVE";
    else if (currentState() == WirelessEtherStateReceive::instance())
        state = "RECEIVE";
    else
        assert(false);

    string message;

    if (messageID == WE_TRANSMIT_SENDDATA)
        message = "WE_TRANSMIT_SENDDATA ( " + string(msg->name()) + string(" )");
    else if (messageID == TMR_PRBENERGYSCAN)
        message = "TMR_PRBENERGYSCAN ( " + string(msg->name()) + string(" )");
    else if (messageID == TMR_PRBRESPSCAN)
        message = "TMR_PRBRESPSCAN ( " + string(msg->name()) + string(" )");
    else if (messageID == TMR_CHANNELSCAN)
        message = "TMR_CHANNELSCAN ( " + string(msg->name()) + string(" )");
    else if (messageID == TMR_BEACON)
        message = "TMR_BEACON ( " + string(msg->name()) + string(" )");
    else if (messageID == TMR_REMOVEENTRY)
        message = "TMR_REMOVEENTRY ( " + string(msg->name()) + string(" )");
    else if (messageID == TMR_AUTHTIMEOUT)
        message = "TMR_AUTHTIMEOUT ( " + string(msg->name()) + string(" )");
    else if (messageID == TMR_ASSTIMEOUT)
        message = "TMR_ASSTIMEOUT ( " + string(msg->name()) + string(" )");
    else if (messageID == TMR_STATS)
        message = "TMR_STATS ( " + string(msg->name()) + string(" )");
    else if (messageID == WIRELESS_SELF_AWAITMAC)
        message = "WIRELESS_SELF_AWAITMAC ( " + string(msg->name()) + string(" )");
    else if (messageID == WIRELESS_SELF_BACKOFF)
        message = "WIRELESS_SELF_BACKOFF ( " + string(msg->name()) + string(" )");
    else if (messageID == WIRELESS_SELF_AWAITACK)
        message = "WIRELESS_SELF_AWAITACK ( " + string(msg->name()) + string(" )");
    else if (messageID == WIRELESS_SELF_ENDSENDACK)
        message = "WIRELESS_SELF_ENDSENDACK ( " + string(msg->name()) + string(" )");
    else if (messageID == WIRELESS_SELF_SCHEDULEACK)
        message = "WIRELESS_SELF_SCHEDULEACK ( " + string(msg->name()) + string(" )");
    else if (messageID == 0)
    {
      //prob. ExpiryEntryListSignal
    }
    else
        assert(false);          // notify any new message added

    wEV << fullPath() << " receiving self message with ID: " << message.c_str() << " in " << state << " state\n";
}

void WirelessEtherModule::sendToUpperLayer(WirelessEtherBasicFrame *frame)
{
    cMessage *dgram = frame->decapsulate();
    send(dgram, inputQueueOutGate());
}

// Sends monitored frame to upper layer un-modified
void WirelessEtherModule::sendMonitorFrameToUpperLayer(WESignalData *sig)
{
    wEV << currentTime() << " " << fullPath() << "Sending monitor frame to upper layer.\n";

    send((cMessage *) sig->dup(), inputQueueOutGate());
}

#if MLDV2
void WirelessEtherModule::sendGQtoUpperLayer()
{
    MLDv2Message *GQmsg = new MLDv2Message(ICMPv6_MLD_QUERY, 20);

    cout << endl << OPP_Global::findNetNodeModule(this)->name() << " sendGQtoUpperLayer(), at simTime:" << simTime() << endl;
//  cout << "_NMAR:" << LStable->NMAR() << endl;

    GQmsg->setLength(28 * 8);

    GQmsg->setMaxRspCode(1000); // query response interval
    GQmsg->setMA(c_ipv6_addr(0));
    GQmsg->setS_Flag(false);
    GQmsg->setQRV(2);
    GQmsg->setQQIC(125);
    GQmsg->setNS(0);

    IPv6Datagram *dgram = new IPv6Datagram;     // well, THIS should GO! --AV

    dgram->encapsulate(GQmsg);
    dgram->setPayloadLength(GQmsg->length());
    dgram->setDestAddress(c_ipv6_addr("FF02:0:0:0:0:0:0:1"));
    dgram->setSrcAddress(c_ipv6_addr("0:0:0:0:0:0:0:1"));
    dgram->setTransportProtocol(IP_PROT_IPv6_ICMP);

    send(dgram, inputQueueOutGate());
}
#endif

WirelessEtherBasicFrame *WirelessEtherModule::generateProbeReq(void)
{
    WirelessEtherBasicFrame *probeFrame = createFrame(FT_MANAGEMENT, ST_PROBEREQUEST, address, AC_VO,
                                                      MACAddress6(WE_BROADCAST_ADDRESS));
    FrameBody *probeFrameBody = createFrameBody(probeFrame);
    probeFrame->encapsulate(probeFrameBody);

    return probeFrame;
}

WirelessEtherBasicFrame *WirelessEtherModule::createFrame(
            FrameType frameType, SubType subType,
            MACAddress6 source, int frameAppType, MACAddress6 destination)
{
    WirelessEtherBasicFrame *frame;
    FrameControl frameControl;
    DurationID durationID;
    SequenceControl sequenceControl;

    switch (frameType)
    {
    case FT_CONTROL:
        switch (subType)
        {
        case ST_ACK:
            frame = new WirelessEtherBasicFrame;

            frameControl.protocolVer = 0;
            frameControl.type = frameType;
            frameControl.subtype = subType;
            frameControl.toDS = false;
            frameControl.fromDS = false;
            frameControl.retry = false;

            static_cast<WirelessEtherBasicFrame *>(frame)->setFrameControl(frameControl);

            // always 0 since moreFrag not implemented
            durationID.bit15 = false;
            durationID.bit14 = false;
            durationID.bit14to0 = 0;

            static_cast<WirelessEtherBasicFrame *>(frame)->setDurationID(durationID);

            assert(inputFrame->encapsulatedMsg());
            static_cast<WirelessEtherBasicFrame *>(frame)->
                setAddress1(static_cast<WirelessEtherRTSFrame *>(inputFrame->encapsulatedMsg())->getAddress2());

            frame->setLength(FL_FRAMECTRL + FL_DURATIONID + FL_ADDR1 + FL_FCS);
            break;
        default:
            assert(false);
        }
        break;

        /*case FT_CTS:

           frame->setLength(FL_FRAMECTRL + FL_DURATIONID + FL_ADDR1 + FL_FCS);
           break;
           case FT_RTS:

           frame->setLength(FL_FRAMECTRL + FL_DURATIONID + FL_ADDR1 + FL_ADDR2 + FL_FCS);
           break; */

    case FT_MANAGEMENT:
        frame = new WirelessEtherManagementFrame;

        frameControl.protocolVer = 0;
        frameControl.type = frameType;
        frameControl.subtype = subType;
        frameControl.toDS = false;
        frameControl.fromDS = false;
        frameControl.retry = false;     // set to true for retries elsewhere

        frame->setLength(FL_FRAMECTRL + FL_DURATIONID + FL_ADDR1 + FL_ADDR2 + FL_ADDR3 + FL_SEQCTRL + FL_FCS);

        static_cast<WirelessEtherManagementFrame *>(frame)->setFrameControl(frameControl);

        durationID.bit15 = false;

        // need to determine ACK + SIFS time check for multicast not
        // just broadcast as well; note also that having no moreFrag
        // support, eliminates other potential values for duration
        (destination == MACAddress6(WE_BROADCAST_ADDRESS)) ? durationID.bit14to0 = 0 : durationID.bit14to0 = (unsigned) ((ACKLENGTH / BASE_SPEED) + SIFS) * 1000;       // add ACK

        static_cast<WirelessEtherManagementFrame *>(frame)->setDurationID(durationID);
        static_cast<WirelessEtherManagementFrame *>(frame)->setAddress1(destination);
        static_cast<WirelessEtherManagementFrame *>(frame)->setAddress2(address);

        if (subType == ST_PROBEREQUEST)
        {
            static_cast<WirelessEtherManagementFrame *>(frame)->setAddress3(destination);
        }
        else
        {
            (apMode == true) ?
                static_cast<WirelessEtherManagementFrame *>(frame)->setAddress3(address) :
                static_cast<WirelessEtherManagementFrame *>(frame)->setAddress3(associateAP.address);
        }

        sequenceControl.fragmentNumber = 0;
        sequenceControl.sequenceNumber = sequenceNumber;
        incrementSequenceNumber();
        static_cast<WirelessEtherManagementFrame *>(frame)->setSequenceControl(sequenceControl);

        frame->setLength(FL_FRAMECTRL + FL_DURATIONID + FL_ADDR1 + FL_ADDR2 + FL_ADDR3 + FL_SEQCTRL + FL_FCS);
        break;

    case FT_DATA:
        frame = new WirelessEtherDataFrame;

        frameControl.protocolVer = 0;
        frameControl.type = FT_DATA;
        frameControl.subtype = ST_DATA; // since of data type

        if (apMode == true)
        {
            frameControl.toDS = false;
            frameControl.fromDS = true;
            static_cast<WirelessEtherDataFrame *>(frame)->setAddress1(destination);
            static_cast<WirelessEtherDataFrame *>(frame)->setAddress2(address);
            static_cast<WirelessEtherDataFrame *>(frame)->setAddress3(source);
        }
        else
        {
            frameControl.toDS = true;
            frameControl.fromDS = false;
            static_cast<WirelessEtherDataFrame *>(frame)->setAddress1(associateAP.address);
            static_cast<WirelessEtherDataFrame *>(frame)->setAddress2(source);
            static_cast<WirelessEtherDataFrame *>(frame)->setAddress3(destination);
        }
        frameControl.retry = false;     // set to true for retries elsewhere
        static_cast<WirelessEtherDataFrame *>(frame)->setFrameControl(frameControl);

        durationID.bit15 = false;
        // need to determine ACK + SIFS time
        // must check for multicast not just broadcast
        // note also that having no moreFrag support, eliminates other
        // potential values for duration
        (destination == MACAddress6(WE_BROADCAST_ADDRESS)) ? durationID.bit14to0 = 0 : durationID.bit14to0 = (unsigned) ((ACKLENGTH / BASE_SPEED) + SIFS) * 1000;       // add ACK

        static_cast<WirelessEtherDataFrame *>(frame)->setDurationID(durationID);
        sequenceControl.fragmentNumber = 0;     // fragmentation not supported
        sequenceControl.sequenceNumber = sequenceNumber;
        incrementSequenceNumber();

        // need to decide how to keep track of sequence in module
        static_cast<WirelessEtherDataFrame *>(frame)->setSequenceControl(sequenceControl);

        // 304 bits
        frame->setLength(FL_FRAMECTRL + FL_DURATIONID + FL_ADDR1 + FL_ADDR2 +
                         FL_ADDR3 + FL_ADDR4 + FL_SEQCTRL + FL_FCS);
        break;

    default:
        assert(false);
        break;
    }
    // XXX frame->setProtocol(PR_WETHERNET);

    // SWOON HACK: To find achievable throughput
    frame->setProbTxInSlot(probTxInSlot);
    frame->setAppType(frameAppType);
    frame->setAvgFrameSize(outputQueue->getAvgFrameSize(frameAppType));

    return frame;
}

FrameBody *WirelessEtherModule::createFrameBody(WirelessEtherBasicFrame *f)
{
    FrameBody *frameBody;
    CapabilityInformation capabilityInfo;
    unsigned int i;

    switch (f->getFrameControl().subtype)
    {
    case ST_ASSOCIATIONREQUEST:
        frameBody = new AssociationRequestFrameBody;

        capabilityInfo.ESS = !adhocMode;
        capabilityInfo.IBSS = adhocMode;
        capabilityInfo.CFPollable = false;      // CF not implemented
        capabilityInfo.CFPollRequest = false;
        capabilityInfo.privacy = false; // wep not implemented

        static_cast<AssociationRequestFrameBody *>(frameBody)->setCapabilityInformation(capabilityInfo);
        static_cast<AssociationRequestFrameBody *>(frameBody)->setSSID(ssid.c_str());
        static_cast<AssociationRequestFrameBody *>(frameBody)->setSupportedRatesArraySize(rates.size());

        for (i = 0; i < rates.size(); i++)
            static_cast<AssociationRequestFrameBody *>(frameBody)->setSupportedRates(i, rates[i]);

        frameBody->setLength(FL_CAPINFO + FL_IEHEADER + ssid.length() + FL_IEHEADER + rates.size());
        break;

    case ST_REASSOCIATIONREQUEST:
        frameBody = new ReAssociationRequestFrameBody;

        static_cast<ReAssociationRequestFrameBody *>(frameBody)->setCurrentAP(associateAP.address);

        capabilityInfo.ESS = !adhocMode;
        capabilityInfo.IBSS = adhocMode;
        capabilityInfo.CFPollable = false;      // CF not implemented
        capabilityInfo.CFPollRequest = false;
        capabilityInfo.privacy = false; // wep not implemented

        static_cast<ReAssociationRequestFrameBody *>(frameBody)->setCapabilityInformation(capabilityInfo);
        static_cast<ReAssociationRequestFrameBody *>(frameBody)->setSSID(ssid.c_str());
        static_cast<ReAssociationRequestFrameBody *>(frameBody)->setSupportedRatesArraySize(rates.size());

        for (i = 0; i < rates.size(); i++)
            static_cast<ReAssociationRequestFrameBody *>(frameBody)->setSupportedRates(i, rates[i]);

        frameBody->setLength(FL_CAPINFO + FL_CURRENTAP + FL_IEHEADER +
                             ssid.length() + FL_IEHEADER + rates.size());
        break;

    case ST_PROBEREQUEST:
        frameBody = new ProbeRequestFrameBody;

        static_cast<ProbeRequestFrameBody *>(frameBody)->setSSID(ssid.c_str());
        static_cast<ProbeRequestFrameBody *>(frameBody)->setSupportedRatesArraySize(rates.size());

        for (i = 0; i < rates.size(); i++)
            static_cast<ProbeRequestFrameBody *>(frameBody)->setSupportedRates(i, rates[i]);

        frameBody->setLength(FL_IEHEADER + ssid.length() + FL_IEHEADER + rates.size());
        break;

    case ST_AUTHENTICATION:
        frameBody = new AuthenticationFrameBody;

        // Only supports "open mode", therefore only two sequence number
        if (apMode == true)
            static_cast<AuthenticationFrameBody *>(frameBody)->setSequenceNumber(2);
        else
            static_cast<AuthenticationFrameBody *>(frameBody)->setSequenceNumber(1);

        // TODO: successful for now, but how are we going to represent
        // in module?
        static_cast<AuthenticationFrameBody *>(frameBody)->setStatusCode(0);

        frameBody->setLength(FL_STATUSCODE + FL_SEQNUM);
        break;

    default:
        assert(false);
        break;
    }
    return frameBody;
}

void WirelessEtherModule::decodeFrame(WESignalData *signal)
{
    _currentReceiveMode->decodeFrame(this, signal);
}

bool WirelessEtherModule::isProbeReq(WESignalData *signal)
{
    WirelessEtherBasicFrame *frame = check_and_cast<WirelessEtherBasicFrame *>(signal->encapsulatedMsg());

    if (frame->getFrameControl().subtype == ST_PROBEREQUEST)
        return true;

    return false;
}

void WirelessEtherModule::initialiseChannelToScan(void)
{
    // Initialise all channels for scanning except channel 0
  channelToScan = new bool[MAX_CHANNEL + 1];
    channelToScan[0] = false;
    for (int i = 1; i <= MAX_CHANNEL; i++)
    {
        channelToScan[i] = true;
    }

    // Mark unwanted channels based on XML input
    cStringTokenizer tokenizer(chanNotToScan.c_str(), "-");
    const char *token;
    while ((token = tokenizer.nextToken()) != NULL)
        channelToScan[atoi(token)] = false;
}

void WirelessEtherModule::insertToAPList(APInfo newEntry)
{
    // If short circuit hack is turned on, ensure tempAPList only contains entries
    // which are "good enough"(i.e. above handover threshold power)
    if (!scanShortCircuit() || (scanShortCircuit() && (newEntry.rxpower > hothreshpower)))
    {
        for (AIT it = tempAPList.begin(); it != tempAPList.end(); it++)
        {
            // Overwrite old entry.
            if ((it->address == newEntry.address) && (it->channel == newEntry.channel))
            {
                it->rxpower = newEntry.rxpower;
                it->hOValue = newEntry.hOValue;
                return;
            }
        }
        // Insert new entry.
        tempAPList.push_back(newEntry);
    }
}

// Transfer frames to stored in the offlineBuffer to the outputQueue
void WirelessEtherModule::makeOfflineBufferAvailable(void)
{
    WirelessEtherBasicFrame *frame;

    while (!offlineOutputBuffer.empty())
    {
        frame = offlineOutputBuffer.front();
        // The offline output buffer only gets filled with frames encapsulating
        // data from upper layers.  It is safe to assume that they all have
        // FT_DATA type. Therefore, we can assume address1 is used for the AP
        // address.
        frame->setAddress1(associateAP.address);
        outputQueue->insertFrame(frame, simTime());
        offlineOutputBuffer.pop_front();
    }
}

void WirelessEtherModule::updateStats(void)
{
    // record averages for overall simulation time
    if (simTime() > samplingStartTime)
    {
        avgRxDataBWStat->collect(RxDataBWStat / statsUpdatePeriod);
        avgTxDataBWStat->collect(TxDataBWStat / statsUpdatePeriod);
        avgTxFrameRateStat->collect(txSuccess);
        avgNoOfRxStat->collect(noOfRxStat / statsUpdatePeriod);
        avgNoOfTxStat->collect(noOfTxStat / statsUpdatePeriod);
        avgNoOfFailedTxStat->collect(noOfFailedTxStat / statsUpdatePeriod);
        avgOutBuffSizeBEStat->collect(outputQueue->sizeOfQ(AC_BE));
        avgOutBuffSizeVIStat->collect(outputQueue->sizeOfQ(AC_VI));
        avgOutBuffSizeVOStat->collect(outputQueue->sizeOfQ(AC_VO));
    }
    // SWOON HACK: To find achievable throughput
    probTxInSlot = outputQueue->getProbTxInSlot(appType, simTime(), this);

    // record vectors if turned ON
    if (statsVec)
    {
        RxDataBWVec->record(RxDataBWStat / statsUpdatePeriod);
        TxDataBWVec->record(TxDataBWStat / statsUpdatePeriod);
        noOfFailedTxVec->record(noOfFailedTxStat / statsUpdatePeriod);
        RxFrameSizeVec->record(RxFrameSizeStat->mean());
        TxFrameSizeVec->record(TxFrameSizeStat->mean());
        TxFrameRateVec->record(txSuccess);
        TxAccessTimeVec->record(TxAccessTimeStat->mean());
        backoffSlotsVec->record(backoffSlotsStat->mean());
        CWVec->record(CWStat->mean());

        if (outputQueue->getAttempted(AC_BE) > 0)
        {
            avgTxFrameSizeBEVec->record(outputQueue->getAvgFrameSize(AC_BE));
            noOfAttemptedBEVec->record(outputQueue->getAttempted(AC_BE));
            noOfCollisionBEVec->record(outputQueue->getCollided(AC_BE));
            probTxInSlotBEVec->record(outputQueue->getProbTxInSlot(AC_BE, simTime(), this));
            lambdaBEVec->record(outputQueue->getLambda(AC_BE));
            outBuffSizeBEVec->record(outputQueue->sizeOfQ(AC_BE));
        }
        if (outputQueue->getAttempted(AC_VI) > 0)
        {
            avgTxFrameSizeVIVec->record(outputQueue->getAvgFrameSize(AC_VI));
            noOfAttemptedVIVec->record(outputQueue->getAttempted(AC_VI));
            noOfCollisionVIVec->record(outputQueue->getCollided(AC_VI));
            probTxInSlotVIVec->record(outputQueue->getProbTxInSlot(AC_VI, simTime(), this));
            lambdaVIVec->record(outputQueue->getLambda(AC_VI));
            outBuffSizeVIVec->record(outputQueue->sizeOfQ(AC_VI));
        }
        if (outputQueue->getAttempted(AC_VO) > 0)
        {
            avgTxFrameSizeVOVec->record(outputQueue->getAvgFrameSize(AC_VO));
            noOfAttemptedVOVec->record(outputQueue->getAttempted(AC_VO));
            noOfCollisionVOVec->record(outputQueue->getCollided(AC_VO));
            probTxInSlotVOVec->record(outputQueue->getProbTxInSlot(AC_VO, simTime(), this));
            lambdaVOVec->record(outputQueue->getLambda(AC_VO));
            outBuffSizeVOVec->record(outputQueue->sizeOfQ(AC_VO));
        }
    }

    // reset stats updated every period
    RxDataBWStat = 0;
    TxDataBWStat = 0;
    noOfRxStat = 0;
    noOfTxStat = 0;
    noOfFailedTxStat = 0;
    txSuccess = 0;
    RxFrameSizeStat->clearResult();
    TxFrameSizeStat->clearResult();
    backoffSlotsStat->clearResult();
    CWStat->clearResult();

    // reschedule next update
    scheduleAt(simTime() + statsUpdatePeriod, updateStatsTimer);
}

void WirelessEtherModule::changeReceiveMode(WEReceiveMode* mode)
{
  _currentReceiveMode = mode;
}


#if L2FUZZYHO                   // (Layer 2 fuzzy logic handover)
double WirelessEtherModule::calculateHOValue(double rxpower, double ap_avail_bw, double bw_req)
{
    double n_rxpower, n_ap_avail_bw, n_bw_req;
    double value;
    hodec fuzSys;

    n_rxpower = (rxpower - hothreshpower) / (-10 - hothreshpower);      // assuming -10 is highest power in dB
    n_ap_avail_bw = (ap_avail_bw / 11); // assuming max avail bw is 11Mb/s
    n_bw_req = bw_req;

    if (n_rxpower < 0)
        n_rxpower = 0;
    fuzSys.inference(n_bw_req, n_rxpower, n_ap_avail_bw, &value);

    return value;
}
#endif // L2FUZZYHO
