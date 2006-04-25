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
	@file DualInterfaceLayer.cc
	@brief Dual NIC implementation
	@author Steve Woon
	@date Ask Steve

*/

#include "sys.h"
#include "debug.h"

#include <iomanip>

#include <boost/bind.hpp>
#include "wirelessethernet.h"
#include "DualInterfaceLayer.h"
#include "IPv6Datagram.h"
#include "TimerConstants.h"
#include "WirelessExternalSignal_m.h"
#include "WirelessEtherFrameBody_m.h"
#include "WirelessEtherSignal_m.h"

#include "InterfaceTableAccess.h"
#include "opp_utils.h"
#include "cCallbackMessage.h"
#include "ExpiryEntryListSignal.h"

const int TMR_HANDOVERWAIT = 3011;
const int TMR_SETMONITORMODE = 3012;
const int TMR_MONITORCHANNEL = 3013;
const int TMR_APLISTENTRYTIMEOUT = 3014;
const int TMR_OBTAINSTATS = 3016;


Define_Module( DualInterfaceLayer );


AccessPointEntry::AccessPointEntry(int listSize)
{
  assert(listSize > 0);
  signalStrength.upSize(listSize);
}

bool operator==(const AccessPointEntry& lhs,
                const AccessPointEntry& rhs)
{
  return ( lhs.address == rhs.address);
}

// Will be used for findMaxEntry in APList
bool lessThan(const AccessPointEntry& lhs, const AccessPointEntry& rhs)
{
  return (lhs.signalStrength.getAverage() < rhs.signalStrength.getAverage());
}

DualInterfaceLayer::~DualInterfaceLayer()
{
  // clear buffered messages
  while(!buffer.empty())
  {
    delete buffer.front();
    buffer.pop_front();
  }

  // de-allocate timers
  delete handoverWaitTimer;
  delete settingMonitorMode;
  delete monitorChannelTimer;

  // eliminate list
  if(apList != NULL)
  {
    delete apList;
  }
}

void DualInterfaceLayer::initialize(int stage)
{
  if(stage == 0)
  {
      readConfiguration();
      handoverWaitTime = 5;
      monitorChannelTime = 0.2;
      obtainStatsTime = 1;
      // initially there is no connected or monitoring interfaces
      connectedLL = -1;
      monitoringLL = -1;
      APEntryLifetime = 100;
      HOThreshPower = -88;

      associatedAP.address = MAC_ADDRESS_UNSPECIFIED_STRUCT;
      associatedAP.DSChannel = -1;

      connectedSignalStrength = INVALID_POWER;
      connectedErrorPercentage = 0;
      connectedAvgBackoffTime = 0;
      connectedAvgWaitTime = 0;

      registerInterface();

      errorPercentageVec = new cOutVector("errorPerc");
      avgBackoffTimeVec = new cOutVector("avgBackoffTime");
      avgWaitTimeVec = new cOutVector("avgWaitTime");

      //apList = new ExpiryEntryList<AccessPointEntry>(this, TMR_APLISTENTRYTIMEOUT);
      apList = new APL(false);
      apList->startTimer();
  }
  else if(stage == 1)
  {
      // timer to wait before making handover decision
      handoverWaitTimer  =
        new cCallbackMessage("handoverWaitHandler", TMR_HANDOVERWAIT);
      *handoverWaitTimer = boost::bind(&DualInterfaceLayer::handoverDecision, this);

      // timer to wait before changing the channel of monitoring interface
      monitorChannelTimer =
        new cCallbackMessage("monitorNextChannel", TMR_MONITORCHANNEL);
      *monitorChannelTimer = boost::bind(&DualInterfaceLayer::monitorNextChannel, this);
      // timer to obtain statistics about connected interface
      obtainStatsTimer =
        new cCallbackMessage("obtainStats", TMR_OBTAINSTATS);
      *obtainStatsTimer = boost::bind(&DualInterfaceLayer::obtainStats, this);
      // start obtaining statistics
      scheduleAt(simTime() + SELF_SCHEDULE_DELAY, obtainStatsTimer);

      // timer to wait before setting a particular interface to monitor mode
      settingMonitorMode  =
        new cCallbackMessage("setMonitoringInterface", TMR_SETMONITORMODE);
      *settingMonitorMode = boost::bind(&DualInterfaceLayer::setMonitoringInterface,
      // set interface 1 to be in monitoring mode
                                        this, 1);
      scheduleAt(simTime() + SELF_SCHEDULE_DELAY, settingMonitorMode);


      // set interface 1 to channel 1
      WirelessExternalSignalChannel* chanReq = new WirelessExternalSignalChannel;
      assert(chanReq);
      chanReq->setType(ST_CHANNEL);
      chanReq->setChan(1);
      send(chanReq, "extSignalOut", 1);
      monitoringChannel = 1;
  }
}

void DualInterfaceLayer::readConfiguration()
{
    std::string addr = par("address").stringValue();
    address.set(addr.c_str());
}

InterfaceEntry *DualInterfaceLayer::registerInterface()
{
    InterfaceEntry *e = new InterfaceEntry();

    std::string tmp = std::string("wlan")+OPP_Global::ltostr(index());
    e->setName(tmp.c_str()); // XXX HACK -- change back to above code!

    e->_linkMod = this; // XXX remove _linkMod on the long term!! --AV

    // port: index of gate where parent module's "netwIn" is connected (in IP)
    int outputPort = gate("fromNL")->fromGate()->index();
    e->setOutputPort(outputPort);

    // generate a link-layer address to be used as interface token for IPv6
    unsigned int iid[2];
    iid[0] = ( address.intValue()[0] << 8 ) | 0xFF;
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

void DualInterfaceLayer::finish()
{}

// Set the specified interface to monitor
void DualInterfaceLayer::setMonitoringInterface(int interface)
{
    Dout(dc::dual_interface|flush_cf, "DUAL INTERFACE LAYER: (WIRELESS) " << currentTime() << " " << fullPath() << " setting interface: "<<interface<<" to Monitor Mode.");

    // Create and send monitoring signal to interface
    WirelessExternalSignal* monSig = new WirelessExternalSignal;
    assert(monSig);
    monSig->setType(ST_MONITOR);
    send(monSig, "extSignalOut", interface);
    monitoringLL = interface;

    // Start timer for switching monitoring channels
    if(monitorChannelTimer->isScheduled())
        monitorChannelTimer->cancel();
    scheduleAt(simTime() + monitorChannelTime, monitorChannelTimer);
}

// Request the connection status of a specified interface
void DualInterfaceLayer::requestConnectionStats(int interface)
{
    Dout(dc::dual_interface|flush_cf, "DUAL INTERFACE LAYER: (WIRELESS) " << currentTime() << " " << fullPath() << " requesting interface: "<<interface<<" connection statistics.");

    // Create and send request stat signal to interface
    WirelessExternalSignal* statReq = new WirelessExternalSignal;
    assert(statReq);
    statReq->setType(ST_STATS_REQUEST);
    send(statReq, "extSignalOut", interface);
}

// Sets the monitoring interface to monitor the next channel
void DualInterfaceLayer::monitorNextChannel(void)
{
    assert( monitoringLL != -1);
    // Calculate next channel to monitor
    monitoringChannel = (monitoringChannel+1)%MAX_CHANNEL;

    // Don't forget last channel
    if(monitoringChannel == 0)
        monitoringChannel = MAX_CHANNEL;

    Dout(dc::dual_interface|flush_cf, "DUAL INTERFACE LAYER: (WIRELESS) " << currentTime() << " " << fullPath() << " requesting interface: "<<monitoringLL<< " channel change to: "<<monitoringChannel<<".");

    // Create and send monitor channel to interface
    WirelessExternalSignalChannel* chanReq = new WirelessExternalSignalChannel;
    assert(chanReq);
    chanReq->setType(ST_CHANNEL);
    chanReq->setChan(monitoringChannel);
    send(chanReq, "extSignalOut", monitoringLL);

    // arrange next channel change
    scheduleAt(simTime() + monitorChannelTime, monitorChannelTimer);
}

// Function to obtain statistics for the connected interface
void DualInterfaceLayer::obtainStats(void)
{
    if(connectedLL != -1)
        requestConnectionStats(connectedLL);

    scheduleAt(simTime() + obtainStatsTime, obtainStatsTimer);
}

void DualInterfaceLayer::handleMessage(cMessage* msg)
{
    assert(msg);

    // Only process external messages
    if(!msg->isSelfMessage())
    {
        // Message is from link layer
        if(strcmp(msg->arrivalGate()->name(),"fromLL") == 0)
        {
            handleLinkLayerMessage(msg);
        }
        // Message from network layer
        else if(strcmp(msg->arrivalGate()->name(),"fromNL") == 0)
        {
            handleNetworkLayerMessage(msg);
        }
        // Message from external signalling channel
        else
        {
            handleSignallingMessage(msg);
        }
    }
    // Self timer messages
    else
    {
        check_and_cast<cTimerMessage*>(msg)->callFunc();
    }
}

void DualInterfaceLayer::handleLinkLayerMessage(cMessage *msg)
{
    // Message from connected link layer
    if(msg->arrivalGate()->index() == connectedLL)
    {
        Dout(dc::dual_interface|flush_cf, "DUAL INTERFACE LAYER: (WIRELESS) " << currentTime() << " " << fullPath() << " frame arrived on fromLL["<<connectedLL<<"](associated interface) and forwarded to Network Layer.");

        send(msg, "toNL");
    }
    // Message from monitoring interface
    else if(msg->arrivalGate()->index() == monitoringLL)
    {
        Dout(dc::dual_interface|flush_cf, "DUAL INTERFACE LAYER: (WIRELESS) " << currentTime() << " " << fullPath() << " frame arrived on fromLL["<<msg->arrivalGate()->index()<<"](monitoring interface)");

        // process/handle frame from monitor interface
        WESignalData* signal = check_and_cast<WESignalData*>(msg);
        processMonitorFrames(signal);
    }
}
void DualInterfaceLayer::handleNetworkLayerMessage(cMessage *msg)
{
    // Message needs buffering
    if(connectedLL == -1)
    {
        Dout(dc::dual_interface|flush_cf, "DUAL INTERFACE LAYER: (WIRELESS) " << currentTime() << " " << fullPath() << " frame arrived on Network Layer and buffered.");

        //Buffer it if not connected
        buffer.push_back(msg);
    }
    //Send it directly to connected interface
    else
    {
        Dout(dc::dual_interface|flush_cf, "DUAL INTERFACE LAYER: (WIRELESS) " << currentTime() << " " << fullPath() << " frame arrived on Network Layer and forwarded to toLL["<<connectedLL<<"].");

        send(msg, "toLL", connectedLL);
    }
}

void DualInterfaceLayer::handleSignallingMessage(cMessage* msg)
{
    WirelessExternalSignal* extSig = check_and_cast<WirelessExternalSignal*>(msg);

    // Signal involving connection result
    if(extSig->getType() == ST_CONNECTION_STATUS)
    {
        WirelessExternalSignalConnectionStatus* res = check_and_cast<WirelessExternalSignalConnectionStatus*>(msg);

        // Associated
        if(res->getState() == S_ASSOCIATED)
        {
            // End handover initiation timer
            if(handoverWaitTimer->isScheduled())
                handoverWaitTimer->cancel();

            // Update new connected interface
            connectedLL = msg->arrivalGate()->index();
            // Update new monitoring interface
            if(connectedLL == 0)
                monitoringLL=1;
            else
                monitoringLL=0;
            //Update associated AP details
            associatedAP.address = res->getConnectedAddress();
            associatedAP.DSChannel = res->getConnectedChannel();
            associatedAP.signalStrength.updateAverage(res->getSignalStrength());
            associatedAP.estAvailBW.updateAverage(res->getEstAvailBW());
            associatedAP.errorPercentage.updateAverage(res->getErrorPercentage());
            associatedAP.avgBackoffTime.updateAverage(res->getAvgBackoffTime());
            associatedAP.avgWaitTime.updateAverage(res->getAvgWaitTime());

            setMonitoringInterface(monitoringLL);

            Dout(dc::dual_interface|flush_cf, "DUAL INTERFACE LAYER: (WIRELESS) " << currentTime() << " " << fullPath() << " Association successful on interface "<<connectedLL<<" channel: "<<associatedAP.DSChannel);
        }
        else if(res->getState() == S_DISCONNECTED)
        {
            connectedLL = -1;
            connectedSignalStrength = INVALID_POWER;
            connectedErrorPercentage = 0;
            connectedAvgBackoffTime = 0;
            connectedAvgWaitTime = 0;

            Dout(dc::dual_interface|flush_cf, "DUAL INTERFACE LAYER: (WIRELESS) " << currentTime() << " " << fullPath() << " Disconnection on interface "<<connectedLL<<".");
        }

    }
    // Signal involving statistics reading
    else if(extSig->getType() == ST_STATS)
    {
        assert(msg->arrivalGate()->index() == connectedLL);

        // Record signal reading
        WirelessExternalSignalStats* sigStat = check_and_cast<WirelessExternalSignalStats*>(msg);
        connectedSignalStrength = sigStat->getSignalStrength();
        connectedErrorPercentage = sigStat->getErrorPercentage();
        connectedAvgBackoffTime = sigStat->getAvgBackoffTime();
        connectedAvgWaitTime = sigStat->getAvgWaitTime();

        errorPercentageVec->record(connectedErrorPercentage);
        avgBackoffTimeVec->record(connectedAvgBackoffTime);
        avgWaitTimeVec->record(connectedAvgWaitTime);

        Dout(dc::dual_interface|flush_cf, "DUAL INTERFACE LAYER: (WIRELESS) " << currentTime() << " " << fullPath() << " Signal Strength from interface "<<connectedLL<<" is "<<connectedSignalStrength);

        // Make handover decision
        handoverDecision();
    }
    // Unrecognised signal
    else
        assert(false);
}

// Function to determine if handover is required
bool DualInterfaceLayer::handoverRequired(void)
{
    // Note that connectedSignalStrength can equal -1000 which is INVALID_POWER
    if(connectedSignalStrength < HOThreshPower)
        return true;

    return false;
}

// Function to make handover decisions
void DualInterfaceLayer::handoverDecision(void)
{
    // Make sure handover has not already been requested
    if(!handoverWaitTimer->isScheduled())
    {
        if(handoverRequired())
        {
            Dout(dc::dual_interface|flush_cf, "DUAL INTERFACE LAYER: (WIRELESS) " << currentTime() << " " << fullPath() << " Handover required.");

            // Find entry with highest signal strength
            AccessPointEntry highest;
            if(apList->findMaxEntry(highest, lessThan))
            {
                // Check if the highest is larger than handover threshold
                if( (highest.signalStrength.getAverage() > HOThreshPower)&&(highest.address != associatedAP.address) )
                {
                    Dout(dc::dual_interface|flush_cf, "DUAL INTERFACE LAYER: (WIRELESS) " << currentTime() << " " << fullPath() << " Found suitable handover target: "<<highest.address);

                    // Schedule for retry
                    assert(!handoverWaitTimer->isScheduled());
                    scheduleAt(simTime()+handoverWaitTime, handoverWaitTimer);

                    // Disable monitor mode channel switching
                    if(monitorChannelTimer->isScheduled())
                        monitorChannelTimer->cancel();

                    // Send handover signal to interface
                    WirelessExternalSignalHandover* hoSig = new WirelessExternalSignalHandover;
                    assert(hoSig);
                    hoSig->setType(ST_HANDOVER);
                    hoSig->setTarget(highest.address);
                    send(hoSig, "extSignalOut", monitoringLL);
                }
            }
        }
    }
}

// Decode monitored frames
void DualInterfaceLayer::processMonitorFrames(WESignalData* signal)
{
    WirelessEtherBasicFrame* frame = check_and_cast<WirelessEtherBasicFrame*>(signal->encapsulatedMsg());
    assert(frame);

    FrameControl frameControl = frame->getFrameControl();

    // Determine the type of frame
    switch(frameControl.subtype)
    {
      case ST_BEACON:
      {
          WirelessEtherManagementFrame* beacon =
              check_and_cast<WirelessEtherManagementFrame*>(signal->encapsulatedMsg());
          BeaconFrameBody* beaconBody =
              static_cast<BeaconFrameBody*>(beacon->decapsulate());

          Dout(dc::dual_interface|flush_cf, "DUAL INTERFACE LAYER: (WIRELESS) " << currentTime() << " " << fullPath() << " Beacon received.");

          AccessPointEntry apEntry;
          // Use beacon to keep track of surrounding APs
          apEntry.address = beacon->getAddress2();
          apEntry.DSChannel = beaconBody->getDSChannel();
          apList->findEntry(apEntry);
          apEntry.signalStrength.updateAverage(signal->power());
          apEntry.expire = simTime()+APEntryLifetime;
          apList->addOrUpdate(apEntry);

          // Check for current signal strength reading for associated interface
          //if(connectedLL != -1)
          //  requestConnectionStats(connectedLL);
      }
      break;
      case ST_PROBEREQUEST:
      {  Dout(dc::dual_interface|flush_cf, "DUAL INTERFACE LAYER: (WIRELESS) " << currentTime() << " " << fullPath() << " Prb Req received.");
      }
      break;
      case ST_PROBERESPONSE:
      {
          Dout(dc::dual_interface|flush_cf, "DUAL INTERFACE LAYER: (WIRELESS) " << currentTime() << " " << fullPath() << " Prb Resp received.");
      }
      break;
      case ST_ASSOCIATIONREQUEST:
      {
          Dout(dc::dual_interface|flush_cf, "DUAL INTERFACE LAYER: (WIRELESS) " << currentTime() << " " << fullPath() << " Ass Req received.");
      }
      break;
      case ST_ASSOCIATIONRESPONSE:
      {
          Dout(dc::dual_interface|flush_cf, "DUAL INTERFACE LAYER: (WIRELESS) " << currentTime() << " " << fullPath() << " Ass Resp received.");
      }
      break;
      case ST_REASSOCIATIONREQUEST:
      {
          Dout(dc::dual_interface|flush_cf, "DUAL INTERFACE LAYER: (WIRELESS) " << currentTime() << " " << fullPath() << " Re-Ass Req received.");
      }
      break;
      case ST_REASSOCIATIONRESPONSE:
      {
          Dout(dc::dual_interface|flush_cf, "DUAL INTERFACE LAYER: (WIRELESS) " << currentTime() << " " << fullPath() << " Re-Ass Resp received.");
      }
      break;
      case ST_DISASSOCIATION:
      {
          Dout(dc::dual_interface|flush_cf, "DUAL INTERFACE LAYER: (WIRELESS) " << currentTime() << " " << fullPath() << " DisAss received.");
      }
      break;
      case ST_DATA:
      {
          Dout(dc::dual_interface|flush_cf, "DUAL INTERFACE LAYER: (WIRELESS) " << currentTime() << " " << fullPath() << " Data received.");
      }
      break;
      case ST_ACK:
      {
          Dout(dc::dual_interface|flush_cf, "DUAL INTERFACE LAYER: (WIRELESS) " << currentTime() << " " << fullPath() << " Ack received.");
      }
      break;
      case ST_AUTHENTICATION:
      {
          Dout(dc::dual_interface|flush_cf, "DUAL INTERFACE LAYER: (WIRELESS) " << currentTime() << " " << fullPath() << " Auth received.");
      }
      break;
      case ST_DEAUTHENTICATION:
      {
          Dout(dc::dual_interface|flush_cf, "DUAL INTERFACE LAYER: (WIRELESS) " << currentTime() << " " << fullPath() << " De-Auth received.");
      }
      break;
      default:
      {
          Dout(dc::dual_interface|flush_cf, "DUAL INTERFACE LAYER: (WIRELESS) " << currentTime() << " " << fullPath() << " Unknown Frame received.");
      }
    }
    delete signal;
}
