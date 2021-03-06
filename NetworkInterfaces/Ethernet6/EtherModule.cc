//
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
  @file EtherModule.cc
  @brief Implementation of EtherModule based on "Efficient and
  Accurate Ethernet Simulation" by Jia Wang and Srinivasan Keshav

  @author Eric Wu
*/

#include <sys.h> // Dout
#include "debug.h" // Dout

#include <memory>
#include <boost/cast.hpp>
#include <boost/bind.hpp>
#include <memory>
#include <cmath> //std::pow for random.hpp

#include "EtherModule.h"

#include "EtherState.h"
#include "EtherStateIdle.h"
#include "EtherStateReceive.h"
#include "EtherStateSend.h"
#include "EtherStateWaitBackoff.h"
#include "EtherStateWaitBackoffJam.h"
#include "EtherStateWaitJam.h"
#include "EtherStateReceiveWaitBackoff.h"
#include "EtherSignal_m.h"
#include "EtherFrame6.h"
#include "LL6ControlInfo_m.h"
#include "InterfaceTableAccess.h"
#include "cCallbackMessage.h"

#include "opp_utils.h"
#include <string>

Define_Module(EtherModule);

void EtherModule::initialize(int stage)
{
  if (stage == 0)
  {
    frameCollided = false;

    LinkLayerModule::initialize();
    backoffRemainingTime = 0;

    setIface_name(PR_ETHERNET);
    iface_type = PR_ETHERNET;

    changeState(EtherStateIdle::instance());

    procdelay = par("procdelay").longValue();

    std::string parAddr = par("address").stringValue();
    if (!parAddr.empty())
      _myAddr.set(parAddr.c_str());
    //Only want to initialise the MN mac address once otherwise routing tables
    //are wrong if we generate another random number from the stream
    else if (_myAddr == MACAddress6())
    {
      MAC_address addr;
      addr.high = OPP_Global::generateInterfaceId() & 0xFFFFFF;
      addr.low = OPP_Global::generateInterfaceId() & 0xFFFFFF;
      _myAddr.set(addr);
      par("address") = _myAddr.stringValue();
    }
    statsVec = par("recordStatisticVector").boolValue();

    inputFrame = 0;
    retry = 0;
    interframeGap = 0;

    inGate = findGate("physIn");
    outGate = findGate("physOut");

    registerInterface();

    //Initialise Variables for Statistics
    statsUpdatePeriod = 1;
    RxDataBWStat = 0;
    TxDataBWStat = 0;
    noOfRxStat = 0;
    noOfTxStat = 0;

    RxDataBWVec = new cOutVector("RxDataBWVec");
    TxDataBWVec = new cOutVector("TxDataBWVec");
    noOfRxVec = new cOutVector("noOfRxVec");
    noOfTxVec = new cOutVector("noOfTxVec");

    if ( statsVec )
    {
      InstRxFrameSizeVec = new cOutVector("InstRxFrameSizeVec");
      InstTxFrameSizeVec = new cOutVector("InstTxFrameSizeVec");
    }
  }
  else if (stage == 1)
  {
    cModule* phyLayer = OPP_Global::findModuleByName(this, "phyLayer");
    assert(phyLayer);

    if ( std::string(phyLayer->par("PHYName").stringValue()) != "PHYSimple")
    {
      opp_warning("PHYSimple NOT FOUND!");
    }

    // Timer to update statistics
    updateStatsNotifier  =
      new cCallbackMessage("EMupdateStats", TMR_ETH_STATS);
    *((cCallbackMessage*)updateStatsNotifier) = boost::bind(
      &EtherModule::updateStats, this);
    scheduleAt(simTime()+statsUpdatePeriod, updateStatsNotifier);
  }
}

void EtherModule::handleMessage(cMessage* msg)
{
  assert(msg);

  if ( !msg->isSelfMessage())
  {
    ++cntReceivedPackets;
    _currentState->processSignal(this, std::auto_ptr<cMessage>(msg));
  }
  else
  {
    printSelfMsg(msg->kind());
    static_cast<cTimerMessage*>(msg)->callFunc();
  }
}

InterfaceEntry *EtherModule::registerInterface()
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
  std::string tmp = std::string("eth")+OPP_Global::ltostr(parentModule()->index());
  e->setName(tmp.c_str()); // XXX HACK -- change back to above code!

  e->_linkMod = this; // XXX remove _linkMod on the long term!! --AV

  // port: index of gate where parent module's "netwIn" is connected (in IP)
  int outputPort = parentModule()->gate("netwIn")->fromGate()->index();
  e->setOutputPort(outputPort);
printf("DBG: %s as '%s' on %d\n", fullPath().c_str(), e->name(), outputPort);

  // generate a link-layer address to be used as interface token for IPv6
  unsigned int iid[2];
  iid[0] = ( _myAddr.intValue()[0] << 8 ) | 0xFF;
  iid[1] = _myAddr.intValue()[1] | 0xFE000000;
  InterfaceToken token(iid[1], iid[0], 64);
  e->setInterfaceToken(token);

  // MAC address as string
  e->setLLAddrStr(_myAddr.stringValue());

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

void EtherModule::reset(void)
{
  Dout(dc::ethernet|flush_cf, "MAC LAYER: " << fullPath() << " reset");

  retry = 0;

  cancelAllTmrMessages();

  for(FIT it = outputBuffer.begin(); it != outputBuffer.end(); it++)
    delete (*it);
  outputBuffer.clear();

  for(TIT it = tmrs.begin(); it != tmrs.end(); it++)
    delete (*it);
  tmrs.clear();

  if (inputFrame)
    delete inputFrame;

  _currentState = EtherStateIdle::instance();
}

void EtherModule::finish()
{
  delete RxDataBWVec;
  delete TxDataBWVec;
  delete noOfRxVec;
  delete noOfTxVec;

}

EtherModule::~EtherModule()
{
  for(FIT it = outputBuffer.begin(); it != outputBuffer.end(); it++)
    delete (*it);
  outputBuffer.clear();

  for(TIT it = tmrs.begin(); it != tmrs.end(); it++)
    if (!(*it)->isScheduled())
    {
      delete (*it);
    }
    else
    {
      (*it)->cancel();
      delete (*it);
    }
  tmrs.clear();
  if (updateStatsNotifier && updateStatsNotifier->isScheduled())
    updateStatsNotifier->cancel();
  delete updateStatsNotifier;
  if (inputFrame)
    delete inputFrame; // XXX will crash if initialize hasn't run for some reason

}

bool EtherModule::sendFrame(cMessage *msg, int gateid)
{
  send(msg, gateid);

  return true;
}

void EtherModule::idleNetworkInterface(void)
{
  Dout(dc::ethernet|flush_cf, "MAC LAYER: " << fullPath() << " is ready to accept L3 packets");
}

bool EtherModule::sendData(EtherFrame6* frame) //XXX this is actually for passing up packets
{
  MACAddress6 frameDestAddr = frame->destAddress();
  std::string strFrameDestAddr = (const char*)frameDestAddr;

  // TODO: LL passes the data to the upper layer if the
  // destination address of the frame is the same as the node's
  // address OR the frame is a broadcast frame ie
  // FF:FF:FF:FF:FF:FF HOWEVER there are predefined multicast
  // addresses ranged from 33:00:00:00:00:00 to 33:FF:FF:FF:FF:FF
  // specifically for IPv6 Neighbour Dicovery, we may have to
  // investigate on that...
  if (strFrameDestAddr == _myAddr.stringValue() ||  // XXX why string comparison...
      strFrameDestAddr == ETH_BROADCAST_ADDRESS)
  {
    cMessage* packet = frame->decapsulate();
    ev << "Passing up packet " << packet << "\n";
    send(packet, inputQueueOutGate());
  }
  else
  {
    ev << "Frame " << frame << ": dest address doesn't match, discarding\n";
  }
  delete frame;

  Dout(dc::debug|flush_cf, OPP_Global::nodeName(this) << "Ethernet HostMacAddr= "
       << _myAddr.stringValue() << "  FrameDestAddr= " << strFrameDestAddr);

  return true;
}

bool EtherModule::receiveData(std::auto_ptr<cMessage> msg) //XXX this is actually for queueing up outgoing packets
{
  // Something to send onto network
  ev << "Received " << msg.get() << " from upper layers for transmission\n";

  LL6ControlInfo *ctrlInfo = check_and_cast<LL6ControlInfo*>(msg->removeControlInfo());
  EtherFrame6* frame = new EtherFrame6(msg->name());
  frame->setSrcAddress(MACAddress6(_myAddr.stringValue()));
  frame->setDestAddress(MACAddress6(ctrlInfo->getDestLLAddr()));
  delete ctrlInfo;
  frame->setProtocol(PR_ETHERNET);
  frame->encapsulate(msg.release());

  EtherSignalData* sigData = new EtherSignalData(frame->name());
  sigData->encapsulate(frame);
  sigData->setSrcModPathName(fullPath().c_str());

  outputBuffer.push_back(sigData);

  if ( _currentState == EtherStateIdle::instance())
  {
    cTimerMessage* tmrMessage = getTmrMessage(SELF_INTERFRAMEGAP);
    if (!(tmrMessage && tmrMessage->isScheduled()))
      static_cast<EtherStateIdle*>(_currentState)->chkOutputBuffer(this);
  }

  return true;
}

void EtherModule::addTmrMessage(cTimerMessage* msg)
{
  tmrs.push_back(msg);
}

void EtherModule::incNumOfRxJam(std::string srcModPathName)
{
  for (SIT it = jams.begin(); it != jams.end(); it++)
    if ( (*it) == srcModPathName)
      return;

  jams.push_back(srcModPathName);
}

void EtherModule::incNumOfRxIdle(std::string srcModPathName)
{
  for (SIT it = idles.begin(); it != idles.end(); it++)
    if ( (*it) == srcModPathName)
      return;

  idles.push_back(srcModPathName);
}

void EtherModule::decNumOfRxIdle(std::string srcModPathName)
{
  for (SIT it = idles.begin(); it != idles.end(); it++)
    if ( (*it) == srcModPathName)
    {
      it = idles.erase(it);
      it--;
    }
}

void EtherModule::decNumOfRxJam(std::string srcModPathName)
{
  for (SIT it = jams.begin(); it != jams.end(); it++)
    if ( (*it) == srcModPathName)
    {
      it = jams.erase(it);
      it--;
    }
}

bool EtherModule::isMediumBusy(void)
{
  if (!jams.size() && !idles.size())
    return false;
  return true;
}

cTimerMessage* EtherModule::getTmrMessage(const int& messageID)
{
  for (TIT it = tmrs.begin(); it != tmrs.end(); it++)
  {
    if ((*it)->kind() == messageID)
      return (*it);
  }
  return 0;
}

void EtherModule::changeState(EtherState* state)
{
  _currentState = state;
}

void EtherModule::cancelAllTmrMessages(void)
{
  for (TIT it = tmrs.begin(); it != tmrs.end(); it++)
    if ( (*it)->isScheduled())
      (*it)->cancel();
}

void EtherModule::printSelfMsg(const int messageID)
{
  const char* state;

  if ( currentState() == EtherStateIdle::instance())
    state = "IDLE";
  else if ( currentState() == EtherStateSend::instance())
    state = "SEND";
  else if ( currentState() == EtherStateReceive::instance())
    state = "RECEIVE";
  else if ( currentState() == EtherStateWaitBackoff::instance())
    state = "WAITBACKOFF";
  else if ( currentState() == EtherStateWaitBackoffJam::instance())
    state = "WAITBACKOFFJAM";
  else if ( currentState() == EtherStateWaitJam::instance())
    state = "WAITJAM";
  else if ( currentState() == EtherStateReceiveWaitBackoff::instance())
    state = "RECEIVEWAITBACKOFF";
  else
    assert(false);


  const char* message;

  if(messageID == TRANSMIT_SENDDATA)
    message = "TRANSMIT_SENDDATA";
  else if ( messageID == TRANSMIT_JAM )
    message = "TRANSMIT_JAM";
  else if ( messageID == SELF_BACKOFF )
    message = "SELF_BACKOFF";
  else if ( messageID == SELF_INTERFRAMEGAP )
    message = "SELF_INTERFRAMEGAP";
  else if ( messageID == TMR_ETH_STATS )
    message = "TMR_ETH_STATS";
  else
    assert(false); // notify any new message added

  Dout(dc::ethernet|flush_cf, "MAC LAYER: " << fullPath() << " receiving self message with ID: " << message << " in " << state << " state");
}

void EtherModule::updateStats(void)
{
  //record vectors
  if(statsVec)
  {
    RxDataBWVec->record(RxDataBWStat/statsUpdatePeriod);
    TxDataBWVec->record(TxDataBWStat/statsUpdatePeriod);
    noOfRxVec->record(noOfRxStat/statsUpdatePeriod);
    noOfTxVec->record(noOfTxStat/statsUpdatePeriod);
  }

  //reset stats updated every period
  RxDataBWStat = 0;
  TxDataBWStat = 0;
  noOfRxStat = 0;
  noOfTxStat = 0;

  //reschedule next update
  scheduleAt(simTime() + statsUpdatePeriod, updateStatsNotifier);
}
