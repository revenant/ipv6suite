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
	@file DualInterfaceLayer.cc
	@brief Dual NIC implementation
	@author Steve Woon
	@date Ask Steve

*/

#include "sys.h"
#include "debug.h"
	

#include <boost/cast.hpp>
#include <iomanip>

#include "wirelessethernet.h"
#include "DualInterfaceLayer.h"
#include "IPDatagram.h"
#include "Messages.h"
#include "IPv6Datagram.h"
#include "TimerConstants.h"
#include "WirelessExternalSignal_m.h"
#include "WirelessEtherFrameBody_m.h"

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

void DualInterfaceLayer::initialize(int stage)
{
  if(stage == 0)
  {
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

    errorPercentageVec = new cOutVector("errorPerc");
    avgBackoffTimeVec = new cOutVector("avgBackoffTime");
    avgWaitTimeVec = new cOutVector("avgWaitTime");

    apList = new ExpiryEntryList<AccessPointEntry>(this, TMR_APLISTENTRYTIMEOUT);

  }
  else if(stage == 1)
  {
    // timer to wait before making handover decision 
    handoverWaitTimer  =
      new Loki::cTimerMessageCB<void>
      (TMR_HANDOVERWAIT, this, this, &DualInterfaceLayer::handoverDecision, "handoverWaitHandler");

    // timer to wait before setting a particular interface to monitor mode
    settingMonitorMode  =
      new Loki::cTimerMessageCB<void, TYPELIST_1(int)>
      (TMR_SETMONITORMODE, this, this,&DualInterfaceLayer::setMonitoringInterface, "setMonitoringInterface");

    // timer to wait before changing the channel of monitoring interface
    monitorChannelTimer = 
      new Loki::cTimerMessageCB<void>
      (TMR_MONITORCHANNEL, this, this, &DualInterfaceLayer::monitorNextChannel, "monitorNextChannel");

    // timer to obtain statistics about connected interface
    obtainStatsTimer = 
      new Loki::cTimerMessageCB<void>
      (TMR_OBTAINSTATS, this, this, &DualInterfaceLayer::obtainStats, "obtainStats");

    // set interface 1 to be in monitoring mode  
    Loki::Field<0> (settingMonitorMode->args) = 1;
    scheduleAt(simTime() + SELF_SCHEDULE_DELAY, settingMonitorMode);

    // start obtaining statistics
    scheduleAt(simTime() + SELF_SCHEDULE_DELAY, obtainStatsTimer);

    // set interface 1 to channel 1
    WirelessExternalSignalChannel* chanReq = new WirelessExternalSignalChannel;
    assert(chanReq);
    chanReq->setType(ST_CHANNEL);
    chanReq->setChan(1);
    send(chanReq, "extSignalOut", 1);
    monitoringChannel = 1;
  }
}

void DualInterfaceLayer::finish()
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

// Set the specified interface to monitor
void DualInterfaceLayer::setMonitoringInterface(int interface)
{
  Dout(dc::dual_interface|flush_cf, "DUAL INTERFACE LAYER: (WIRELESS) " << std::fixed << std::showpoint << std::setprecision(12) << simTime() << " " << fullPath() << " setting interface: "<<interface<<" to Monitor Mode.");
  
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
  Dout(dc::dual_interface|flush_cf, "DUAL INTERFACE LAYER: (WIRELESS) " << std::fixed << std::showpoint << std::setprecision(12) << simTime() << " " << fullPath() << " requesting interface: "<<interface<<" connection statistics.");
  
  // Create and send monitoring signal to interface
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
  
  Dout(dc::dual_interface|flush_cf, "DUAL INTERFACE LAYER: (WIRELESS) " << std::fixed << std::showpoint << std::setprecision(12) << simTime() << " " << fullPath() << " requesting interface: "<<monitoringLL<< " channel change to: "<<monitoringChannel<<".");
  
  // Create and send monitoring signal to interface
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

void DualInterfaceLayer::handleLinkLayerMessage(std::auto_ptr<cMessage> msg)
{
  // Message from connected link layer
  if(msg->arrivalGate()->index() == connectedLL)
  {
    Dout(dc::dual_interface|flush_cf, "DUAL INTERFACE LAYER: (WIRELESS) " << std::fixed << std::showpoint << std::setprecision(12) << simTime() << " " << fullPath() << " frame arrived on fromLL["<<connectedLL<<"](associated interface) and forwarded to Network Layer.");
       
    send(msg.release(), "toNL");
  }
  // Message from monitoring interface
  else if(msg->arrivalGate()->index() == monitoringLL) 
  {
    Dout(dc::dual_interface|flush_cf, "DUAL INTERFACE LAYER: (WIRELESS) " << std::fixed << std::showpoint << std::setprecision(12) << simTime() << " " << fullPath() << " frame arrived on fromLL["<<msg->arrivalGate()->index()<<"](monitoring interface)");
        
    // process/handle frame from monitor interface
    WESignalData* signal = boost::polymorphic_downcast<WESignalData*>(msg.get());
    processMonitorFrames(signal);
  }
}
void DualInterfaceLayer::handleNetworkLayerMessage(std::auto_ptr<cMessage> msg)
{
  // Message needs buffering
  if(connectedLL == -1)
  {
    Dout(dc::dual_interface|flush_cf, "DUAL INTERFACE LAYER: (WIRELESS) " << std::fixed << std::showpoint << std::setprecision(12) << simTime() << " " << fullPath() << " frame arrived on Network Layer and buffered.");

    //Buffer it if not connected
    buffer.push_back(msg.release());
  }
  //Send it directly to connected interface
  else
  {
    Dout(dc::dual_interface|flush_cf, "DUAL INTERFACE LAYER: (WIRELESS) " << std::fixed << std::showpoint << std::setprecision(12) << simTime() << " " << fullPath() << " frame arrived on Network Layer and forwarded to toLL["<<connectedLL<<"].");

    send(msg.release(), "toLL", connectedLL);
  }
}

void DualInterfaceLayer::handleSignallingMessage(std::auto_ptr<cMessage> msg)
{
  WirelessExternalSignal* extSig = static_cast<WirelessExternalSignal*>(msg.get());
  
  // Signal involving connection result
  if(extSig->getType() == ST_CONNECTION_STATUS)
  {
    WirelessExternalSignalConnectionStatus* res = static_cast<WirelessExternalSignalConnectionStatus*>(msg.get());
        
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
      
      Dout(dc::dual_interface|flush_cf, "DUAL INTERFACE LAYER: (WIRELESS) " << std::fixed << std::showpoint << std::setprecision(12) << simTime() << " " << fullPath() << " Association successful on interface "<<connectedLL<<" channel: "<<associatedAP.DSChannel);
    }
    else if(res->getState() == S_DISCONNECTED)
    {
      connectedLL = -1;
      connectedSignalStrength = INVALID_POWER;
      connectedErrorPercentage = 0;
      connectedAvgBackoffTime = 0;
      connectedAvgWaitTime = 0;
          
      Dout(dc::dual_interface|flush_cf, "DUAL INTERFACE LAYER: (WIRELESS) " << std::fixed << std::showpoint << std::setprecision(12) << simTime() << " " << fullPath() << " Disconnection on interface "<<connectedLL<<".");
    }

  }
  // Signal involving statistics reading
  else if(extSig->getType() == ST_STATS)
  {
    assert(msg->arrivalGate()->index() == connectedLL);
        
    // Record signal reading
    WirelessExternalSignalStats* sigStat = static_cast<WirelessExternalSignalStats*>(msg.get());
    connectedSignalStrength = sigStat->getSignalStrength();
    connectedErrorPercentage = sigStat->getErrorPercentage();
    connectedAvgBackoffTime = sigStat->getAvgBackoffTime();
    connectedAvgWaitTime = sigStat->getAvgWaitTime();
    
    errorPercentageVec->record(connectedErrorPercentage);
    avgBackoffTimeVec->record(connectedAvgBackoffTime);
    avgWaitTimeVec->record(connectedAvgWaitTime);
    
    Dout(dc::dual_interface|flush_cf, "DUAL INTERFACE LAYER: (WIRELESS) " << std::fixed << std::showpoint << std::setprecision(12) << simTime() << " " << fullPath() << " Signal Strength from interface "<<connectedLL<<" is "<<connectedSignalStrength);
        
    // Make handover decision
    handoverDecision();
  }
  // Unrecognised signal
  else
    assert(false);
}
void DualInterfaceLayer::handleMessage(cMessage* pmsg)
{
  assert(pmsg);
  
  // Only process external messages
  if(!pmsg->isSelfMessage())
  {
    std::auto_ptr<cMessage> msg(pmsg); 
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
    static_cast<cTimerMessage*>(pmsg)->callFunc();
  }
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
      Dout(dc::dual_interface|flush_cf, "DUAL INTERFACE LAYER: (WIRELESS) " << std::fixed << std::showpoint << std::setprecision(12) << simTime() << " " << fullPath() << " Handover required.");
       
      // Find entry with highest signal strength
      AccessPointEntry highest;
      if(apList->findMaxEntry(highest, lessThan))
      {
        // Check if the highest is larger than handover threshold
        if( (highest.signalStrength.getAverage() > HOThreshPower)&&(highest.address != associatedAP.address) )
        {
          Dout(dc::dual_interface|flush_cf, "DUAL INTERFACE LAYER: (WIRELESS) " << std::fixed << std::showpoint << std::setprecision(12) << simTime() << " " << fullPath() << " Found suitable handover target: "<<highest.address);

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
  WirelessEtherBasicFrame* frame = signal->data();
  assert(frame);

  FrameControl frameControl = frame->getFrameControl();
  
  // Determine the type of frame
  switch(frameControl.subtype)
  {
    case ST_BEACON:
    {
      WirelessEtherManagementFrame* beacon =
        static_cast<WirelessEtherManagementFrame*>(signal->data());
      BeaconFrameBody* beaconBody =
        static_cast<BeaconFrameBody*>(beacon->decapsulate());

      Dout(dc::dual_interface|flush_cf, "DUAL INTERFACE LAYER: (WIRELESS) " << std::fixed << std::showpoint << std::setprecision(12) << simTime() << " " << fullPath() << " Beacon received.");
      
      AccessPointEntry apEntry;
      // Use beacon to keep track of surrounding APs
      apEntry.address = beacon->getAddress2();
      apEntry.DSChannel = beaconBody->getDSChannel();
      apList->findEntry(apEntry);
      apEntry.signalStrength.updateAverage(signal->power());
      apEntry.expire = simTime()+APEntryLifetime;
      apList->addEntry(apEntry);
      
      // Check for current signal strength reading for associated interface
      //if(connectedLL != -1)
      //  requestConnectionStats(connectedLL);
    }  
      break;
    case ST_PROBEREQUEST:
    {  Dout(dc::dual_interface|flush_cf, "DUAL INTERFACE LAYER: (WIRELESS) " << std::fixed << std::showpoint << std::setprecision(12) << simTime() << " " << fullPath() << " Prb Req received.");
    } 
     break;
    case ST_PROBERESPONSE:
    {
      Dout(dc::dual_interface|flush_cf, "DUAL INTERFACE LAYER: (WIRELESS) " << std::fixed << std::showpoint << std::setprecision(12) << simTime() << " " << fullPath() << " Prb Resp received.");
    }  
      break;
    case ST_ASSOCIATIONREQUEST:
    {  
      Dout(dc::dual_interface|flush_cf, "DUAL INTERFACE LAYER: (WIRELESS) " << std::fixed << std::showpoint << std::setprecision(12) << simTime() << " " << fullPath() << " Ass Req received.");
    }
      break;
    case ST_ASSOCIATIONRESPONSE:
    {
      Dout(dc::dual_interface|flush_cf, "DUAL INTERFACE LAYER: (WIRELESS) " << std::fixed << std::showpoint << std::setprecision(12) << simTime() << " " << fullPath() << " Ass Resp received.");
    }
      break;
    case ST_REASSOCIATIONREQUEST:
    {  
      Dout(dc::dual_interface|flush_cf, "DUAL INTERFACE LAYER: (WIRELESS) " << std::fixed << std::showpoint << std::setprecision(12) << simTime() << " " << fullPath() << " Re-Ass Req received.");
    }
      break;
    case ST_REASSOCIATIONRESPONSE:
    {  
      Dout(dc::dual_interface|flush_cf, "DUAL INTERFACE LAYER: (WIRELESS) " << std::fixed << std::showpoint << std::setprecision(12) << simTime() << " " << fullPath() << " Re-Ass Resp received.");
    }  
      break;
    case ST_DISASSOCIATION:
    {
      Dout(dc::dual_interface|flush_cf, "DUAL INTERFACE LAYER: (WIRELESS) " << std::fixed << std::showpoint << std::setprecision(12) << simTime() << " " << fullPath() << " DisAss received.");
    }  
      break;
    case ST_DATA:
    {
      Dout(dc::dual_interface|flush_cf, "DUAL INTERFACE LAYER: (WIRELESS) " << std::fixed << std::showpoint << std::setprecision(12) << simTime() << " " << fullPath() << " Data received.");
    }  
      break;
    case ST_ACK:
    {
      Dout(dc::dual_interface|flush_cf, "DUAL INTERFACE LAYER: (WIRELESS) " << std::fixed << std::showpoint << std::setprecision(12) << simTime() << " " << fullPath() << " Ack received.");
    }
      break;
    case ST_AUTHENTICATION:
    {  
      Dout(dc::dual_interface|flush_cf, "DUAL INTERFACE LAYER: (WIRELESS) " << std::fixed << std::showpoint << std::setprecision(12) << simTime() << " " << fullPath() << " Auth received.");
    }  
      break;
    case ST_DEAUTHENTICATION:
    {  
      Dout(dc::dual_interface|flush_cf, "DUAL INTERFACE LAYER: (WIRELESS) " << std::fixed << std::showpoint << std::setprecision(12) << simTime() << " " << fullPath() << " De-Auth received.");
    }
      break;
    default:
    {
      Dout(dc::dual_interface|flush_cf, "DUAL INTERFACE LAYER: (WIRELESS) " << std::fixed << std::showpoint << std::setprecision(12) << simTime() << " " << fullPath() << " Unknown Frame received.");
    }
  }
}

#endif
