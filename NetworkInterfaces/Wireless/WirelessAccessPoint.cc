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
    @file WirelessAccessPoint.cc
    @brief Definition file for WirelessAccessPoint

    simple implementation of access point module

    @author Eric Wu
*/

#include "sys.h"
#include "debug.h"


#include <iostream>
#include <iomanip>

#include "WirelessAccessPoint.h"
#include <string>

#include "WirelessEtherBridge.h"

#include "cTTimerMessageCB.h"


#include "wirelessethernet.h"
#include "WorldProcessor.h"
#include "opp_utils.h"
#include "wirelessethernet.h"
#include "MobilityHandler.h"
#include "WirelessEtherSignal_m.h"
#include "WirelessEtherStateIdle.h"
#include "MACAddress6.h"
#include "WirelessEtherAPReceiveMode.h"
#include <memory>

#include "ExpiryEntryList.h"
#include "WEQueue.h"

Define_Module(WirelessAccessPoint);

const WirelessEtherInterface UNSPECIFIED_WIRELESS_ETH_IFACE = {
    MACAddress6(MAC_ADDRESS_UNSPECIFIED_STRUCT),
    RM_UNSPECIFIED,
    0,
    0,
    0,
    RC_UNSPECIFIED,
    SC_UNSPECIFIED
};

bool operator==(const WirelessEtherInterface & lhs, const WirelessEtherInterface & rhs)
{
    return (lhs.address == rhs.address);
}

bool operator!=(const WirelessEtherInterface & lhs, const WirelessEtherInterface & rhs)
{
    return !(lhs.address == rhs.address);
}

bool operator<(const WirelessEtherInterface & lhs, const WirelessEtherInterface & rhs)
{
    return (lhs.expire < rhs.expire);
}

WirelessAccessPoint::~WirelessAccessPoint()
{
    delete avgCollDurationVec;
    delete achievableThroughputBEVec;
    delete achievableThroughputVIVec;
    delete achievableThroughputVOVec;

    delete noOfVIStat;
    delete noOfVOStat;
    delete idleBWStat;
    delete collisionBWStat;
    delete successBWStat;
    delete dataTpBWStat;

    delete avgTxDataBWBE;
    delete avgTxDataBWVI;
    delete avgTxDataBWVO;

    delete avgDataBWBE;
    delete avgDataBWVI;
    delete avgDataBWVO;

    delete probOfTxInSlotBEVec;
    delete probOfTxInSlotVIVec;
    delete probOfTxInSlotVOVec;

    delete noOfVIVec;
    delete noOfVOVec;

    delete avgAchievableThroughputBE;
    delete avgAchievableThroughputVI;
    delete avgAchievableThroughputVO;

    delete predUsedBWVIStat;
    delete predUsedBWVIStat;
    delete predCollBWVIStat;
    delete predCollBWVOStat;

    delete avgCollDurationStat;
    delete usedBWStat;

    delete frameSizeTxVec;
    delete frameSizeTxStat;
    delete avgFrameSizeTxStat;
    delete frameSizeRxVec;
    delete frameSizeRxStat;
    delete avgFrameSizeRxStat;

    while (!mStats.empty())
    {
        delete(mStats.front())->probOfTxInSlotVec;
        delete(mStats.front())->achievableThroughput;
        delete mStats.front();
        mStats.pop_front();
    }
    if (ifaces != NULL)
    {
        delete ifaces;
    }
}

void WirelessAccessPoint::initialize(int stage)
{
    if (stage == 0)
    {
        WATCH(channel);
        LinkLayerModule::initialize();
        cModule *mod = OPP_Global::findModuleByName(this, "worldProcessor");
        assert(mod);
        wproc = static_cast<WorldProcessor *>(mod);
        iface_type = PR_WETHERNET;
        procdelay = par("procdelay").longValue();
        iface_name = "wlanap";

        // read values from Etc/default.ini or omnetpp.ini
        channel = OPP_Global::findNetNodeModule(this)->par("chann").longValue();
        beaconPeriod = OPP_Global::findNetNodeModule(this)->par("beaconPeriod").doubleValue();
        authWaitEntryTimeout = OPP_Global::findNetNodeModule(this)->par("authWaitEntryTimeout").doubleValue();
        authEntryTimeout = OPP_Global::findNetNodeModule(this)->par("authEntryTimeout").doubleValue();
        assEntryTimeout = OPP_Global::findNetNodeModule(this)->par("assEntryTimeout").doubleValue();
        consecFailedTransLimit = OPP_Global::findNetNodeModule(this)->par("consecFailedTransLimit").longValue();

        apMode = true;
        address.set(MAC_ADDRESS_UNSPECIFIED_STRUCT);

        ifaces = new ExpiryEntryList<WirelessEtherInterface>(this, TMR_REMOVEENTRY);

        usedBW.sampleTotal = 0;
        usedBW.sampleTime = 1;
        usedBW.average = 0;

        RxDataBWBE = 0;
        RxDataBWVI = 0;
        RxDataBWVO = 0;
        TxDataBWBE = 0;
        TxDataBWVI = 0;
        TxDataBWVO = 0;
        currentCollDurationBE = 0;
        currentDurationBE = 0;
        currentDurationVI = 0;
        currentDurationVO = 0;
        collDurationBE = 0;
        durationBE = 0;
        durationVI = 0;
        durationVO = 0;
        durationDataBE = 0;
        durationDataVI = 0;
        durationDataVO = 0;

        estAvailBW = BASE_SPEED / (1000000);
        estAvailBWVec = new cOutVector("estAvailBW");

        avgCollDurationVec = new cOutVector("avgCollDurationVec");
        achievableThroughputBEVec = new cOutVector("achievableThroughputBEVec");
        achievableThroughputVIVec = new cOutVector("achievableThroughputVIVec");
        achievableThroughputVOVec = new cOutVector("achievableThroughputVOVec");
        achievableTpTotalVec = new cOutVector("achievableTpTotalVec");
        collisionVec = new cOutVector("collisionVec");
        idleVec = new cOutVector("idleVec");

        probOfTxInSlotBEVec = new cOutVector("probOfTxInSlotBEVec");
        probOfTxInSlotVIVec = new cOutVector("probOfTxInSlotVIVec");
        probOfTxInSlotVOVec = new cOutVector("probOfTxInSlotVOVec");

        noOfVIVec = new cOutVector("noOfVIVec");
        noOfVOVec = new cOutVector("noOfVOVec");

        predUsedBWVIStat = new cStdDev("predUsedBWVIStat");
        predUsedBWVOStat = new cStdDev("predUsedBWVOStat");
        predCollBWVIStat = new cStdDev("predCollBWVIStat");
        predCollBWVOStat = new cStdDev("predCollBWVOStat");

        noOfVIStat = new cStdDev("noOfVIStat");
        noOfVOStat = new cStdDev("noOfVOStat");
        idleBWStat = new cStdDev("idleBWStat");
        collisionBWStat = new cStdDev("collisionBWStat");
        successBWStat = new cStdDev("successBWStat");
        dataTpBWStat = new cStdDev("dataTpBWStat");

        avgTxDataBWBE = new cStdDev("avgTxDataBWBE");
        avgTxDataBWVI = new cStdDev("avgTxDataBWVI");
        avgTxDataBWVO = new cStdDev("avgTxDataBWVO");

        avgDataBWBE = new cStdDev("avgDataBWBE");
        avgDataBWVI = new cStdDev("avgDataBWVI");
        avgDataBWVO = new cStdDev("avgDataBWVO");

        avgAchievableThroughputBE = new cStdDev("avgAchievableThroughputBE");
        avgAchievableThroughputVI = new cStdDev("avgAchievableThroughputVI");
        avgAchievableThroughputVO = new cStdDev("avgAchievableThroughputVO");

        avgCollDurationStat = new cStdDev("avgCollDurationStat");
        usedBWStat = new cStdDev("usedBWStat");

        frameSizeTxVec = new cOutVector("frameSizeTx");
        frameSizeTxStat = new cStdDev("frameSizeTxStat");
        avgFrameSizeTxStat = new cStdDev("avgFrameSizeTxStat");
        frameSizeRxVec = new cOutVector("frameSizeRx");
        frameSizeRxStat = new cStdDev("frameSizeRxStat");
        avgFrameSizeRxStat = new cStdDev("avgFrameSizeRxStat");

        _currentReceiveMode = WEAPReceiveMode::instance();
    }
    else if (stage == 1)
    {
        // needed for AP bridge
        cMessage *protocolNotifier = new cMessage("PROTOCOL_NOTIFIER");
        protocolNotifier->setKind(MK_PACKET);
        send(protocolNotifier, inputQueueOutGate());

        double randomStart = uniform(0, 1);

        // On power up, access point will start sending beacons.
        scheduleAt(simTime() + beaconPeriod + randomStart, powerUpBeaconNotifier);

        wEV  << fullPath() << "\n"
             << " ====================== \n"
             << " MAC ADDR: " << address.stringValue() << "\n"
             << " SSID: " << ssid.c_str() << "\n"
             << " CHANNEL " << channel << " \n"
             << " PATH LOSS EXP: " << pLExp << "\n"
             << " STD DEV: " << pLStdDev << "dB\n"
             << " TXPOWER: " << txpower << "mW\n"
             << " THRESHOLDPOWER: " << threshpower << "dBm\n"
             << " MAXRETRY: " << maxRetry << "\n"
             << " BEACON PERIOD: " << beaconPeriod << "\n"
             << " AUTH WAIT ENTRY TIMEOUT: " << authWaitEntryTimeout << "\n"
             << " AUTH ENTRY TIMEOUT: " << authEntryTimeout << "\n"
             << " ASS ENTRY TIMEOUT: " << assEntryTimeout << "\n"
             << " FAST ACTIVE SCAN: " << fastActScan << "\n"
             << " CROSSTALK: " << crossTalk << "\n";
    }
    baseInit(stage);
}

void WirelessAccessPoint::handleMessage(cMessage *msg)
{
    if ((MAC_address) address == MAC_ADDRESS_UNSPECIFIED_STRUCT)
    {
        if (std::string(msg->name()) == "WE_AP_NOTIFY_MAC")     // XXX what's this?? -AV
        {
            address.set(static_cast<cPar *>(msg->parList().get(0))->stringValue());
            par("address") = address;

            wEV  << fullPath() << "\n"
                 << " ====================== \n"
                 << " MAC ADDR: " << address.stringValue() << "\n"
                 << " SSID: " << ssid.c_str() << "\n"
                 << " CHANNEL " << channel << " \n"
                 << " PATH LOSS EXP: " << pLExp << "\n"
                 << " STD DEV: " << pLStdDev << "dB\n"
                 << " TXPOWER: " << txpower << "mW\n"
                 << " THRESHOLDPOWER: " << threshpower << "dBm\n"
                 << " MAXRETRY: " << maxRetry << "\n"
                 << " BEACON PERIOD: " << beaconPeriod << "\n"
                 << " AUTH WAIT ENTRY TIMEOUT: " << authWaitEntryTimeout << "\n"
                 << " AUTH ENTRY TIMEOUT: " << authEntryTimeout << "\n"
                 << " ASS ENTRY TIMEOUT: " << assEntryTimeout << "\n"
                 << " FAST ACTIVE SCAN: " << fastActScan << "\n"
                 << " CROSSTALK: " << crossTalk << "\n";
        }
        delete msg;
        return;
    }

    WirelessEtherModule::handleMessage(msg);
}

void WirelessAccessPoint::finish(void)
{
    recordScalar("avgRxDataBWStat", avgRxDataBWStat->mean());
    recordScalar("avgTxDataBWStat", avgTxDataBWStat->mean());
    recordScalar("avgNoOfFailedTxStat", avgNoOfFailedTxStat->mean());
    recordScalar("avgTxAccessTimeStat", avgTxAccessTimeStat->mean());
    recordScalar("avgBackoffSlots", avgBackoffSlotsStat->mean());
    recordScalar("avgCW", avgCWStat->mean());
    recordScalar("avgOutBuffSizeBEStat", avgOutBuffSizeBEStat->mean());
    recordScalar("avgOutBuffSizeVIStat", avgOutBuffSizeVIStat->mean());
    recordScalar("avgOutBuffSizeVOStat", avgOutBuffSizeVOStat->mean());
    // recordScalar("avgAchievableThroughputBE", avgAchievableThroughputBE->mean());
    // recordScalar("avgAchievableThroughputVI", avgAchievableThroughputVI->mean());
    // recordScalar("avgAchievableThroughputVO", avgAchievableThroughputVO->mean());
    recordScalar("avgTxDataBWBE", avgTxDataBWBE->mean());
    recordScalar("avgTxDataBWVI", avgTxDataBWVI->mean());
    recordScalar("avgTxDataBWVO", avgTxDataBWVO->mean());
    recordScalar("avgDataBWBE", avgDataBWBE->mean());
    recordScalar("avgDataBWVI", avgDataBWVI->mean());
    recordScalar("avgDataBWVO", avgDataBWVO->mean());
    recordScalar("predUsedBWVI", predUsedBWVIStat->mean());
    recordScalar("predUsedBWVO", predUsedBWVOStat->mean());
    recordScalar("predCollBWVI", predCollBWVIStat->mean());
    recordScalar("predCollBWVO", predCollBWVOStat->mean());

    recordScalar("noOfVI", noOfVIStat->mean());
    recordScalar("noOfVO", noOfVOStat->mean());
    recordScalar("idleBW", idleBWStat->mean());
    recordScalar("collisionBW", collisionBWStat->mean());
    recordScalar("successBW", successBWStat->mean());
    recordScalar("dataTpBW", dataTpBWStat->mean());

    /*std::list<MobileStats*>::iterator msit;
       for(msit=mStats.begin(); msit!=mStats.end(); msit++)
       {
       recordScalar(std::string("avgAchievableT."+ (*msit)->name).c_str(), (*msit)->avgAchievableThroughput->mean());
       } */
}

void WirelessAccessPoint::sendBeacon(void)
{
    assert(apMode);
    assert(channel <= MAX_CHANNEL && channel >= -1);

    wEV << currentTime() << " " << fullPath() << " beacon channel: " << channel << "\n";

    WirelessEtherBasicFrame *beacon = createFrame(FT_MANAGEMENT, ST_BEACON, address, AC_VO,
                                                  MACAddress6(WE_BROADCAST_ADDRESS));
    FrameBody *beaconFrameBody = createFrameBody(beacon);
    beacon->encapsulate(beaconFrameBody);
    outputQueue->insertFrame(beacon, simTime());

    // schedule the next beacon message
    double nextSchedTime = simTime() + beaconPeriod;

    assert(!powerUpBeaconNotifier->isScheduled());
    reschedule(powerUpBeaconNotifier, nextSchedTime);

    if (_currentState == WirelessEtherStateIdle::instance())
        static_cast<WirelessEtherStateIdle *>(_currentState)->chkOutputBuffer(this);
}

void WirelessAccessPoint::receiveData(std::auto_ptr<cMessage> msg)
{
    // the frame should have already been created in the bridge module
    WESignalData *frame = dynamic_cast<WESignalData *>(msg.get()->decapsulate());
    assert(frame);

    // renew timeout value for destination, since it is
    // receiving data
    WirelessEtherDataFrame *dataFrame = check_and_cast<WirelessEtherDataFrame *>(frame->decapsulate());
    if (dataFrame->getAddress1() != MACAddress6(WE_BROADCAST_ADDRESS))
    {
        WirelessEtherInterface dest = findIfaceByMAC(dataFrame->getAddress1());
        assert(dest != UNSPECIFIED_WIRELESS_ETH_IFACE);
        dest.expire += assEntryTimeout;
        ifaces->addEntry(dest);
    }

    dataFrame->getSequenceControl().fragmentNumber = 0;
    dataFrame->getSequenceControl().sequenceNumber = sequenceNumber;
    incrementSequenceNumber();

    outputQueue->insertFrame(dataFrame, simTime());
    delete frame;

    // when received something from bridge into outputBuffer, need to send it
    // on Wireless side
    if (_currentState == WirelessEtherStateIdle::instance())
        static_cast<WirelessEtherStateIdle *>(_currentState)->chkOutputBuffer(this);
}

FrameBody *WirelessAccessPoint::createFrameBody(WirelessEtherBasicFrame *f)
{
    FrameBody *frameBody;
    CapabilityInformation capabilityInfo;
    HandoverParameters handoverParams;
    unsigned int i;

    switch (f->getFrameControl().subtype)
    {
    case ST_BEACON:
        {
            frameBody = new BeaconFrameBody;

            static_cast<BeaconFrameBody *>(frameBody)->setTimestamp(simTime());

            // need to find out what is a TU in standard
            static_cast<BeaconFrameBody *>(frameBody)->setBeaconInterval((int) (1000 * beaconPeriod));

            // Additional parameters for HO decision (not part of standard)
            /*handoverParams.avgBackoffTime = totalBackoffTime.average;
               handoverParams.avgWaitTime = totalWaitTime.average;
               handoverParams.avgErrorRate = errorPercentage;
               handoverParams.estAvailBW = estAvailBW;
             */

            capabilityInfo.ESS = !adhocMode;
            capabilityInfo.IBSS = adhocMode;
            capabilityInfo.CFPollable = false;  // CF not implemented
            capabilityInfo.CFPollRequest = false;
            capabilityInfo.privacy = false;     // wep not implemented

            static_cast<BeaconFrameBody *>(frameBody)->setHandoverParameters(handoverParams);
            static_cast<BeaconFrameBody *>(frameBody)->setCapabilityInformation(capabilityInfo);
            static_cast<BeaconFrameBody *>(frameBody)->setSSID(ssid.c_str());        // need size
            static_cast<BeaconFrameBody *>(frameBody)->setSupportedRatesArraySize(rates.size());

            // variable size
            for (i = 0; i < rates.size(); i++)
                static_cast<BeaconFrameBody *>(frameBody)->setSupportedRates(i, rates[i]);

            static_cast<BeaconFrameBody *>(frameBody)->setDSChannel(channel);

            frameBody->setLength(FL_TIMESTAMP + FL_BEACONINT + FL_CAPINFO +
                                 FL_IEHEADER + ssid.length() + FL_IEHEADER + rates.size() + FL_DSCHANNEL);
        }
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
    case ST_ASSOCIATIONRESPONSE:
        {
            frameBody = new AssociationResponseFrameBody;

            // adhoc mode not supported, should add indication in module
            capabilityInfo.ESS = !adhocMode;
            capabilityInfo.IBSS = adhocMode;
            capabilityInfo.CFPollable = false;  // CF not implemented
            capabilityInfo.CFPollRequest = false;
            capabilityInfo.privacy = false;     // wep not implemented

            static_cast<AssociationResponseFrameBody *>(frameBody)->
                setCapabilityInformation(capabilityInfo);

            WirelessEtherInterface wie = findIfaceByMAC(f->getAddress1());
            assert(wie != UNSPECIFIED_WIRELESS_ETH_IFACE);

            static_cast<AssociationResponseFrameBody *>(frameBody)->setStatusCode(wie.statusCode);

            static_cast<AssociationResponseFrameBody *>(frameBody)->
                setSupportedRatesArraySize(rates.size());

            for (i = 0; i < rates.size(); i++)
                static_cast<AssociationResponseFrameBody *>(frameBody)->setSupportedRates(i, rates[i]);

            frameBody->setLength(FL_CAPINFO + FL_STATUSCODE + FL_IEHEADER + rates.size());
        }
        break;

    case ST_REASSOCIATIONRESPONSE:
        {
            frameBody = new ReAssociationResponseFrameBody;

            capabilityInfo.ESS = !adhocMode;
            capabilityInfo.IBSS = adhocMode;
            capabilityInfo.CFPollable = false;  // CF not implemented
            capabilityInfo.CFPollRequest = false;
            capabilityInfo.privacy = false;     // wep not implemented

            static_cast<ReAssociationResponseFrameBody *>(frameBody)->
                setCapabilityInformation(capabilityInfo);

            WirelessEtherInterface wie = findIfaceByMAC(f->getAddress1());
            assert(wie != UNSPECIFIED_WIRELESS_ETH_IFACE);

            static_cast<ReAssociationResponseFrameBody *>(frameBody)->setStatusCode(wie.statusCode);

            static_cast<ReAssociationResponseFrameBody *>(frameBody)->
                setSupportedRatesArraySize(rates.size());

            for (i = 0; i < rates.size(); i++)
                static_cast<ReAssociationResponseFrameBody *>(frameBody)->setSupportedRates(i, rates[i]);

            frameBody->setLength(FL_CAPINFO + FL_STATUSCODE + FL_IEHEADER + rates.size());
        }
        break;

    case ST_DISASSOCIATION:
        {
            frameBody = new DisAssociationFrameBody;

            WirelessEtherInterface wie = findIfaceByMAC(f->getAddress1());
            assert(wie != UNSPECIFIED_WIRELESS_ETH_IFACE);

            static_cast<DisAssociationFrameBody *>(frameBody)->setReasonCode(wie.reasonCode);

            // remove the wireless Ethernet interface off the list since
            // it is disassociated with the AP
            ifaces->removeEntry(wie);

            frameBody->setLength(FL_REASONCODE);
        }
        break;

    case ST_PROBERESPONSE:
        {
            frameBody = new ProbeResponseFrameBody;

            static_cast<ProbeResponseFrameBody *>(frameBody)->setTimestamp((double) simulation.simTime());

            // need to find out what is a TU in standard
            static_cast<ProbeResponseFrameBody *>(frameBody)->
                setBeaconInterval((int) (1000 * beaconPeriod));

            // Additional parameters for HO decision (not part of standard)
            /*handoverParams.avgBackoffTime = totalBackoffTime.average;
               handoverParams.avgWaitTime = totalWaitTime.average;
               handoverParams.avgErrorRate = errorPercentage;
               handoverParams.estAvailBW = estAvailBW;
             */
            capabilityInfo.ESS = !adhocMode;
            capabilityInfo.IBSS = adhocMode;
            capabilityInfo.CFPollable = false;  // CF not implemented
            capabilityInfo.CFPollRequest = false;
            capabilityInfo.privacy = false;     // wep not implemented

            static_cast<BeaconFrameBody *>(frameBody)->setHandoverParameters(handoverParams);
            static_cast<ProbeResponseFrameBody *>(frameBody)->setCapabilityInformation(capabilityInfo);
            static_cast<ProbeResponseFrameBody *>(frameBody)->setSSID(ssid.c_str());

            static_cast<ProbeResponseFrameBody *>(frameBody)->setSupportedRatesArraySize(rates.size());

            for (i = 0; i < rates.size(); i++)
                static_cast<ProbeResponseFrameBody *>(frameBody)->setSupportedRates(i, rates[i]);

            static_cast<ProbeResponseFrameBody *>(frameBody)->setDSChannel(channel);

            frameBody->setLength(FL_TIMESTAMP + FL_BEACONINT + FL_CAPINFO +
                                 FL_IEHEADER + ssid.length() + FL_IEHEADER + rates.size() + FL_DSCHANNEL);
        }
        break;

    case ST_DEAUTHENTICATION:
        {
            frameBody = new DeAuthenticationFrameBody;

            // Removed since deauthentication is sent when MS is not an entry
            // in the table. In which case will cause an assertion. So for now
            // the reason code is RC_DEAUTH_MS_LEAVING
            //
            // WirelessEtherInterface wie = findIfaceByMAC(f->getAddress1());
            // assert(wie != UNSPECIFIED_WIRELESS_ETH_IFACE);

            static_cast<DeAuthenticationFrameBody *>(frameBody)->setReasonCode(RC_DEAUTH_MS_LEAVING);
            // setReasonCode(wie.reasonCode);

            frameBody->setLength(FL_REASONCODE);
        }
        break;

    default:
        assert(false);
        break;
    }
    return frameBody;
}

void WirelessAccessPoint::addIface(MACAddress6 mac, ReceiveMode receiveMode, int sequenceNo)
{
    WirelessEtherInterface iface;

    iface.address = mac;
    iface.receiveMode = receiveMode;
    iface.expire = simTime();
    iface.consecFailedTrans = 0;
    iface.currentSequence = sequenceNo;

    // Choosing entry's expiry time depending on
    // its type
    if (receiveMode == RM_AUTHRSP_ACKWAIT)
        iface.expire += authWaitEntryTimeout;
    else if (receiveMode == RM_ASSOCIATION)
        iface.expire += authEntryTimeout;
    else if (receiveMode == RM_ASSRSP_ACKWAIT)
        iface.expire += authEntryTimeout;
    // iface.expire += iface.expire - elapsed;
    else if (receiveMode == RM_DATA)
        iface.expire += assEntryTimeout;

    wEV  << fullPath() << " " << simTime()
         << " ADDR: " << iface.address.stringValue()
         << " RM: " << iface.receiveMode
         << " EXP: " << iface.expire << "\n";

    ifaces->addEntry(iface);
}

void WirelessAccessPoint::setIfaceStatus(MACAddress6 mac, StatusCode)
{
    WirelessEtherInterface wie = findIfaceByMAC(mac);
    assert(wie != UNSPECIFIED_WIRELESS_ETH_IFACE);
}

WirelessEtherInterface WirelessAccessPoint::findIfaceByMAC(MACAddress6 mac)
{
    WirelessEtherInterface target;
    target.address = mac;
    if (!ifaces->findEntry(target))
        target = UNSPECIFIED_WIRELESS_ETH_IFACE;

    return target;
}

// Update the number of consecutive failed data frame tranmissions, given the failed frame
void WirelessAccessPoint::updateConsecutiveFailedCount(WirelessEtherBasicFrame *outputFrame)
{
    assert(outputFrame);
    FrameControl frameControl = outputFrame->getFrameControl();

    // Only consider data frames
    if (frameControl.subtype == ST_DATA)
    {
        WirelessEtherInterface dest = findIfaceByMAC(outputFrame->getAddress1());

        // Check that destination is in AP list
        if (dest != UNSPECIFIED_WIRELESS_ETH_IFACE)
        {
            // Remove entry if consecutive transmission limit is reached
            if (dest.consecFailedTrans >= consecFailedTransLimit)
            {
                wEV << fullPath() << " " << simTime() << " Entry removed: " << outputFrame->getAddress1() << "\n";
                ifaces->removeEntry(dest);
            }
            else
            {
                dest.consecFailedTrans++;
                ifaces->addEntry(dest);
            }
        }
        else
            wEV << fullPath() << " " << simTime() << " Entry already removed\n";

    }
}

// SWOON HACK: To find achievable throughput
void WirelessAccessPoint::updateMStats(std::string fullPathName, int appType, double prob, double frameSize)
{
    std::list < MobileStats * >::iterator msit;
    MobileStats *ms = NULL;

    // Find existing entry
    for (msit = mStats.begin(); msit != mStats.end(); msit++)
    {
        if ((*msit)->name == fullPathName)
        {
            ms = (*msit);
            break;
        }
    }
    // Create a new entry, if an existing found
    if (ms == NULL)
    {
        ms = new MobileStats;
        ms->name = fullPathName;
        ms->achievableThroughput = new cOutVector(std::string("achievableT." + fullPathName).c_str());
        ms->probOfTxInSlotVec = new cOutVector(std::string("probOfTxInSlot." + fullPathName).c_str());
        ms->avgAchievableThroughput = new cStdDev();
        mStats.push_back(ms);
    }
    // Update entry values
    ms->lastUpdateTime = simTime();
    ms->appType = appType;
    ms->probTxInSlot = prob;
    ms->avgFrameSize = frameSize;
}

void WirelessAccessPoint::estimateThroughput(double &aTAPBE, double &aTAPVI, double &aTAPVO)
{
    std::list < MobileStats * >::iterator msit;

    // Final throughput estimation variables
    double probTxBE = outputQueue->getProbTxInSlot(AC_BE, simTime(), this);
    double probTxVI = outputQueue->getProbTxInSlot(AC_VI, simTime(), this);
    double probTxVO = outputQueue->getProbTxInSlot(AC_VO, simTime(), this);
    double probCollision, probSlotIdle = 1, successDuration = 0, successBits = 0;
    double probSuccess = 0, probSuccessBE, probSuccessVI, probSuccessVO;
    double aTMS;

    // Intermediate variables for estimation
    double probOfTx, probOneOrMore, duration, num, denom;

    // Prediction variables
    /* double probOfTxVI=1, probOfTxVO=1;
       double probSlotIdleVI=1, probSlotIdleVO=1;
       double probOneOrMoreVI, probOneOrMoreVO;
       double probCollisionVI, probCollisionVO;
       double successDurationVI=0, successDurationVO=0;
       double totalProbSuccessVI=0, totalProbSuccessVO=0;
       double probOfSuccessVI, probOfSuccessVO;
       double collDiffVI, collDiffVO;
       double noOfVI, noOfVO;
       double denomVI, denomVO; */
    double oneVI, oneVO;
    double frameSizeVI = 6000, frameSizeVO = 1580;
    double frameRateVI = 27, frameRateVO = 43;

    // Can substitute BASE_SPEED for different rates
    double typDurVI = (frameSizeVI / BASE_SPEED) + successOhDurationVI;
    double typDurVO = (frameSizeVO / BASE_SPEED) + successOhDurationVO;
    double typBWVI = ((frameSizeVI / BASE_SPEED) + successOhDurationVI) * frameRateVI * BASE_SPEED / 1000000;
    double typBWVO = ((frameSizeVO / BASE_SPEED) + successOhDurationVO) * frameRateVO * BASE_SPEED / 1000000;

    // Set probTx values for extra VI and VO
    /*for(msit=mStats.begin(); msit!=mStats.end(); msit++)
       {
       if( (simTime()-(*msit)->lastUpdateTime) <= 2)
       {
       if((*msit)->appType == AC_VI)
       {
       std::cout<<probOfTxVI<<" ";
       probOfTxVI = (*msit)->probTxInSlot;
       }
       else if((*msit)->appType == AC_VO)
       {
       std::cout<<probOfTxVO<<" ";
       probOfTxVO = (*msit)->probTxInSlot;
       }

       }
       }
       // Use probTx for VO, if VI is not found
       if(probOfTxVI == 1)
       probOfTxVI = probOfTxVO;
       // and vice versa
       if(probOfTxVO == 1)
       probOfTxVO = probOfTxVI; */

    // Calculate the probability of a slot being idle
    // Start by including the prob for associated MSs
    for (msit = mStats.begin(); msit != mStats.end(); msit++)
    {
        if ((simTime() - (*msit)->lastUpdateTime) <= 2)
            probOfTx = (*msit)->probTxInSlot;
        else
            probOfTx = 0;
        probSlotIdle *= 1 - probOfTx;
        if (statsVec)
            (*msit)->probOfTxInSlotVec->record(probOfTx);
    }
    // Finally include AP queues
    probSlotIdle *= (1 - probTxBE);
    probSlotIdle *= (1 - probTxVI);
    probSlotIdle *= (1 - probTxVO);

    // Calculate probSlotIdle for extra VO and VI predictions
    /*probSlotIdleVI = probSlotIdle*(1-probOfTxVI);
       probSlotIdleVO = probSlotIdle*(1-probOfTxVO); */

    /*if(statsVec)
       {
       probOfTxInSlotBEVec->record(probTxBE);
       probOfTxInSlotVIVec->record(probTxVI);
       probOfTxInSlotVOVec->record(probTxVO);
       } */

    // Calculate probability of having one or more tx in a slot
    probOneOrMore = 1 - probSlotIdle;

    // Calculate probOneOrMore for extra VO and VI predictions
    /*probOneOrMoreVI = 1-probSlotIdleVI;
       probOneOrMoreVO = 1-probSlotIdleVO; */


    // Calculate the probability of successfulTx for each queue and finding the successful tx duration.
    // Start with MS queues
    for (msit = mStats.begin(); msit != mStats.end(); msit++)
    {
        if ((simTime() - (*msit)->lastUpdateTime) <= 2)
            probOfTx = (*msit)->probTxInSlot;
        else
            probOfTx = 0;

        (*msit)->probSuccessfulTx = probOfTx * probSlotIdle / (1 - probOfTx);
        // (*msit)->probSuccessfulTxVI = probOfTx*probSlotIdleVI/(1-probOfTx);
        // (*msit)->probSuccessfulTxVO = probOfTx*probSlotIdleVO/(1-probOfTx);

        probSuccess += (*msit)->probSuccessfulTx;
        // totalProbSuccessVI += (*msit)->probSuccessfulTxVI;
        // totalProbSuccessVO += (*msit)->probSuccessfulTxVO;

        if ((*msit)->appType == AC_VO)
            duration = (*msit)->avgFrameSize / BASE_SPEED + successOhDurationVO;
        else if ((*msit)->appType == AC_VI)
            duration = (*msit)->avgFrameSize / BASE_SPEED + successOhDurationVI;
        else
            duration = (*msit)->avgFrameSize / BASE_SPEED + successOhDurationBE;

        successDuration += (*msit)->probSuccessfulTx * duration;
        // successDurationVI += (*msit)->probSuccessfulTxVI*duration;
        // successDurationVO += (*msit)->probSuccessfulTxVO*duration;

        successBits += (*msit)->probSuccessfulTx * (*msit)->avgFrameSize;
    }
    // Finally include AP queues
    probSuccessBE = probTxBE * probSlotIdle / (1 - probTxBE);
    probSuccess += probSuccessBE;
    successDuration +=
        probSuccessBE * ((double) outputQueue->getAvgFrameSize(AC_BE) / BASE_SPEED + successOhDurationBE);
    successBits += probSuccessBE * ((double) outputQueue->getAvgFrameSize(AC_BE));

    probSuccessVI = probTxVI * probSlotIdle / (1 - probTxVI);
    probSuccess += probSuccessVI;
    successDuration +=
        probSuccessVI * ((double) outputQueue->getAvgFrameSize(AC_VI) / BASE_SPEED + successOhDurationVI);
    successBits += probSuccessVI * ((double) outputQueue->getAvgFrameSize(AC_VI));

    probSuccessVO = probTxVO * probSlotIdle / (1 - probTxVO);
    probSuccess += probSuccessVO;
    successDuration +=
        probSuccessVO * ((double) outputQueue->getAvgFrameSize(AC_VO) / BASE_SPEED + successOhDurationVO);
    successBits += probSuccessVO * ((double) outputQueue->getAvgFrameSize(AC_VO));

    // Calculate probSuccess and successDuration for extra VO and VI predictions
    /*double probSuccessBEVI = probTxBE*probSlotIdleVI/(1-probTxBE);
       totalProbSuccessVI += probSuccessBEVI;
       successDurationVI += probSuccessBEVI*((double)outputQueue->getAvgFrameSize(AC_BE)/BASE_SPEED + successOhDurationBE);
       double probSuccessBEVO = probTxBE*probSlotIdleVO/(1-probTxBE);
       totalProbSuccessVO += probSuccessBEVO;
       successDurationVO += probSuccessBEVO*((double)outputQueue->getAvgFrameSize(AC_BE)/BASE_SPEED + successOhDurationBE);

       double probSuccessVIVI = probTxVI*probSlotIdleVI/(1-probTxVI);
       totalProbSuccessVI += probSuccessVIVI;
       successDurationVI += probSuccessVIVI*((double)outputQueue->getAvgFrameSize(AC_VI)/BASE_SPEED + successOhDurationVI);
       double probSuccessVIVO = probTxVI*probSlotIdleVO/(1-probTxVI);
       totalProbSuccessVO += probSuccessVIVO;
       successDurationVO += probSuccessVIVO*((double)outputQueue->getAvgFrameSize(AC_VI)/BASE_SPEED + successOhDurationVI);

       double probSuccessVOVI = probTxVO*probSlotIdleVI/(1-probTxVO);
       totalProbSuccessVI += probSuccessVOVI;
       successDurationVI += probSuccessVOVI*((double)outputQueue->getAvgFrameSize(AC_VO)/BASE_SPEED + successOhDurationVO);
       double probSuccessVOVO = probTxVO*probSlotIdleVO/(1-probTxVO);
       totalProbSuccessVO += probSuccessVOVO;
       successDurationVO += probSuccessVOVO*((double)outputQueue->getAvgFrameSize(AC_VO)/BASE_SPEED + successOhDurationVO);

       probOfSuccessVI = probOfTxVI*probSlotIdleVI/(1-probOfTxVI);
       totalProbSuccessVI += probOfSuccessVI;
       successDurationVI += probOfSuccessVI*typDurVI;
       probOfSuccessVO = probOfTxVO*probSlotIdleVO/(1-probOfTxVO);
       totalProbSuccessVO += probOfSuccessVO;
       successDurationVO += probOfSuccessVO*typDurVO; */

    // Calculate probability of collision in a slot
    probCollision = (probOneOrMore - probSuccess);

    // Calculate probCollision for extra VO and VI predictions
    // probCollisionVI = probOneOrMoreVI - totalProbSuccessVI;
    // probCollisionVO = probOneOrMoreVO - totalProbSuccessVO;

    denom = probCollision * avgCollDurationStat->mean() + probSlotIdle * SLOTTIME + successDuration;
    // denomVI = probCollisionVI*avgCollDurationStat->mean() + probSlotIdleVI*SLOTTIME + successDurationVI;
    // denomVO = probCollisionVO*avgCollDurationStat->mean() + probSlotIdleVO*SLOTTIME + successDurationVO;

    // If there is no probOfTx measurement for both application types, assume no collision overhead
    /*if( (probOfTxVI == 1)&&(probOfTxVO == 1) )
       collDiffVI = collDiffVO = 0;
       else
       {
       collDiffVI = probCollisionVI*avgCollDurationStat->mean() - probCollision*avgCollDurationStat->mean();
       collDiffVO = probCollisionVO*avgCollDurationStat->mean() - probCollision*avgCollDurationStat->mean();
       } */

    if (denom > 0)
    {
        double occupiedBW =
            ((successDuration + probCollision * avgCollDurationStat->mean()) / denom) * BASE_SPEED / 1000000;

        // Idle bandwidth with saturation offset
        double idleBW = ((probSlotIdle * SLOTTIME / denom) * BASE_SPEED / 1000000) - 0.15;
        if (idleBW < 0)
            idleBW = 0;

        double durationBW =
            (currentDurationBE + currentDurationVI + currentDurationVO) * BASE_SPEED / 1000000;
        double collisionBW = occupiedBW - durationBW;
        if (collisionBW < 0)
            collisionBW = 0;
        double percentageCollision = collisionBW / durationBW;
        double tp = (successBits / denom) / 1000000;

        if (duration < 0.001)
            percentageCollision = 0;

        double predUsedBWVI = 2 * typBWVI;
        double predCollBWVI = 2 * typBWVI * percentageCollision;
        double predUsedBWVO = 2 * typBWVO;
        double predCollBWVO = 2 * typBWVO * percentageCollision;
        oneVI = predUsedBWVI + predCollBWVI;
        oneVO = predUsedBWVO + predCollBWVO;

        newNoOfVI = (int)(((idleBW+(currentDurationBE+currentCollDurationBE)*BASE_SPEED/1000000)/oneVI)*0.6 + newNoOfVI*0.4);
        newNoOfVO = (int)(((idleBW+(currentDurationBE+currentCollDurationBE)*BASE_SPEED/1000000)/oneVO)*0.6 + newNoOfVO*0.4);
        // Bandwidth and overhead incured by adding one VI MS
        // oneVI = (collDiffVI/denom)*BASE_SPEED/1000000 + 2*typBWVI;
        // Bandwidth and overhead incured by adding one VO MS
        // oneVO = (collDiffVO/denom)*BASE_SPEED/1000000 + 2*typBWVO;
        // oneVI = ((probSlotIdle*SLOTTIME/denom)-(probSlotIdleVI*SLOTTIME/denomVI))*BASE_SPEED/1000000;
        // oneVO = ((probSlotIdle*SLOTTIME/denom)-(probSlotIdleVO*SLOTTIME/denomVO))*BASE_SPEED/1000000;

        /*if((probOfTxVO == 1)&&(probOfTxVI == 1))
           {
           oneVI = 0;
           oneVO = 0;
           if(currentTxDataBWBE == 0)
           {
           oneVI += 2*typBWVI;
           oneVO += 2*typBWVO;
           }
           } */

        // Find the estimated no. of each application that can be supported
        // average between new and old value for the estimate with alpha=0.5
        // newNoOfVI = (int)(((idleBW+currentTxDataBWBE)/oneVI)*0.6 + newNoOfVI*0.4);
        // newNoOfVO = (int)(((idleBW+currentTxDataBWBE)/oneVO)*0.6 + newNoOfVO*0.4);

        if ((statsVec) && (simTime() > 15))
        {
            noOfVIVec->record(newNoOfVI);
            noOfVOVec->record(newNoOfVO);

            achievableTpTotalVec->record(durationBW);
            collisionVec->record(collisionBW);
            idleVec->record(idleBW);
        }
        if (simTime() > samplingStartTime)
        {
            noOfVIStat->collect(newNoOfVI);
            noOfVOStat->collect(newNoOfVO);

            predUsedBWVIStat->collect(predUsedBWVI);
            predUsedBWVOStat->collect(predUsedBWVO);
            predCollBWVIStat->collect(predCollBWVI);
            predCollBWVOStat->collect(predCollBWVO);

            successBWStat->collect(durationBW);
            collisionBWStat->collect(collisionBW);
            idleBWStat->collect(idleBW);
            dataTpBWStat->collect(tp);
        }

        // Calculate achievable throughput for MS
        for (msit = mStats.begin(); msit != mStats.end(); msit++)
        {
            num = (*msit)->probSuccessfulTx * (*msit)->avgFrameSize;
            aTMS = num / denom;

            if (statsVec)
                (*msit)->achievableThroughput->record((1000000 * aTMS));
            (*msit)->avgAchievableThroughput->collect((1000000 * aTMS));
        }
        // Calculate achievable throughput for AP queues
        num = probSuccessBE * outputQueue->getAvgFrameSize(AC_BE);
        aTAPBE = num / (1000000 * denom);
        num = probSuccessVI * outputQueue->getAvgFrameSize(AC_VI);
        aTAPVI = num / (1000000 * denom);
        num = probSuccessVO * outputQueue->getAvgFrameSize(AC_VO);
        aTAPVO = num / (1000000 * denom);
    }
    else
        aTAPBE = aTAPVI = aTAPVO = 0;
}


void WirelessAccessPoint::updateStats(void)
{
    double aTAPBE, aTAPVI, aTAPVO;
    estimateThroughput(aTAPBE, aTAPVI, aTAPVO);

    if (statsVec)
    {
        achievableThroughputBEVec->record(aTAPBE);
        achievableThroughputVIVec->record(aTAPVI);
        achievableThroughputVOVec->record(aTAPVO);
    }

    if (simTime() > samplingStartTime)
    {
        avgAchievableThroughputBE->collect(aTAPBE);
        avgAchievableThroughputVI->collect(aTAPVI);
        avgAchievableThroughputVO->collect(aTAPVO);

        avgTxDataBWBE->collect(TxDataBWBE / statsUpdatePeriod);
        avgTxDataBWVI->collect(TxDataBWVI / statsUpdatePeriod);
        avgTxDataBWVO->collect(TxDataBWVO / statsUpdatePeriod);

        avgDataBWBE->collect(durationDataBE * BASE_SPEED / 1000000);
        avgDataBWVI->collect(durationDataVI * BASE_SPEED / 1000000);
        avgDataBWVO->collect(durationDataVO * BASE_SPEED / 1000000);
    }

    RxDataBWBE = 0;
    RxDataBWVI = 0;
    RxDataBWVO = 0;
    TxDataBWBE = 0;
    TxDataBWVI = 0;
    TxDataBWVO = 0;

    currentCollDurationBE = collDurationBE;
    currentDurationBE = durationBE;
    currentDurationVI = durationVI;
    currentDurationVO = durationVO;

    collDurationBE = 0;
    durationBE = 0;
    durationVI = 0;
    durationVO = 0;
    durationDataBE = 0;
    durationDataVI = 0;
    durationDataVO = 0;

    WirelessEtherModule::updateStats();
}
