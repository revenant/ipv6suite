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
#include "config.h"

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

Define_Module(WirelessAccessPoint);

const WirelessEtherInterface UNSPECIFIED_WIRELESS_ETH_IFACE =
{
  MACAddress6(MAC_ADDRESS_UNSPECIFIED_STRUCT),
  RM_UNSPECIFIED,
  0,
    0,
  RC_UNSPECIFIED,
  SC_UNSPECIFIED
};

bool operator==(const WirelessEtherInterface& lhs,
                const WirelessEtherInterface& rhs)
{
  return ( lhs.address == rhs.address);
}

bool operator!=(const WirelessEtherInterface& lhs,
                const WirelessEtherInterface& rhs)
{
  return !( lhs.address == rhs.address);
}

bool operator<(const WirelessEtherInterface& lhs,
                             const WirelessEtherInterface& rhs)
{
    return (lhs.expire < rhs.expire);
}

void WirelessAccessPoint::initialize(int stage)
{
  if ( stage == 0 )
  {
    LinkLayerModule::initialize();
    cModule* mod = OPP_Global::findModuleByName(this, "worldProcessor");
    assert(mod);
    wproc = static_cast<WorldProcessor*>(mod);
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
    estAvailBW = BASE_SPEED/(1024*1024);
    estAvailBWVec = new cOutVector("estAvailBW");

    usedBWStat = new cStdDev("usedBWStat");

    _currentReceiveMode = WEAPReceiveMode::instance();
  }
  else if (stage == 1 )
  {
    // needed for AP bridge
    cMessage* protocolNotifier = new cMessage("PROTOCOL_NOTIFIER");
    protocolNotifier->setKind(MK_PACKET);
    send(protocolNotifier, inputQueueOutGate());

    // On power up, access point will start sending beacons.
    cTTimerMessageCBA<void, void>* powerUpBeaconNotifier;

    powerUpBeaconNotifier  =
      new cTTimerMessageCBA<void, void>
      (TMR_BEACON, this, makeCallback(this, &WirelessAccessPoint::sendBeacon), "sendBeacon");

    addTmrMessage(powerUpBeaconNotifier);
    scheduleAt(simTime()+beaconPeriod, powerUpBeaconNotifier);
  }
  baseInit(stage);
}

void WirelessAccessPoint::handleMessage(cMessage* msg)
{
  if ((MAC_address)address == MAC_ADDRESS_UNSPECIFIED_STRUCT)
  {
    if ( std::string(msg->name()) == "WE_AP_NOTIFY_MAC" ) // XXX what's this?? -AV
    {
      address.set(static_cast<cPar*>(msg->parList().get(0))->stringValue());

      Dout(dc::wireless_ethernet|flush_cf, "MAC LAYER: (WIRELESS) "
           << fullPath() << "\n"
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
           << " CROSSTALK: " << crossTalk << "\n");


      idleNetworkInterface();
    }
    delete msg;
    return;
  }

  WirelessEtherModule::handleMessage(msg);
}

void WirelessAccessPoint::finish(void)
{
  Dout(dc::wireless_stats|flush_cf, OPP_Global::nodeName(this) << " usedBWStat = " << usedBWStat->mean());
  Dout(dc::wireless_stats|flush_cf, OPP_Global::nodeName(this) << " errorPercentageStat = " << errorPercentageStat->mean());
  Dout(dc::wireless_stats|flush_cf, OPP_Global::nodeName(this) << " backoffTimeStat = " << backoffTimeStat->mean());
  Dout(dc::wireless_stats|flush_cf, OPP_Global::nodeName(this) << " waitTimeStat = " << waitTimeStat->mean());
  Dout(dc::wireless_stats|flush_cf, OPP_Global::nodeName(this) << " noOfRetries = " << noOfRetries);
  Dout(dc::wireless_stats|flush_cf, OPP_Global::nodeName(this) << " noOfAttemptedTx = " << noOfAttemptedTx);
  if(ifaces != NULL)
  {
    delete ifaces;
  }
}

void WirelessAccessPoint::idleNetworkInterface(void)
{
  /* XXX notifying the output queue removed from here --AV */
}

void WirelessAccessPoint::sendBeacon(void)
{
  assert(apMode);
  assert(channel <= MAX_CHANNEL && channel >= -1);

  Dout(dc::wireless_ethernet|flush_cf, "MAC LAYER: (WIRELESS) " << std::fixed << std::showpoint << std::setprecision(12) << simTime() << " " << fullPath() << " beacon channel: " << channel);

  WirelessEtherBasicFrame* beacon =
        createFrame(FT_MANAGEMENT, ST_BEACON, address,
                    MACAddress6(WE_BROADCAST_ADDRESS));
  FrameBody* beaconFrameBody = createFrameBody(beacon);
  beacon->encapsulate(beaconFrameBody);
  WESignalData* beaconSignal = encapsulateIntoWESignalData(beacon);
  beaconSignal->setChannel(channel);
  outputBuffer.push_back(beaconSignal);

  // schedule the next beacon message
  double nextSchedTime = simTime() + beaconPeriod;

  cTimerMessage* tmr = getTmrMessage(TMR_BEACON);
  assert(tmr && !tmr->isScheduled());

  tmr->reschedule(nextSchedTime);

  if(_currentState == WirelessEtherStateIdle::instance())
      static_cast<WirelessEtherStateIdle*>(_currentState)->chkOutputBuffer(this);
}

void WirelessAccessPoint::receiveData(std::auto_ptr<cMessage> msg)
{
  // the frame should have already been created in the bridge module
  WESignalData* frame = dynamic_cast<WESignalData*>(msg.get()->decapsulate());
  assert(frame);

  outputBuffer.push_back(frame);

    // relying on internal outputBuffer, therefore need to get data from external
    // outputQueue into it quickly.
    idleNetworkInterface();

  // when received something from bridge into outputBuffer, need to send it
  // on Wireless side
  if ( _currentState == WirelessEtherStateIdle::instance())
    static_cast<WirelessEtherStateIdle*>(_currentState)->chkOutputBuffer(this);
}

FrameBody* WirelessAccessPoint::createFrameBody(WirelessEtherBasicFrame* f)
{
    FrameBody* frameBody;
    CapabilityInformation capabilityInfo;
    HandoverParameters handoverParams;
    unsigned int i;

    switch(f->getFrameControl().subtype)
    {
      case ST_BEACON:
      {
        frameBody = new BeaconFrameBody;

        static_cast<BeaconFrameBody*>(frameBody)->
          setTimestamp(simTime());

        // need to find out what is a TU in standard
        static_cast<BeaconFrameBody*>(frameBody)->
          setBeaconInterval((int)(1000*beaconPeriod));

        // Additional parameters for HO decision (not part of standard)
        handoverParams.avgBackoffTime = totalBackoffTime.average;
        handoverParams.avgWaitTime = totalWaitTime.average;
        handoverParams.avgErrorRate = errorPercentage;
        handoverParams.estAvailBW = estAvailBW;

        capabilityInfo.ESS = !adhocMode;
        capabilityInfo.IBSS = adhocMode;
        capabilityInfo.CFPollable = false; // CF not implemented
        capabilityInfo.CFPollRequest = false;
        capabilityInfo.privacy = false; // wep not implemented

        static_cast<BeaconFrameBody*>(frameBody)->
          setHandoverParameters(handoverParams);
        static_cast<BeaconFrameBody*>(frameBody)->
          setCapabilityInformation(capabilityInfo);
        static_cast<BeaconFrameBody*>(frameBody)->
          setSSID(ssid.c_str()); // need size
        static_cast<BeaconFrameBody*>(frameBody)->
          setSupportedRatesArraySize(rates.size());

        // variable size
        for (i = 0; i < rates.size(); i++)
          static_cast<BeaconFrameBody*>(frameBody)->
            setSupportedRates(i, rates[i]);

        static_cast<BeaconFrameBody*>(frameBody)->
          setDSChannel(channel);

        frameBody->setLength(FL_TIMESTAMP + FL_BEACONINT + FL_CAPINFO +
                             FL_IEHEADER + ssid.length() + FL_IEHEADER +
                             rates.size() + FL_DSCHANNEL);
      }
      break;

            case ST_AUTHENTICATION:
                frameBody = new AuthenticationFrameBody;

                // Only supports "open mode", therefore only two sequence number
                if(apMode == true)
                    static_cast<AuthenticationFrameBody*>(frameBody)->setSequenceNumber(2);
                else
                    static_cast<AuthenticationFrameBody*>(frameBody)->setSequenceNumber(1);

          // TODO: successful for now, but how are we going to represent
          // in module?
          static_cast<AuthenticationFrameBody*>(frameBody)->setStatusCode(0);

          frameBody->setLength(FL_STATUSCODE + FL_SEQNUM);
      break;
      case ST_ASSOCIATIONRESPONSE:
      {
        frameBody = new AssociationResponseFrameBody;

        // adhoc mode not supported, should add indication in module
        capabilityInfo.ESS = !adhocMode;
        capabilityInfo.IBSS = adhocMode;
        capabilityInfo.CFPollable = false; // CF not implemented
        capabilityInfo.CFPollRequest = false;
        capabilityInfo.privacy = false; // wep not implemented

        static_cast<AssociationResponseFrameBody*>(frameBody)->
          setCapabilityInformation(capabilityInfo);

        WirelessEtherInterface wie = findIfaceByMAC(f->getAddress1());
        assert(wie != UNSPECIFIED_WIRELESS_ETH_IFACE);

        static_cast<AssociationResponseFrameBody*>(frameBody)->
          setStatusCode(wie.statusCode);

        static_cast<AssociationResponseFrameBody*>(frameBody)->
          setSupportedRatesArraySize(rates.size());

        for (i = 0; i < rates.size(); i++)
          static_cast<AssociationResponseFrameBody*>(frameBody)->
            setSupportedRates(i, rates[i]);

        frameBody->setLength(FL_CAPINFO + FL_STATUSCODE + FL_IEHEADER +
                             rates.size());
      }
      break;

      case ST_REASSOCIATIONRESPONSE:
      {
        frameBody = new ReAssociationResponseFrameBody;

        capabilityInfo.ESS = !adhocMode;
        capabilityInfo.IBSS = adhocMode;
        capabilityInfo.CFPollable = false; // CF not implemented
        capabilityInfo.CFPollRequest = false;
        capabilityInfo.privacy = false; // wep not implemented

        static_cast<ReAssociationResponseFrameBody*>(frameBody)->
          setCapabilityInformation(capabilityInfo);

        WirelessEtherInterface wie = findIfaceByMAC(f->getAddress1());
        assert(wie != UNSPECIFIED_WIRELESS_ETH_IFACE);

        static_cast<ReAssociationResponseFrameBody*>(frameBody)->
          setStatusCode(wie.statusCode);

        static_cast<ReAssociationResponseFrameBody*>(frameBody)->
          setSupportedRatesArraySize(rates.size());

        for (i = 0; i < rates.size(); i++)
          static_cast<ReAssociationResponseFrameBody*>(frameBody)->
            setSupportedRates(i, rates[i]);

        frameBody->setLength(FL_CAPINFO + FL_STATUSCODE + FL_IEHEADER +
                             rates.size());
      }
      break;

      case ST_DISASSOCIATION:
      {
        frameBody = new DisAssociationFrameBody;

        WirelessEtherInterface wie = findIfaceByMAC(f->getAddress1());
        assert(wie != UNSPECIFIED_WIRELESS_ETH_IFACE);

        static_cast<DisAssociationFrameBody*>(frameBody)->
          setReasonCode(wie.reasonCode);

        // remove the wireless Ethernet interface off the list since
        // it is disassociated with the AP
        ifaces->removeEntry(wie);

        frameBody->setLength(FL_REASONCODE);
      }
      break;

      case ST_PROBERESPONSE:
      {
        frameBody = new ProbeResponseFrameBody;

        static_cast<ProbeResponseFrameBody*>(frameBody)->
          setTimestamp((double)simulation.simTime());

        // need to find out what is a TU in standard
        static_cast<ProbeResponseFrameBody*>(frameBody)->
          setBeaconInterval((int)(1000*beaconPeriod));

        // Additional parameters for HO decision (not part of standard)
        handoverParams.avgBackoffTime = totalBackoffTime.average;
        handoverParams.avgWaitTime = totalWaitTime.average;
        handoverParams.avgErrorRate = errorPercentage;
        handoverParams.estAvailBW = estAvailBW;

        capabilityInfo.ESS = !adhocMode;
        capabilityInfo.IBSS = adhocMode;
        capabilityInfo.CFPollable = false; // CF not implemented
        capabilityInfo.CFPollRequest = false;
        capabilityInfo.privacy = false; // wep not implemented

        static_cast<BeaconFrameBody*>(frameBody)->
          setHandoverParameters(handoverParams);
        static_cast<ProbeResponseFrameBody*>(frameBody)->
          setCapabilityInformation(capabilityInfo);
        static_cast<ProbeResponseFrameBody*>(frameBody)->
          setSSID(ssid.c_str());

        static_cast<ProbeResponseFrameBody*>(frameBody)->
          setSupportedRatesArraySize(rates.size());

        for (i = 0; i < rates.size(); i++)
          static_cast<ProbeResponseFrameBody*>(frameBody)->
            setSupportedRates(i, rates[i]);

        static_cast<ProbeResponseFrameBody*>(frameBody)->
          setDSChannel(channel);

        frameBody->setLength(FL_TIMESTAMP + FL_BEACONINT + FL_CAPINFO +
                             FL_IEHEADER + ssid.length() + FL_IEHEADER +
                             rates.size() + FL_DSCHANNEL);
      }
      break;

      case ST_DEAUTHENTICATION:
      {
        frameBody = new DeAuthenticationFrameBody;

        // Removed since deauthentication is sent when MS is not an entry
        // in the table. In which case will cause an assertion. So for now
        // the reason code is RC_DEAUTH_MS_LEAVING
        //
        //WirelessEtherInterface wie = findIfaceByMAC(f->getAddress1());
        //assert(wie != UNSPECIFIED_WIRELESS_ETH_IFACE);

        static_cast<DeAuthenticationFrameBody*>(frameBody)->
          setReasonCode(RC_DEAUTH_MS_LEAVING);
                //setReasonCode(wie.reasonCode);

        frameBody->setLength(FL_REASONCODE);
      }
      break;

      default:
        assert(false);
      break;
    }
    return frameBody;
}

void WirelessAccessPoint::addIface(MACAddress6 mac, ReceiveMode receiveMode)
{
  WirelessEtherInterface iface;

  iface.address = mac;
  iface.receiveMode = receiveMode;
  iface.expire = simTime();
    iface.consecFailedTrans = 0;

  // Choosing entry's expiry time depending on
  // its type
    if(receiveMode == RM_AUTHRSP_ACKWAIT)
        iface.expire += authWaitEntryTimeout;
    else if(receiveMode == RM_ASSOCIATION)
        iface.expire += authEntryTimeout;
    else if(receiveMode == RM_ASSRSP_ACKWAIT)
        iface.expire += authEntryTimeout;
    //iface.expire += iface.expire - elapsed;
    else if(receiveMode == RM_DATA)
        iface.expire += assEntryTimeout;

      Dout(dc::wireless_ethernet|flush_cf, "MAC LAYER: (WIRELESS) "
           << fullPath() << " " << simTime()
           << " ADDR: " << iface.address.stringValue()
           << " RM: " << iface.receiveMode
           << " EXP: " <<  iface.expire);

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
  if(!ifaces->findEntry(target))
   target = UNSPECIFIED_WIRELESS_ETH_IFACE;

  return target;
}

// For the frame destination at the front of the buffer, update
// the number of consecutive failed data frame tranmissions.
void WirelessAccessPoint::updateConsecutiveFailedCount()
{
    assert(outputBuffer.size());

    // Access frame in the front of buffer
    WESignalData* a = *(outputBuffer.begin());
    WirelessEtherBasicFrame* outputFrame =
      static_cast<WirelessEtherBasicFrame*>(a->encapsulatedMsg());
    FrameControl frameControl = outputFrame->getFrameControl();

    // Only consider data frames
    if(frameControl.subtype == ST_DATA)
    {
        WirelessEtherInterface dest = findIfaceByMAC(outputFrame->getAddress1());

        // Check that destination is in AP list
        if(dest != UNSPECIFIED_WIRELESS_ETH_IFACE)
        {
            // Remove entry if consecutive transmission limit is reached
            if(dest.consecFailedTrans >= consecFailedTransLimit)
            {
                Dout(dc::wireless_ethernet|flush_cf, "MAC LAYER: (WIRELESS) "
           << fullPath() << " " << simTime()
           << " Entry removed: " << outputFrame->getAddress1());
                ifaces->removeEntry(dest);
            }
            else
            {
                dest.consecFailedTrans++;
                ifaces->addEntry(dest);
            }
        }
        else
            Dout(dc::wireless_ethernet|flush_cf, "MAC LAYER: (WIRELESS) "
           << fullPath() << " " << simTime()
           << " Entry already removed");

    }
}

void WirelessAccessPoint::updateStats(void)
{
  // Get the available bandwidth
  usedBW.average = usedBW.sampleTotal/usedBW.sampleTime;
  estAvailBW = (BASE_SPEED - usedBW.average*8)/(1024*1024);

  Dout(dc::wireless_ethernet|flush_cf, "MAC LAYER: (WIRELESS) "
       << fullPath() << " " << simTime()
       << " Estimated available Bandwidth (over "<< usedBW.sampleTime <<" sec interval): " << estAvailBW << "Mb/s");

  if(statsVec)
    estAvailBWVec->record(estAvailBW);

  WirelessEtherModule::updateStats();

  usedBWStat->collect((usedBW.average*8)/(1024*1024));

  usedBW.sampleTotal = 0;


}
