// -*- C++ -*-
//
// Eric Wu
// Copyright (C) 2001 Monash University, Melbourne, Australia
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
    @file EtherModule.h
    @brief Header file for EtherModule

    Simple implementation of Ethernet module

    @author Eric Wu
*/

#ifndef __ETHER_MODULE_H__
#define __ETHER_MODULE_H__

#include <memory> //auto_ptr

#ifndef BOOST_CAST_HPP
#include <boost/cast.hpp>
#endif //BOOST_CAST_HPP

#include <omnetpp.h>
#include <list>
#include <map>

#include "LinkLayerModule.h"
#include "ethernet.h"
#include "MACAddress6.h"
#include "cTimerMessage.h"
#include "cTimerMessageCB.h"
#include <string>

class EtherState;
class EtherSignal;
class EtherSignalData;
class EtherFrame6;
class InterfaceEntry;

/**
 * FIXME documentation!
 */
class EtherModule: public LinkLayerModule
{
 public:
  Module_Class_Members(EtherModule, LinkLayerModule, 0);
  virtual void initialize(int stage);
  virtual void handleMessage(cMessage* msg);
  virtual void finish();
  virtual ~EtherModule();

  virtual int numInitStages(void) const { return 2; }

  // adds interface entry into InterfaceTable
  InterfaceEntry *registerInterface();

  // send packet to network
  virtual bool sendFrame(cMessage *msg, int gateid);

  // receive packet from other layer besides physical layer
  virtual bool receiveData(std::auto_ptr<cMessage> msg);

  // send packet to other layer besides physical layer
  virtual bool sendData(EtherFrame6* frame);

  // reset back to initial state
  void reset(void);

  // attributes

  void idleNetworkInterface(void);

  long procDelay(void) { return procdelay; }

  std::list<EtherSignalData*> outputBuffer;
  EtherSignalData* inputFrame; // one frame slot buffer

  int inPHYGate() { return inGate; }
  int outPHYGate() { return outGate; }

  simtime_t backoffRemainingTime;

  // input gate of the Output Queue for incoming packet from other layer or peer L2 modules
  virtual int outputQueueInGate() { return findGate("netwIn"); }

  // output gate of the Input Queue to other layer or peer L2 modules
  virtual int inputQueueOutGate() { return findGate("netwOut"); }

  // retrasnmission backoff attributes
  void incrementRetry(void) { retry++; }
  unsigned int getRetry(void) const { return retry; }
  void resetRetry(void) { retry = 0; }

  // CSMA-CD

  bool frameCollided;

  EtherState* currentState()
    {
      return _currentState;
    }

  void changeState(EtherState* state);

  // self timer mssages

  void addTmrMessage(cTimerMessage* msg);
  cTimerMessage* getTmrMessage(const int& messageID);

  void cancelAllTmrMessages(void);

  void incNumOfRxJam(std::string srcModPathName);
  void decNumOfRxJam(std::string srcModPathName);
  void incNumOfRxIdle(std::string srcModPathName);
  void decNumOfRxIdle(std::string srcModPathName);

  bool isMediumBusy(void);

  simtime_t interframeGap;

  /********* STATISTICS VARIABLES (start) ************/

  bool statsVec;
  Loki::cTimerMessageCB<void>* updateStatsNotifier;
  virtual void updateStats(void);

  double statsUpdatePeriod; //seconds

  double RxDataBWStat;        //received data bandwidth (Mbit)
  double TxDataBWStat;        //transmitted data bandwidth (Mbit)
  double noOfRxStat;          //number of received data frames (frames)
  double noOfTxStat;          //number of transmitted data frames (frames)
  
  //vector of statistics every second
  cOutVector* RxDataBWVec;        //(Mbit/s)
  cOutVector* TxDataBWVec;        //(Mbit/s)
  cOutVector* noOfRxVec;          //(frames/s)
  cOutVector* noOfTxVec;          //(frames/s)

  //vector of instantaneaous readings
  cOutVector* InstRxFrameSizeVec; //(bytes) successful ones
  cOutVector* InstTxFrameSizeVec; //(bytes) successful ones

  /********* STATISTICS VARIABLES (end) ************/
 protected:
  EtherState* _currentState;
  long procdelay; // ms
  MACAddress6 _myAddr;
  std::list<cTimerMessage*> tmrs;
  std::list<std::string> jams;
  std::list<std::string> idles;

  // PHY links
  int inGate; // input from network
  int outGate; // output to network

  unsigned int retry;
private:
  // debug
  void printSelfMsg(const int messageID);
};

typedef std::list<EtherSignalData*>::iterator FIT;
typedef std::list<cTimerMessage*>::iterator TIT;
typedef std::list<std::string>::iterator SIT;

#endif

