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
#include "config.h"

#include <iostream>
#include <iomanip>


#include <cassert>

//#include <boost/tokenizer.hpp> // pulls in half of boost -- NO WAY!!! --Andras
#include "StringTokenizer.h"     // lot smaller than boost/tokenizer.hpp
#include <cmath> //std::pow
//#include <boost/random.hpp> // was not used --Andras
#include <math.h>
#include <sstream>
#include <string>


#include "cTTimerMessageCB.h"

#if MLDV2
#include "IPv6Datagram.h"
#endif
#include "IPv6InterfaceData.h"
#include "Messages.h"

#include "opp_utils.h"
//#include "opp_akaroa.h"  //XXX --Andras

#include "WorldProcessor.h"
#include "MobileEntity.h"
#include "MobilityHandler.h"
#include "PHYWirelessSignal.h"

#include "wirelessethernet.h"

#include "WirelessAccessPoint.h"
#include "WirelessEtherModule.h"
#include "WirelessEtherSignal.h"

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

#if L2FUZZYHO // (Layer 2 fuzzy logic handover)
#include"hodec.h"
#endif // L2FUZZYHO

#include "XML/XMLOmnetParser.h"

Define_Module_Like( WirelessEtherModule, NetworkInterface6);

void WirelessEtherModule::baseInit(int stage)
{
  if(stage == 0)
  {
    changeState(WirelessEtherStateIdle::instance());

    sequenceNumber = 0;
    inputFrame = 0;
    retry = 0;
    _wirelessRange = 0;
    resetContentionWindowPower();
    ackReceived = false;
    idleDest.clear();
    noOfRxFrames = 0;
    frameSource = "";
    backoffTime = 0;

//initialise variables for statistics
    throughput.sampleTotal = 0;
    throughput.sampleTime = 1;
    throughput.average = 0;
    noOfFailedTx = 0;
    noOfSuccessfulTx = 0;
    errorPercentage = 0;
    totalWaitTime.sampleTotal = 0;
    totalWaitTime.sampleTime = 1;
    totalWaitTime.average = 0;
    totalBackoffTime.sampleTotal = 0;
    totalBackoffTime.sampleTime = 1;
    totalBackoffTime.average = 0;

    throughputVec = new cOutVector("throughput");
    errorPercentageVec = new cOutVector("errorPerc");
    noOfFailedTxVec = new cOutVector("noOfFailedTx");
    totalBackoffTimeVec = new cOutVector("avgBackoffTime");
    totalWaitTimeVec = new cOutVector("avgWaitTime");
    totalBytesTransmitted = 0;

    cXMLElement* weinfo = par("nwiXmlConfig");
    if (weinfo)
    {
      XMLConfiguration::XMLOmnetParser p;
      //default.ini loads empty.xml at element netconf
      if (p.getNodeProperties(weinfo, "debugChannel", false) == "")
        p.parseWEInfo(this, weinfo);
      else
        Dout(dc::xml_addresses, " no global WEInfo for node "<<OPP_Global::nodeName(this));
    }

    wproc->parseWirelessEtherInfo(this);
    beginCollectionTime = OPP_Global::findNetNodeModule(this)->par("beginCollectionTime").doubleValue();
    endCollectionTime = OPP_Global::findNetNodeModule(this)->par("endCollectionTime").doubleValue();
  }
  else if(stage == 1)
  {
    std::cout<<"statsVec: "<<statsVec<<endl;

    cModule* mobMan = OPP_Global::findModuleByName(this, "mobilityHandler");
    assert(mobMan);
    _mobCore = static_cast<MobilityHandler*>(mobMan);

// Timer to update statistics
    updateStatsNotifier  =
      new Loki::cTimerMessageCB<void>
      (TMR_STATS, this, this, &WirelessEtherModule::updateStats, "updateStats");

    scheduleAt(simTime()+1, updateStatsNotifier);
  }

}

void WirelessEtherModule::initialize(int stage)
{
  if (stage == 0)
  {
    l2LinkDownRecorder = 0;
    l2DelayRecorder = 0;
    l2HODelay = new cOutVector("IEEE 802.11 HO Latency");
    linkdownTime = 0;

    LinkLayerModule::initialize();
    cModule* mod = OPP_Global::findModuleByName(this, "worldProcessor");
    assert(mod);
    wproc = static_cast<WorldProcessor*>(mod);
    setIface_name(PR_WETHERNET);
    iface_type = PR_WETHERNET;

    procdelay = par("procdelay").longValue();
    apMode = false;

    channel = 0;
    associateAP.address =  MAC_ADDRESS_UNSPECIFIED_STRUCT;
    associateAP.channel = INVALID_CHANNEL;
    associateAP.rxpower = INVALID_POWER;
    associateAP.associated = false;
    associateAP.estAvailBW = 0;
    associateAP.errorPercentage = 0;
    associateAP.avgBackoffTime = 0;
    associateAP.avgWaitTime = 0;

    handoverTarget.valid = false;

    for(int i = 0; i < NumTrigVals ; i++)
    {
      l2Trigger[i] = 0;
    }
  }
  else if (stage == 1)
  {
    // list to store signal strength readings
    signalStrength = new AveragingList(sSMaxSample);

    initialiseChannelToScan();

    // On power up, wireless ethernet interface is staarting to
    // perform active scanning..
    if(activeScan)
    {
      _currentReceiveMode = WEAScanReceiveMode::instance();

      cTTimerMessageCBA<void, void>* prbEnergyScanNotifier;

      prbEnergyScanNotifier  =
        new cTTimerMessageCBA<void, void>
        (TMR_PRBENERGYSCAN, this, makeCallback(this, &WirelessEtherModule::probeChannel), "probeChannel");
      addTmrMessage(prbEnergyScanNotifier);
      scheduleAt(simTime() + SELF_SCHEDULE_DELAY, prbEnergyScanNotifier);

      //Timer for probe response wait
      cTTimerMessageCBA<void, void>* prbRespScanNotifier;

      prbRespScanNotifier  =
        new cTTimerMessageCBA<void, void>
        (TMR_PRBRESPSCAN, this, makeCallback(this, &WirelessEtherModule::probeChannel), "probeChannel");
      addTmrMessage(prbRespScanNotifier);
    }
    else
    {
      _currentReceiveMode = WEPScanReceiveMode::instance();

      cTTimerMessageCBA<void, void>* channelScanNotifier;

      channelScanNotifier  =
        new cTTimerMessageCBA<void, void>
        (TMR_CHANNELSCAN, this, makeCallback(this, &WirelessEtherModule::passiveChannelScan), "passiveChannelScan");
      addTmrMessage(channelScanNotifier);
      scheduleAt(simTime() + SELF_SCHEDULE_DELAY, channelScanNotifier);
    }

    //Timer to give authentication procedure a timelimit
    cTTimerMessageCBA<void, void>* authTimeoutNotifier;

    authTimeoutNotifier  =
      new cTTimerMessageCBA<void, void>
      (TMR_AUTHTIMEOUT, this, makeCallback(this, &WirelessEtherModule::authTimeoutHandler), "authTimeoutHandler");
    addTmrMessage(authTimeoutNotifier);

    //Timer to give association procedure a timelimit
    cTTimerMessageCBA<void, void>* assTimeoutNotifier;

    assTimeoutNotifier  =
      new cTTimerMessageCBA<void, void>
      (TMR_ASSTIMEOUT, this, makeCallback(this, &WirelessEtherModule::assTimeoutHandler), "assTimeoutHandler");
    addTmrMessage(assTimeoutNotifier);

    Dout(dc::wireless_ethernet|flush_cf, "MAC LAYER: (WIRELESS) "
         << fullPath() << "\n"
         << " ====================== \n"
         << " MAC ADDR: " << address.stringValue() << "\n"
         << " SSID: " << ssid.c_str() << "\n"
         << " PATH LOSS EXP: " << pLExp << "\n"
         << " STD DEV: " << pLStdDev << "dB\n"
         << " TXPOWER: " << txpower << "mW\n"
         << " THRESHOLDPOWER: " << threshpower << "dBm\n"
         << " HOTHRESHOLDPOWER: " << hothreshpower << "dBm\n"
         <<    " PROBEENERGY_TIMEOUT: " << probeEnergyTimeout << "\n"
         << " PROBERESPONSE_TIMEOUT: " << probeResponseTimeout << "\n"
         << " AUTHENTICATION_TIMEOUT: " << authenticationTimeout << " (TU)\n"
         << " ASSOCIATION_TIMEOUT: " << associationTimeout << " (TU)\n"
         << " MAXRETRY: " << maxRetry << "\n"
         << " FAST_ACTIVE_SCAN: " << fastActScan << "\n"
         << " CROSSTALK: " << crossTalk << "\n"
         << " SHADOWING: " << shadowing << "\n"
         << " CHANNELS TO AVOID: " << chanNotToScan << "\n"
         << " MAX SS SAMPLE COUNT: " << sSMaxSample << "\n");
  }
  baseInit(stage);
}

void WirelessEtherModule::finish()
{
  delete [] channelToScan;
  delete signalStrength;
}

// Function to handle external signals
void WirelessEtherModule::receiveSignal(std::auto_ptr<cMessage> msg)
{
  WirelessExternalSignal* extSig = static_cast<WirelessExternalSignal*>(msg.get());

  // Monitor mode signal
  if(extSig->getType() == ST_MONITOR)
  {
    startMonitorMode();
  }
  // Channel specifier signal
  else if(extSig->getType() == ST_CHANNEL)
  {
    WirelessExternalSignalChannel* sigChan = static_cast<WirelessExternalSignalChannel*>(msg.get());
    channel = sigChan->getChan();

    Dout(dc::wireless_ethernet|flush_cf, "MAC LAYER: (WIRELESS) " << std::fixed << std::showpoint << std::setprecision(12) << simTime() << " Switching to channel: " << channel);
    startMonitorMode();
  }
  // Request for stats reading from associated AP
  else if(extSig->getType() == ST_STATS_REQUEST)
  {
    sendStatsSignal();
  }
  // Request handover to a specified target
  else if(extSig->getType() == ST_HANDOVER)
  {
    // Keep record of the target and indicate its valid
    WirelessExternalSignalHandover* sigHO = static_cast<WirelessExternalSignalHandover*>(msg.get());
    handoverTarget.target.address = sigHO->getTarget();
    handoverTarget.valid = true;

    Dout(dc::wireless_ethernet|flush_cf, "MAC LAYER: (WIRELESS) " << fullPath()<< " " << std::fixed << std::showpoint << std::setprecision(12) << simTime() << " Handover to target: " << handoverTarget.target.address);
    // Start active scan for handover
    restartScanning();
  }
  else
    assert(false);

  // need to handle association signal as well
}

void WirelessEtherModule::receiveData(std::auto_ptr<cMessage> msg)
{
  //if (_currentReceiveMode != WEDataReceiveMode::instance())
  //  return;

/*XXX changed to use control info --AV
  LLInterfacePkt* recPkt = check_and_cast<LLInterfacePkt*>
    (msg.get());
  assert(recPkt != 0);
*/
  LL6ControlInfo *ctrlInfo = check_and_cast<LL6ControlInfo*>(msg->removeControlInfo());
  WirelessEtherBasicFrame* frame = createFrame
    (FT_DATA, ST_DATA, address, MACAddress6(ctrlInfo->getDestLLAddr()));
  delete ctrlInfo;

/*XXX replaced with 1 line below --AV
  cMessage* dupMsg = recPkt->data().dgram->dup();
  delete recPkt->data().dgram;
*/
  //XXX cMessage* dupMsg = recPkt->data().dgram;
  //XXX frame->setProtocol(PR_WETHERNET);
  frame->encapsulate(msg.get());
  frame->setName(msg->name());
  msg.release();   // XXX maybe get rid of auto_ptr here? --AV

  // relying on internal buffer, therefore need to get data from external
  // outputQueue into it quickly.
  idleNetworkInterface();

  if(associateAP.associated)
    {
      WESignalData* a = new WESignalData(frame);
      a->setName(frame->name());
      outputBuffer.push_back(a);
      delete frame;

      if ( _currentState == WirelessEtherStateIdle::instance())
        static_cast<WirelessEtherStateIdle*>(_currentState)->chkOutputBuffer(this);
      }
    else
      // Note that since no association with an AP exists in DataReceiveMode,
      // there will be no address for the AP destination. Hence, this should
      // be filled in before sending.  It should be added in address1.
      offlineOutputBuffer.push_back(frame);
}

void WirelessEtherModule::handleMessage(cMessage* msg)
{
  assert(msg);

  if ( !msg->isSelfMessage())
  {
    ++cntReceivedPackets;
    _currentState->processSignal(this, auto_ptr<cMessage>(msg));
  }
  else
  {
    printSelfMsg(msg);
    static_cast<cTimerMessage*>(msg)->callFunc();
  }
}

/**
 * @todo can make this a virtual function of mobile interfaces etc. will need a
 * function in LinkLayerModule to tell which interfaces are mobile? or maybe all * interfaces can have connect and disconnect triggers.
 *
 */

void WirelessEtherModule::setLayer2Trigger( cTimerMessage* trig, enum TrigVals v)
{
  if (l2Trigger[v])
    delete l2Trigger[v];

  l2Trigger[v] = trig;

  //dc::wireless_ethernet.precision(6);
  Dout(dc::wireless_ethernet|flush_cf, "Set Layer 2 Trigger: (WIRELESS) "
         << fullPath() << " #: "<< v << "\n");

}
const unsigned int* WirelessEtherModule::macAddress(void)
{
  return address.intValue();
}

std::string WirelessEtherModule::macAddressString(void)
{
  return address.stringValue();
}

cTimerMessage* WirelessEtherModule::getTmrMessage(const int& messageID)
{
  for (TIT it = tmrs.begin(); it != tmrs.end(); it++)
  {
    if ((*it)->kind() == messageID)
      return (*it);
  }
  return 0;
}

void WirelessEtherModule::sendFrame(WESignal* msg)
{
  // Make sure list of idles to send is cleared
  idleDest.clear();

  // Go up two levels to obtain just the unique module name
  std::string modName = parentModule()->parentModule()->fullPath();
  msg->setSourceName(modName);

  double r1,r2,r3,r4;
  int chanSep;

  //Find the max and min channel which crosstalk can occur
  int maxChan = (channel+4 > MAX_CHANNEL) ? MAX_CHANNEL : channel+4;
  int minChan = (channel-4 < 1) ? 1 : channel-4;

  //Find modules which can receive the frame
  // XXX this doesn't compile if USE_MOBILITY is undefined! --AV
  ModuleList mods = wproc->
  findWirelessEtherModulesByChannelRange(minChan, maxChan);

  // Four random values which will be used to determine whether
  // data will cross over to adjacent channels

  r1 = uniform(0,100);
  r2 = uniform(0,100);
  r3 = uniform(0,100);
  r4 = uniform(0,100);

  // Go through each module and determine whether to transmit to them
  for ( MLIT it = mods.begin(); it != mods.end(); it++)
  {
    Entity* e = (*it);
    cModule* interface =  e->containerModule()->parentModule()->parentModule();
    size_t noOfInterface = interface->gate("wlin")->size();

    //Send to relevent interface in a node
    for(size_t i=0; i< noOfInterface; i++)
    {
      cModule* phylayer = interface->gate("wlin",i)->toGate()->ownerModule();
      cModule* linkLayer =  phylayer->gate("linkOut")->toGate()->ownerModule();
      assert(linkLayer);

      WirelessEtherModule* a = static_cast<WirelessEtherModule*>(linkLayer->submodule("networkInterface"));
      // Dont send to ourselves
      if ( a == this)
        continue;

      assert(a->_mobCore);
      int distance = _mobCore->getEntity()->
        distance(a->_mobCore->getEntity());

      double rxPower = getRxPower(distance);

      // estimated receiving power must be greater than or equal to the
      // threshold power of the other end to in order to reach the
      // destination
      if ((rxPower >= a->getThreshPower()) && (a->channel != 0))
      {
        chanSep = abs(channel - a->channel);
        // Determine whether crosstalk will occur based on probabilities:
        // 73% for  1 channel seperation
        // 27% for  2   "
        // 4% for   3   "
        // 0.5% for 4   "
        // These figures were obtained from the White Paper
        // "Channel Overlap Calculations for 802.11b Networks" by Mitchell Burton
        // of Cirond Technologies
        if(    (chanSep == 0)
            || (crossTalk
            &&(((chanSep == 1)&&(r1<73))
            || ((chanSep == 2)&&(r2<27))
            || ((chanSep == 3)&&(r3<4))
            || ((chanSep == 4)&&(r4<.5))))
          )
        {
          msg->setPower(rxPower);
          msg->setChannel(a->channel);
          // propagation delay
          double propDelay = distance / (3 * pow((double)10, (double)8));
          // Mark the module which need the end of the frame
          // Need to also remember the Rx Power
          boost::shared_ptr<DestInfo> dInfo(new DestInfo);
          dInfo->mod = a;
          dInfo->index = i;
          dInfo->rxPower = rxPower;
          dInfo->channel = a->channel;
          idleDest.push_back(dInfo);

          // Dout(dc::wireless_ethernet|flush_cf, "MAC LAYER: (WIRELESS) " << std::fixed << std::showpoint << std::setprecision(12) << simTime() << " Sending SOF to: " << interface->fullPath() << " on index: "<< i);
          sendDirect(msg->dup(), propDelay, interface, "wlin", i);
        }
      }
    }
  }
  mods.clear();
}

void WirelessEtherModule::sendEndOfFrame()
{
  WESignalIdle* idle = new WESignalIdle;

  // Go up two levels to obtain just the unique module name
  std::string modName = parentModule()->parentModule()->fullPath();
  idle->setSourceName(modName);

  //Check if modules need the end of the frame
  if(!idleDest.empty())
  {
    assert(_mobCore);

    for ( IDIT it = idleDest.begin(); it != idleDest.end(); it++)
    {
      // Check if the station is still in the same channel to receive the end of the frame
      if((*it)->mod->channel == (*it)->channel)
      {
        int distance = _mobCore->getEntity()->
                        distance((*it)->mod->_mobCore->getEntity());

        // use the same calculated rxPower as for start of frame
        double rxPower = (*it)->rxPower;

        // if the end of frame is suddenly under the receiving stations threshold,
        // there must be something wrong (Entity shouldnt be moving that quick!)
        assert(rxPower >= (*it)->mod->getThreshPower());

        // set end of frame properties
        idle->setPower(rxPower);
        idle->setChannel((*it)->mod->channel);

        // propagation delay
        double propDelay = distance / (3 * pow((double)10, (double)8));

        cModule* interface =  static_cast<cModule*>((*it)->mod->parentModule()->parentModule());

        //Dout(dc::wireless_ethernet|flush_cf, "MAC LAYER: (WIRELESS) " << std::fixed << std::showpoint << std::setprecision(12) << simTime() << " Sending EOF to: " << interface->fullPath() << " on index: "<< (*it)->index);

        sendDirect(idle->dup(), propDelay, interface, "wlin", (*it)->index);
      }
    }
    idleDest.clear();
  }

  delete idle;
}

void WirelessEtherModule::idleNetworkInterface(void)
{
  /* XXX notifying the output queue removed from here --AV */
}

/**
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
  double rxpwr = 10*log10((double)txpower) - 40 - 10*pLExp*log10((double)distance);
  if(shadowing)
    rxpwr += normal(0, pLStdDev);

  return rxpwr;
}

bool WirelessEtherModule::handleSendingBcastFrame(void)
{
  assert(!outputBuffer.empty());
  WESignalData* signal = *(outputBuffer.begin());
  assert(signal->data());

  WirelessEtherBasicFrame* frame = static_cast<WirelessEtherBasicFrame*>
    (signal->data());

  assert(frame);

  if (frame->getAddress1() == MACAddress6(ETH_BROADCAST_ADDRESS))
  {
    if (frame->getFrameControl().subtype == ST_PROBEREQUEST)
    {
      // Start probe energy timeout when the probe frame is sent.
      // schedule the next scan message which will process the scan
      // after the SCAN_INTERVAL if nothing is received
      double nextSchedTime = simTime() + probeEnergyTimeout;

      cTimerMessage* tmr = getTmrMessage(TMR_PRBENERGYSCAN);
      assert(tmr && !tmr->isScheduled());

      tmr->reschedule(nextSchedTime);
  }

    assert(!getRetry());
    Dout(dc::wireless_ethernet|flush_cf, "MAC LAYER: " << std::fixed << std::showpoint << setprecision(12)<< simTime()
       << " sec, " << fullPath() << " outputBuff count: " << outputBuffer.size());
    assert(!outputBuffer.empty());
    delete signal;
    outputBuffer.pop_front();

    idleNetworkInterface();

    changeState(WirelessEtherStateIdle::instance());
    static_cast<WirelessEtherStateIdle*>(_currentState)->chkOutputBuffer(this);

    return true;
  }

  return false;
}

void WirelessEtherModule::scanNextChannel(void)
{
  assert(!apMode);

  reset();

  delete inputFrame;

  cTimerMessage* tmr_scan = getTmrMessage(TMR_PRBENERGYSCAN);
  tmr_scan->reschedule(simTime() + SELF_SCHEDULE_DELAY);
}

void WirelessEtherModule::sendSuccessSignal(void)
{
  // XXX change these notifications to use the Blackboard --AV
  if (gate("extSignalOut")==NULL || gate("extSignalOut")->toGate()==NULL)
    return;
  cModule* linkLayer =  gate("extSignalOut")->toGate()->ownerModule(); //XXX crashed if there was no extSignalOut[0] gate or it was not connected --AV

  // Check if linklayer external signalling channel is connected
  if(linkLayer->gate("extSignalOut")->isConnected())
  {
    WirelessExternalSignalConnectionStatus* extSig = new WirelessExternalSignalConnectionStatus;
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
  cModule* linkLayer =  gate("extSignalOut")->toGate()->ownerModule();

  // Check if linklayer external signalling channel is connected
  if(linkLayer->gate("extSignalOut")->isConnected())
  {
    WirelessExternalSignalStats* extSig = new WirelessExternalSignalStats;
    extSig->setType(ST_STATS);
    // Need to assign power. Keep track by monitoring received frames from associated AP.
    // If not associated, then put power as invalid or low.
    extSig->setSignalStrength(associateAP.rxpower);
    extSig->setErrorPercentage(errorPercentage);
    extSig->setAvgBackoffTime(totalBackoffTime.average);
    extSig->setAvgWaitTime(totalWaitTime.average);
    send(extSig, "extSignalOut", 0);
  }
}

void WirelessEtherModule::startMonitorMode(void)
{
  associateAP.address = MAC_ADDRESS_UNSPECIFIED_STRUCT;
  associateAP.channel = INVALID_CHANNEL;
  associateAP.rxpower = INVALID_POWER;
  associateAP.associated = false;
  _currentReceiveMode = WEMonitorReceiveMode::instance();
  changeState(WirelessEtherStateIdle::instance());

  // flush input and output buffer
  if (inputFrame)
  {
    delete inputFrame;
    inputFrame = 0;
  }

  std::list<WESignalData*>::iterator oit;

  for (oit = outputBuffer.begin(); oit != outputBuffer.end(); oit++)
    delete (*oit);

  outputBuffer.clear();

  double nextSchedTime = simTime();

  // cancel all timer message except for sending the end of frames
  for (TIT it = tmrs.begin(); it != tmrs.end(); it++)
  {
    if ( (*it)->isScheduled())
    {
      if(((*it)->kind() == TRANSMIT_SENDDATA)||((*it)->kind() == WIRELESS_SELF_ENDSENDACK))
        nextSchedTime = (*it)->arrivalTime();
      else
        (*it)->cancel();
    }
  }
  Dout(dc::wireless_ethernet|flush_cf, "MAC LAYER: (WIRELESS) " << std::fixed << std::showpoint << std::setprecision(12) << simTime() << " " << fullPath() << " Monitor mode has been (re-)started, scanning channel: "<<channel);
}

void WirelessEtherModule::restartScanning(void)
{
  if(associateAP.associated)
  {
    if (gate("extSignalOut")!=NULL && gate("extSignalOut")->toGate()!=NULL)
    {
      cModule* linkLayer =  gate("extSignalOut")->toGate()->ownerModule();

      // Check if linklayer external signalling channel is connected
      if(linkLayer->gate("extSignalOut")->isConnected())
      {
        WirelessExternalSignalConnectionStatus* extSig = new WirelessExternalSignalConnectionStatus;
        extSig->setType(ST_CONNECTION_STATUS);
        extSig->setState(S_DISCONNECTED);
        send(extSig, "extSignalOut", 0);
      }
    }
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

  std::list<WESignalData*>::iterator oit;

  for (oit = outputBuffer.begin(); oit != outputBuffer.end(); oit++)
    delete (*oit);

  outputBuffer.clear();

  double nextSchedTime = simTime();

  // cancel all timer message except for sending the end of frames
  for (TIT it = tmrs.begin(); it != tmrs.end(); it++)
  {
    if ( (*it)->isScheduled())
    {
      if(((*it)->kind() == TRANSMIT_SENDDATA)||((*it)->kind() == WIRELESS_SELF_ENDSENDACK))
        nextSchedTime = (*it)->arrivalTime();
      else
        (*it)->cancel();
    }
  }

  if(activeScan)
  {
    _currentReceiveMode = WEAScanReceiveMode::instance();
    nextSchedTime += SELF_SCHEDULE_DELAY;
    cTimerMessage* tmr = getTmrMessage(TMR_PRBENERGYSCAN);
    assert(tmr && !tmr->isScheduled());
    Dout(dc::wireless_ethernet|flush_cf, "MAC LAYER: (WIRELESS) " << std::fixed << std::showpoint << std::setprecision(12) << simTime() << " " << fullPath() << " Scheduled time for active scan is: " << nextSchedTime);
    scheduleAt(nextSchedTime, tmr);
  }
  else
  {
    _currentReceiveMode = WEPScanReceiveMode::instance();
    nextSchedTime += SELF_SCHEDULE_DELAY;
    cTimerMessage* tmr = getTmrMessage(TMR_CHANNELSCAN);
    assert(tmr && !tmr->isScheduled());
    Dout(dc::wireless_ethernet|flush_cf, "MAC LAYER: (WIRELESS) " << std::fixed << std::showpoint << std::setprecision(12) << simTime() << " " << fullPath() << " Scheduled time for passive scan is: " << nextSchedTime);
    scheduleAt(nextSchedTime, tmr);
  }

}

void WirelessEtherModule::reset(void)
{
  for (TIT it = tmrs.begin(); it != tmrs.end(); it++)
  {
    if ( (*it)->isScheduled())
      (*it)->cancel();
  }

  retry = 0;

  resetContentionWindowPower();
  backoffTime = 0;

  changeState(WirelessEtherStateIdle::instance());
}

bool WirelessEtherModule::isFrameForMe(WirelessEtherBasicFrame* chkFrame)
{
  return ( chkFrame->getAddress1() == address ||
           chkFrame->getAddress1() == MACAddress6(ETH_BROADCAST_ADDRESS));
}

// Find the AP with the highest signal strength
bool WirelessEtherModule::highestPowerAPEntry(APInfo &highest)
{
  if(!tempAPList.empty())
  {
    double highestPower = (tempAPList.begin())->rxpower;
    highest = *(tempAPList.begin());

    for ( AIT it = tempAPList.begin(); it != tempAPList.end(); it++ )
    {
      if ((*it).rxpower > highestPower )
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
bool WirelessEtherModule::highestHOValueAPEntry(APInfo &highest)
{
  if(!tempAPList.empty())
  {
    double highestHOValue = (tempAPList.begin())->hOValue;
    highest = *(tempAPList.begin());

    for ( AIT it = tempAPList.begin(); it != tempAPList.end(); it++ )
    {
      if ((*it).hOValue > highestHOValue )
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
bool WirelessEtherModule::findAPEntry(APInfo &target)
{
  if(!tempAPList.empty())
  {
    for ( AIT it = tempAPList.begin(); it != tempAPList.end(); it++ )
    {
      if ((*it).address == target.address )
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

  // reset all current CSMA/CA values
  reset();

  assert(!apMode);
  assert(channel <= MAX_CHANNEL && channel >= 0);

  // find the next channel to scan
  int channelCopy = channel;
  channel = MAX_CHANNEL+1;

  for(int i=channelCopy; i<MAX_CHANNEL; i++)
  {
    channelCopy++;
    if(channelToScan[channelCopy])
    {
      channel = channelCopy;
      break;
    }
  }

  // Once new channel to scan has been decided, dont care about packets still received,
  // so count can be resetted. Could cause miss count otherwise.
  resetNoOfRxFrames();

  // all frequency bands have been scanned and now select the best AP
  //if ( channel == (MAX_CHANNEL + 1) )
  if (( channel == (MAX_CHANNEL + 1) )  || (scanShortCircuit() &&  tempAPList.size() ))  // GD Hack
   // Hack Assumes that Any access point with sufficient power will be
   // Acceptable.  If not, need to jump back into scanning (if channels left)...
   // Steve: With hack, scanning will stop as long as one probe is received. It may
   //        not have sufficient power.
  {
    bool found = false;
    // reset the channel
    channel = 0;

    //look for a specified AP if the target is valid
    if ( handoverTarget.valid )
    {
      found = findAPEntry(handoverTarget.target);
    }
    else
    {
    // search for AP with best signal strength if no target AP is specified
#if L2FUZZYHO // (Layer 2 fuzzy logic handover)
      found = highestHOValueAPEntry(handoverTarget.target);
#else
      found = highestPowerAPEntry(handoverTarget.target);
#endif // L2FUZZYHO
    }
    // only attempt to connect if a suitable AP is found
    if ( found )
    {

      // Only attempt to connect if the signal strength is above handover threshold
      if(handoverTarget.target.rxpower > hothreshpower)
      {
        associateAP = handoverTarget.target;
#if MLDV2
        cout << "handoverTarget.target.rxpower > hothreshpower, at " << simTime() << endl;
        cout << "MAC:" << associateAP.address.stringValue() << endl;

        sendGQtoUpperLayer();
#endif //MLDV2
        assert(((MAC_address)associateAP.address !=  MAC_ADDRESS_UNSPECIFIED_STRUCT) ||
               (associateAP.channel != INVALID_CHANNEL) ||
               (associateAP.rxpower != INVALID_POWER));

        //clear tempAPList
        tempAPList.clear();
        Dout(dc::wireless_ethernet|flush_cf, "MAC LAYER: (WIRELESS) "
             << OPP_Global::nodeName(this) << " \n"
             << " ---------------------------------------------------- \n"
             << " active scan COMPLETE.. Access Point found: \n"
             << " AP MAC: " << associateAP.address.stringValue() <<"\n"
             << " operating channel: " << associateAP.channel <<"\n"
             << " ---------------------------------------------------- \n");

       _currentReceiveMode = WEAuthenticationReceiveMode::instance();
        assert( _currentState == WirelessEtherStateIdle::instance());

        channel = associateAP.channel;

        // Create authentication frame and send it
        WirelessEtherBasicFrame* authentication =
          createFrame(FT_MANAGEMENT, ST_AUTHENTICATION, address,
                     associateAP.address);
        FrameBody* authFrameBody = createFrameBody(authentication);
        authentication->encapsulate(authFrameBody);
        WESignalData* authSignal = new WESignalData(authentication);
        authSignal->setChannel(channel);
        outputBuffer.push_back(authSignal);
        delete authentication;

        // Start the authentication timeout timer
        cTimerMessage* tmr = getTmrMessage(TMR_AUTHTIMEOUT);
        assert(tmr);

        if(tmr->isScheduled())
        {
          tmr->cancel();
        }
        tmr->reschedule(simTime() + (authenticationTimeout * TU));

        assert( _currentState == WirelessEtherStateIdle::instance() );

        static_cast<WirelessEtherStateIdle*>
          (_currentState)->chkOutputBuffer(this);

        return;
      }
    }

    // reset all current CSMA/CA values
    reset();

    for(WIT it = outputBuffer.begin(); it != outputBuffer.end(); it++)
      delete (*it);
    outputBuffer.clear();

    Dout(dc::wireless_ethernet|flush_cf, "MAC LAYER: (WIRELESS) " << fullPath() << " No suitable access point was found, performing active scan again.");

    double nextSchedTime = simTime() + SELF_SCHEDULE_DELAY;

    cTimerMessage* tmr = getTmrMessage(TMR_PRBENERGYSCAN);
    assert(tmr && !tmr->isScheduled());

    tmr->reschedule(nextSchedTime);

    static_cast<WirelessEtherStateIdle*>(_currentState)->chkOutputBuffer(this);

    return;
  }

  Dout(dc::wireless_ethernet|flush_cf, "MAC LAYER: (WIRELESS) " << std::fixed << std::showpoint << std::setprecision(12) << simTime() << " " << fullPath() << " scanning channel: " << channel);

  WESignalData* probeSignal = generateProbeReq();
  outputBuffer.push_back(probeSignal);

  static_cast<WirelessEtherStateIdle*>(_currentState)->chkOutputBuffer(this);
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
  channel = MAX_CHANNEL+1;

  for(int i=channelCopy; i<MAX_CHANNEL; i++)
  {
    channelCopy++;
    if(channelToScan[channelCopy])
    {
      channel = channelCopy;
      break;
    }
  }

  // Once new channel to scan has been decided, dont care about packets still received,
  // so count can be resetted. Could cause miss count otherwise.
  resetNoOfRxFrames();

  // all frequency bands have been scanned and now select the best AP
  //if ( channel == (MAX_CHANNEL + 1) )
  if (( channel == (MAX_CHANNEL + 1) )  || (scanShortCircuit() &&  tempAPList.size() ))  // GD Hack
   // Hack Assumes that Any access point with sufficient power will be
   // Acceptable.  If not, need to jump back into scanning (if channels left)...
   // Steve: With hack, scanning will stop as long as one probe is received. It may
   //        not have sufficient power.
  {
    bool found = false;
    // reset the channel
    channel = 0;

    //look for a specified AP if the target is valid
    if ( handoverTarget.valid )
    {
      found = findAPEntry(handoverTarget.target);
    }
    else
    {
    // search for AP with best signal strength if no target AP is specified
#if L2FUZZYHO // (Layer 2 fuzzy logic handover)
      found = highestHOValueAPEntry(handoverTarget.target);
#else
      found = highestPowerAPEntry(handoverTarget.target);
#endif // L2FUZZYHO
    }
    // only attempt to connect if a suitable AP is found
    if ( found )
    {

      // Only attempt to connect if the signal strength is above handover threshold
      if(handoverTarget.target.rxpower > hothreshpower)
      {
        associateAP = handoverTarget.target;

        assert(((MAC_address)associateAP.address !=  MAC_ADDRESS_UNSPECIFIED_STRUCT) ||
               (associateAP.channel != INVALID_CHANNEL) ||
               (associateAP.rxpower != INVALID_POWER));

        //clear tempAPList
        tempAPList.clear();

        Dout(dc::wireless_ethernet|flush_cf, "MAC LAYER: (WIRELESS) "
             << OPP_Global::nodeName(this) << " \n"
             << " ---------------------------------------------------- \n"
             << " Passive scan COMPLETE.. Access Point found: \n"
             << " AP MAC: " << associateAP.address.stringValue() <<"\n"
             << " operating channel: " << associateAP.channel <<"\n"
             << " ---------------------------------------------------- \n");

       _currentReceiveMode = WEAuthenticationReceiveMode::instance();
        assert( _currentState == WirelessEtherStateIdle::instance());

        channel = associateAP.channel;

        // Create authentication frame and send it
        WirelessEtherBasicFrame* authentication =
          createFrame(FT_MANAGEMENT, ST_AUTHENTICATION, address,
                     associateAP.address);
        FrameBody* authFrameBody = createFrameBody(authentication);
        authentication->encapsulate(authFrameBody);
        WESignalData* authSignal = new WESignalData(authentication);
        authSignal->setChannel(channel);
        outputBuffer.push_back(authSignal);
        delete authentication;

        // Start the authentication timeout timer
        cTimerMessage* tmr = getTmrMessage(TMR_AUTHTIMEOUT);
        assert(tmr);

        if(tmr->isScheduled())
        {
          tmr->cancel();
        }
        tmr->reschedule(simTime() + (authenticationTimeout * TU));

        assert( _currentState == WirelessEtherStateIdle::instance() );

        static_cast<WirelessEtherStateIdle*>
          (_currentState)->chkOutputBuffer(this);

        return;
      }
    }

    // reset all current CSMA/CA values
    reset();

    for(WIT it = outputBuffer.begin(); it != outputBuffer.end(); it++)
      delete (*it);
    outputBuffer.clear();

    Dout(dc::wireless_ethernet|flush_cf, "MAC LAYER: (WIRELESS) " << fullPath() << " No suitable access point was found, performing passive scan again.");
    double nextSchedTime = simTime() + SELF_SCHEDULE_DELAY;

    cTimerMessage* tmr = getTmrMessage(TMR_CHANNELSCAN);
    assert(tmr && !tmr->isScheduled());

    tmr->reschedule(nextSchedTime);

    static_cast<WirelessEtherStateIdle*>(_currentState)->chkOutputBuffer(this);

    return;
  }

  Dout(dc::wireless_ethernet|flush_cf, "MAC LAYER: (WIRELESS) " << std::fixed << std::showpoint << std::setprecision(12) << simTime() << " " << fullPath() << " passive scanning channel: " << channel);

  double nextSchedTime = simTime() + channelScanTime;

  cTimerMessage* tmr = getTmrMessage(TMR_CHANNELSCAN);
  assert(tmr && !tmr->isScheduled());

  tmr->reschedule(nextSchedTime);

  static_cast<WirelessEtherStateIdle*>(_currentState)->chkOutputBuffer(this);
}

void WirelessEtherModule::authTimeoutHandler(void)
{
  Dout(dc::wireless_ethernet|flush_cf, "MAC LAYER: (WIRELESS) " << std::fixed << std::showpoint << std::setprecision(12) << simTime() << " " << fullPath() << " Authentication timed out by: " << address);
  restartScanning();
}

void WirelessEtherModule::assTimeoutHandler(void)
{
  Dout(dc::wireless_ethernet|flush_cf, "MAC LAYER: (WIRELESS) " << std::fixed << std::showpoint << std::setprecision(12) << simTime() << " " << fullPath() << " Association timed out by: " << address);
  restartScanning();
}

void WirelessEtherModule::printSelfMsg(const cMessage* msg)
{
  const char* state;
  int messageID = msg->kind();

  if ( currentState() == WirelessEtherStateIdle::instance())
    state = "IDLE";
  else if ( currentState() == WirelessEtherStateSend::instance())
    state = "SEND";
  else if ( currentState() == WirelessEtherStateBackoff::instance())
    state = "BACKOFF";
  else if ( currentState() == WirelessEtherStateAwaitACK::instance())
    state = "AWAITACK";
  else if ( currentState() == WirelessEtherStateAwaitACKReceive::instance())
    state = "AWAITACKRECEIVE";
  else if ( currentState() == WirelessEtherStateBackoffReceive::instance())
    state = "BACKOFFRECEIVE";
  else if ( currentState() == WirelessEtherStateReceive::instance())
    state = "RECEIVE";

  else
    assert(false);

  string message;

  if(messageID == TRANSMIT_SENDDATA)
    message = "TRANSMIT_SENDDATA ( " + string(msg->name()) + string(" )");
  else if(messageID == TMR_PRBENERGYSCAN)
    message = "TMR_PRBENERGYSCAN ( " + string(msg->name()) + string(" )");
  else if(messageID == TMR_PRBRESPSCAN)
    message = "TMR_PRBRESPSCAN ( " + string(msg->name()) + string(" )");
  else if(messageID == TMR_CHANNELSCAN)
    message = "TMR_CHANNELSCAN ( " + string(msg->name()) + string(" )");
  else if(messageID == TMR_BEACON)
    message = "TMR_BEACON ( " + string(msg->name()) + string(" )");
  else if(messageID == TMR_REMOVEENTRY)
    message = "TMR_REMOVEENTRY ( " + string(msg->name()) + string(" )");
  else if(messageID == TMR_AUTHTIMEOUT)
    message = "TMR_AUTHTIMEOUT ( " + string(msg->name()) + string(" )");
  else if(messageID == TMR_ASSTIMEOUT)
    message = "TMR_ASSTIMEOUT ( " + string(msg->name()) + string(" )");
  else if(messageID == TMR_STATS)
    message = "TMR_STATS ( " + string(msg->name()) + string(" )");
  else if(messageID == WIRELESS_SELF_AWAITMAC)
    message = "WIRELESS_SELF_AWAITMAC ( " + string(msg->name()) + string(" )");
  else if(messageID == WIRELESS_SELF_BACKOFF)
    message = "WIRELESS_SELF_BACKOFF ( " + string(msg->name()) + string(" )");
  else if(messageID == WIRELESS_SELF_AWAITACK)
    message = "WIRELESS_SELF_AWAITACK ( " + string(msg->name()) + string(" )");
  else if(messageID == WIRELESS_SELF_ENDSENDACK)
    message = "WIRELESS_SELF_ENDSENDACK ( " + string(msg->name()) + string(" )");
  else
    assert(false); // notify any new message added

  Dout(dc::wireless_ethernet|flush_cf, "MAC LAYER: (WIRELESS) " << fullPath() << " receiving self message with ID: " << message.c_str() << " in " << state << " state");
}

void WirelessEtherModule::sendToUpperLayer(WirelessEtherBasicFrame* frame)
{
  cMessage *dgram = frame->decapsulate();
  send(dgram, inputQueueOutGate());
}

// Sends monitored frame to upper layer un-modified
void WirelessEtherModule::sendMonitorFrameToUpperLayer(WESignalData* sig)
{
  Dout(dc::wireless_ethernet|flush_cf, "MAC LAYER: (WIRELESS) " << std::fixed << std::showpoint << std::setprecision(12) << simTime() << " " << fullPath() << "Sending monitor frame to upper layer.");

  send(sig->dup(), inputQueueOutGate());
}

#if MLDV2
void WirelessEtherModule::sendGQtoUpperLayer()
{
  MLDv2Message *GQmsg= new MLDv2Message(ICMPv6_MLD_QUERY,20);

  cout << endl << OPP_Global::findNetNodeModule(this)->name() << " sendGQtoUpperLayer(), at simTime:" << simTime() << endl;
//  cout << "_NMAR:" << LStable->NMAR() << endl;

  GQmsg->setLength(28);

  GQmsg->setMaxRspCode(1000);//query response interval
  GQmsg->setMA(c_ipv6_addr(0));
  GQmsg->setS_Flag(false);
  GQmsg->setQRV(2);
  GQmsg->setQQIC(125);
  GQmsg->setNS(0);

  IPv6Datagram* dgram = new IPv6Datagram;

  dgram->encapsulate(GQmsg);
  dgram->setPayloadLength(GQmsg->length());
  dgram->setDestAddress(c_ipv6_addr("FF02:0:0:0:0:0:0:1"));
  dgram->setSrcAddress(c_ipv6_addr("0:0:0:0:0:0:0:1"));
  dgram->setTransportProtocol(IP_PROT_IPv6_ICMP);

  send(dgram, inputQueueOutGate());
}
#endif

WESignalData* WirelessEtherModule::generateProbeReq(void)
{
  WirelessEtherBasicFrame* probeFrame =
    createFrame(FT_MANAGEMENT, ST_PROBEREQUEST, address,
                MACAddress6(ETH_BROADCAST_ADDRESS));
  FrameBody* probeFrameBody = createFrameBody(probeFrame);

  probeFrame->encapsulate(probeFrameBody);

  WESignalData* probeSignal = new WESignalData(probeFrame);
  probeSignal->setChannel(channel);
  delete probeFrame;

  return probeSignal;
}

WirelessEtherBasicFrame* WirelessEtherModule::
createFrame(FrameType frameType, SubType subType,
            MACAddress6 source, MACAddress6 destination)
{
  WirelessEtherBasicFrame *frame;
  FrameControl frameControl;
  DurationID durationID;
  SequenceControl sequenceControl;

  switch(frameType)
  {
    case FT_CONTROL:
      switch(subType)
      {
        case ST_ACK:
          frame = new WirelessEtherBasicFrame;

          frameControl.protocolVer = 0;
          frameControl.type = frameType;
          frameControl.subtype = subType;
          frameControl.toDS = false;
          frameControl.fromDS = false;
          frameControl.retry = false;

          static_cast<WirelessEtherBasicFrame*>(frame)->
            setFrameControl(frameControl);

          //always 0 since moreFrag not implemented
          durationID.bit15 = false;
          durationID.bit14 = false;
          durationID.bit14to0 = 0;

          static_cast<WirelessEtherBasicFrame*>(frame)->
            setDurationID(durationID);

          assert(inputFrame->data());
          static_cast<WirelessEtherBasicFrame*>(frame)->
            setAddress1(static_cast<WirelessEtherRTSFrame*>(inputFrame->data())->getAddress2());

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
      break;*/

    case FT_MANAGEMENT:
      frame = new WirelessEtherManagementFrame;

      frameControl.protocolVer = 0;
      frameControl.type = frameType;
      frameControl.subtype = subType;
      frameControl.toDS = false;
      frameControl.fromDS = false;
      (retry > 0) ? frameControl.retry = true : frameControl.retry = false;

      frame->setLength(FL_FRAMECTRL + FL_DURATIONID + FL_ADDR1 +
                       FL_ADDR2 + FL_ADDR3 + FL_SEQCTRL + FL_FCS);

      static_cast<WirelessEtherManagementFrame*>(frame)->
        setFrameControl(frameControl);

      durationID.bit15 = false;

      // need to determine ACK + SIFS time check for multicast not
      // just broadcast as well; note also that having no moreFrag
      // support, eliminates other potential values for duration
      (destination == MACAddress6(ETH_BROADCAST_ADDRESS)) ?
        durationID.bit14to0 = 0 :
        durationID.bit14to0 = (unsigned)((ACKLENGTH/BASE_SPEED)+SIFS)*1000; //add ACK

      static_cast<WirelessEtherManagementFrame*>(frame)->
        setDurationID(durationID);
      static_cast<WirelessEtherManagementFrame*>(frame)->
        setAddress1(destination);
      static_cast<WirelessEtherManagementFrame*>(frame)->
        setAddress2(address);

      if(subType == ST_PROBEREQUEST)
      {
        static_cast<WirelessEtherManagementFrame*>(frame)->
          setAddress3(destination);
      }
      else
      {
        (apMode == true) ?
          static_cast<WirelessEtherManagementFrame*>(frame)->
          setAddress3(address) :
          static_cast<WirelessEtherManagementFrame*>(frame)->
          setAddress3(associateAP.address);
      }

      sequenceControl.fragmentNumber = 0;
      sequenceControl.sequenceNumber = sequenceNumber;
      static_cast<WirelessEtherManagementFrame*>(frame)->
        setSequenceControl(sequenceControl);

      frame->setLength(FL_FRAMECTRL + FL_DURATIONID + FL_ADDR1 +
                       FL_ADDR2 + FL_ADDR3 + FL_SEQCTRL + FL_FCS);
      break;

    case FT_DATA:
      frame = new WirelessEtherDataFrame;

      frameControl.protocolVer = 0;
      frameControl.type = FT_DATA;
      frameControl.subtype = ST_DATA;    //since of data type

      if(apMode == true)
      {
        frameControl.toDS = false;
        frameControl.fromDS = true;
        static_cast<WirelessEtherDataFrame*>(frame)->setAddress1(destination);
        static_cast<WirelessEtherDataFrame*>(frame)->setAddress2(address);
        static_cast<WirelessEtherDataFrame*>(frame)->setAddress3(source);
      }
      else
      {
        frameControl.toDS = true;
        frameControl.fromDS = false;
        static_cast<WirelessEtherDataFrame*>(frame)->
          setAddress1(associateAP.address);
        static_cast<WirelessEtherDataFrame*>(frame)->
          setAddress2(source);
        static_cast<WirelessEtherDataFrame*>(frame)->
          setAddress3(destination);
      }
      (retry > 0) ? frameControl.retry = true : frameControl.retry = false;
      static_cast<WirelessEtherDataFrame*>(frame)->setFrameControl(frameControl);

      durationID.bit15 = false;
      // need to determine ACK + SIFS time
      // must check for multicast not just broadcast
      // note also that having no moreFrag support, eliminates other
      // potential values for duration
      (destination == MACAddress6(ETH_BROADCAST_ADDRESS)) ?
        durationID.bit14to0 = 0 :
        durationID.bit14to0 = (unsigned)((ACKLENGTH/BASE_SPEED)+SIFS)*1000; //add ACK

      static_cast<WirelessEtherDataFrame*>(frame)->setDurationID(durationID);
      sequenceControl.fragmentNumber = 0; //fragmentation not supported
      sequenceControl.sequenceNumber = sequenceNumber;

      //need to decide how to keep track of sequence in module
      static_cast<WirelessEtherDataFrame*>(frame)->
        setSequenceControl(sequenceControl);

      frame->setLength(FL_FRAMECTRL + FL_DURATIONID + FL_ADDR1 + FL_ADDR2 +
                       FL_ADDR3 + FL_ADDR4 + FL_SEQCTRL + FL_FCS);
      break;

    default:
      assert(false);
      break;
  }
  //XXX frame->setProtocol(PR_WETHERNET);

  return frame;
}

FrameBody* WirelessEtherModule::createFrameBody(WirelessEtherBasicFrame* f)
{
  FrameBody* frameBody;
  CapabilityInformation capabilityInfo;
  unsigned int i;

  switch(f->getFrameControl().subtype)
  {
    case ST_ASSOCIATIONREQUEST:
      frameBody = new AssociationRequestFrameBody;

      capabilityInfo.ESS = !adhocMode;
      capabilityInfo.IBSS = adhocMode;
      capabilityInfo.CFPollable = false; //CF not implemented
      capabilityInfo.CFPollRequest = false;
      capabilityInfo.privacy = false; //wep not implemented

      static_cast<AssociationRequestFrameBody*>(frameBody)->
        setCapabilityInformation(capabilityInfo);
      static_cast<AssociationRequestFrameBody*>(frameBody)->
        setSSID(ssid.c_str());
      static_cast<AssociationRequestFrameBody*>(frameBody)->
        setSupportedRatesArraySize(rates.size());

      for (i = 0; i < rates.size(); i++)
        static_cast<AssociationRequestFrameBody*>(frameBody)->
          setSupportedRates(i, rates[i]);

      frameBody->setLength(FL_CAPINFO + FL_IEHEADER + ssid.length() +
                           FL_IEHEADER + rates.size());
      break;

    case ST_REASSOCIATIONREQUEST:
      frameBody = new ReAssociationRequestFrameBody;

      static_cast<ReAssociationRequestFrameBody*>(frameBody)->
        setCurrentAP(associateAP.address);

      capabilityInfo.ESS = !adhocMode;
      capabilityInfo.IBSS = adhocMode;
      capabilityInfo.CFPollable = false; //CF not implemented
      capabilityInfo.CFPollRequest = false;
      capabilityInfo.privacy = false; //wep not implemented

      static_cast<ReAssociationRequestFrameBody*>(frameBody)->
        setCapabilityInformation(capabilityInfo);
      static_cast<ReAssociationRequestFrameBody*>(frameBody)->
        setSSID(ssid.c_str());
      static_cast<ReAssociationRequestFrameBody*>(frameBody)->
        setSupportedRatesArraySize(rates.size());

      for (i = 0; i < rates.size(); i++)
        static_cast<ReAssociationRequestFrameBody*>(frameBody)->
          setSupportedRates(i, rates[i]);

      frameBody->setLength(FL_CAPINFO + FL_CURRENTAP + FL_IEHEADER +
                           ssid.length() + FL_IEHEADER + rates.size());
      break;

    case ST_PROBEREQUEST:
      frameBody = new ProbeRequestFrameBody;

      static_cast<ProbeRequestFrameBody*>(frameBody)->
        setSSID(ssid.c_str());
      static_cast<ProbeRequestFrameBody*>(frameBody)->
        setSupportedRatesArraySize(rates.size());

      for (i = 0; i < rates.size(); i++)
        static_cast<ProbeRequestFrameBody*>(frameBody)->
          setSupportedRates(i, rates[i]);

      frameBody->setLength(FL_IEHEADER + ssid.length() + FL_IEHEADER +
                           rates.size());
      break;

    case ST_AUTHENTICATION:
      frameBody = new AuthenticationFrameBody;

      //Only supports "open mode", therefore only two sequence number
      if(apMode == true)
        static_cast<AuthenticationFrameBody*>(frameBody)->setSequenceNumber(2);
      else
        static_cast<AuthenticationFrameBody*>(frameBody)->setSequenceNumber(1);

      // TODO: successful for now, but how are we going to represent
      // in module?
      static_cast<AuthenticationFrameBody*>(frameBody)->setStatusCode(0);

      frameBody->setLength(FL_STATUSCODE + FL_SEQNUM);
      break;

    default:
      assert(false);
      break;
  }
  return frameBody;
}

void WirelessEtherModule::decodeFrame(WESignalData* signal)
{
  _currentReceiveMode->decodeFrame(this, signal);
}

bool WirelessEtherModule::isProbeReq(WESignalData* signal)
{
  WirelessEtherBasicFrame* frame = (signal->data());

  if ( frame->getFrameControl().subtype == ST_PROBEREQUEST )
    return true;

  return false;
}

void WirelessEtherModule::initialiseChannelToScan(void)
{
  // Initialise all channels for scanning except channel 0
  channelToScan = new bool[MAX_CHANNEL+1];
  channelToScan[0]=false;
  for(int i=1; i <= MAX_CHANNEL; i++)
  {
    channelToScan[i]=true;
  }

  // Mark unwanted channels based on XML input
  StringTokenizer tokenizer(chanNotToScan.c_str(),"-");
  const char *token;
  while ((token = tokenizer.nextToken())!=NULL)
     channelToScan[atoi(token)]=false;

/* XXX replaced with the code above --Andras
  boost::tokenizer<boost::char_separator<char> >::iterator tokenIt;
  boost::char_separator<char> sep("-");
  boost::tokenizer<boost::char_separator<char> > tokens(chanNotToScan, sep);

  for (tokenIt = tokens.begin(); tokenIt != tokens.end(); tokenIt++)
  {
    channelToScan[atoi(tokenIt->c_str())]=false;
  }
*/
}

void WirelessEtherModule::insertToAPList(APInfo newEntry)
{
  // If short circuit hack is turned on, ensure tempAPList only contains entries
  // which are "good enough"(i.e. above handover threshold power)
  if(!scanShortCircuit() || (scanShortCircuit() && (newEntry.rxpower > hothreshpower)))
  {
    for ( AIT it = tempAPList.begin(); it != tempAPList.end(); it++ )
    {
      // Overwrite old entry.
      if ( (it->address == newEntry.address)&&(it->channel == newEntry.channel) )
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

// Transfer frames to stored in the offlineBuffer to the outputBuffer
void WirelessEtherModule::makeOfflineBufferAvailable(void)
{
  WirelessEtherBasicFrame* frame;

  while(!offlineOutputBuffer.empty())
  {
    frame = offlineOutputBuffer.front();
    // The offline output buffer only gets filled with frames encapsulating
    // data from upper layers.  It is safe to assume that they all have
    // FT_DATA type. Therefore, we can assume address1 is used for the AP
    // address.
    frame->setAddress1(associateAP.address);

    WESignalData* a = new WESignalData(frame);
    outputBuffer.push_back(a);
    offlineOutputBuffer.pop_front();
  }
}

void WirelessEtherModule::updateStats(void)
{
  //Calculate throughput average and error percentage after interval time
  throughput.average = throughput.sampleTotal/throughput.sampleTime;
  errorPercentage = (noOfFailedTx+noOfSuccessfulTx)>0 ? noOfFailedTx/(noOfFailedTx+noOfSuccessfulTx):0;
  totalWaitTime.average = totalWaitTime.sampleTotal/totalWaitTime.sampleTime;
  totalBackoffTime.average = (noOfSuccessfulTx > 0) ? totalBackoffTime.sampleTotal/noOfSuccessfulTx : totalBackoffTime.sampleTotal;

  if(statsVec)
  {
    noOfFailedTxVec->record(noOfFailedTx);
    throughputVec->record(throughput.average);
    errorPercentageVec->record(errorPercentage);
    totalBackoffTimeVec->record(totalBackoffTime.average);
    totalWaitTimeVec->record(totalWaitTime.average);
  }

  noOfFailedTx = 0;
  noOfSuccessfulTx = 0;
  throughput.sampleTotal = 0;
  totalWaitTime.sampleTotal = 0;
  totalBackoffTime.sampleTotal = 0;

  scheduleAt(simTime()+throughput.sampleTime, updateStatsNotifier);
}

#if L2FUZZYHO // (Layer 2 fuzzy logic handover)
double WirelessEtherModule::calculateHOValue(double rxpower, double ap_avail_bw, double bw_req)
{
    double n_rxpower, n_ap_avail_bw, n_bw_req;
    double value;
    hodec fuzSys;

    n_rxpower = (rxpower - hothreshpower)/(-10 - hothreshpower); //assuming -10 is highest power in dB
    n_ap_avail_bw = (ap_avail_bw/11); //assuming max avail bw is 11Mb/s
    n_bw_req = bw_req;

    if(n_rxpower < 0)
        n_rxpower = 0;
    fuzSys.inference(n_bw_req,n_rxpower,n_ap_avail_bw,&value);

    return value;
}
#endif // L2FUZZYHO

