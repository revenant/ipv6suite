// -*- C++ -*-
// Copyright (C) 2008 Johnny Lai
//
// This file is part of IPv6Suite
//
// IPv6Suite is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// IPv6Suite is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.


/**
 * @file   RTPVoip.cc
 * @author Johnny Lai
 * @date   16 Apr 2008
 *
 * @brief  Implementation of RTPVoip
 *
 * @todo
 *
 */

#include <cassert>

#include "RTPVoip.h"
#include "UDPControlInfo_m.h"
#include "IPv6Address.h"
#include "opp_utils.h" //nodename
//#include "NotificationBoard.h"
//#include "NotifierConsts.h"
#include <boost/bind.hpp>
#include "TimerConstants.h"

typedef std::vector<IPvXAddress> AddressVector;

Define_Module(RTPVoip);

int RTPVoip::numInitStages() const
{
  return 4;
}

void RTPVoip::initialize(int stageNo)
{
  RTP::initialize(stageNo);

  if (stageNo != 3)
    return;

  frameLength = (double)par("frameLength");
  if (frameLength > 0.1)
    opp_error("frame length i.e. Tp is less than 100ms for sure");

  bitrate =  par("bitrate").longValue();
  if (bitrate > 64000)
    opp_error("bitrates are usually less than G.711's 64k, bitrate request was %d", bitrate);

  framesPerPacket = par("framesPerPacket").longValue();
  if (framesPerPacket < 1 || framesPerPacket > 100)
    opp_error("Invalid framesPerPacket specified %d, it should be less than 100", framesPerPacket);

  payloadLength = (double)bitrate/(1.0/(frameLength * (double)framesPerPacket ))/8;
  if (floor(payloadLength) != payloadLength)
  {
    opp_error("Frational amounts when tyring to determine payload length %d, is this normal?", payloadLength);
  }

  //minimum according to G.114 Table I.4
  packetisationInterval = frameLength * ((double)framesPerPacket + 1.0) + par("lookahead").doubleValue();
  //maximum according to G.114 Table I.4
  //packetisationInterval = frameLength * (2.0*(double)framesPerPacket + 1.0) + par("lookahead");

  rtcpBw = (double)par("rtcpBandwidth"); 
  if (rtcpBw < 1)
    rtcpBw = rtcpBw * (double)bitrate * 8.0;

  simtime_t startTime = par("startTime");
  if (startTime)
  {
    p59cb = new cCallbackMessage("p.59");
    talkStatesVector =
      new cOutVector((std::string("talkState of ") + OPP_Global::nodeName(this)).c_str());
    stStat = new cStdDev((std::string("ST duration of ") + OPP_Global::nodeName(this)).c_str());
    msStat = new cStdDev((std::string("MS duration of ") + OPP_Global::nodeName(this)).c_str());
    dtStat = new cStdDev((std::string("DT duration of ") + OPP_Global::nodeName(this)).c_str());
    tsStat = new cStdDev((std::string("TS duration of ") + OPP_Global::nodeName(this)).c_str());
    callerPauseStat = new cStdDev((std::string("caller pause duration of ") + OPP_Global::nodeName(this)).c_str());
    calleePauseStat = new cStdDev((std::string("callee pause duration of ") + OPP_Global::nodeName(this)).c_str());
  }

 simtime_t stopTime = par("stopTime");
 if (stopTime > 0)
  {
    cCallbackMessage* quitApp = new cCallbackMessage("quitApp leak");
    (*quitApp) = boost::bind(&RTPVoip::leaveSession, this);  
    scheduleAt(stopTime, quitApp);
  }
}


void RTPVoip::finish()
{
  RTP::finish();
  if (p59cb)
  {
    stStat->recordScalar((std::string("ST time of ") + OPP_Global::nodeName(this)).c_str());
    dtStat->recordScalar((std::string("DT time of ") + OPP_Global::nodeName(this)).c_str());  
    msStat->recordScalar((std::string("MS time of ") + OPP_Global::nodeName(this)).c_str());  
    tsStat->recordScalar((std::string("TS time of ") + OPP_Global::nodeName(this)).c_str());
    callerPauseStat->recordScalar((std::string("caller pause of ") + OPP_Global::nodeName(this)).c_str());
    calleePauseStat->recordScalar((std::string("callee pause of ") + OPP_Global::nodeName(this)).c_str());
  }
}

///For non omnetpp csimplemodule derived classes
RTPVoip::RTPVoip():p59cb(0),talkStatesVector(0),stStat(0),dtStat(0), msStat(0),
                   tsStat(0),callerPause(0), calleePause(0),callerPauseStat(0),
                   calleePauseStat(0)
{
}

RTPVoip::~RTPVoip()
{
 if (p59cb && p59cb->isScheduled())
  {
    p59cb->cancel();
    delete p59cb;
  }
}

///next four are different rng streams
const static int stateTrans = 4;
const static int x1 = 1;
const static int x2 = 2;
const static int x3 = 3;

const static double P1 = 0.4;
const static double P2 = 0.5;
const static double P3 = 0.5;

void RTPVoip::P59TS(RTP* callee)
{
  assert(!p59cb->isScheduled());
  talkStatesVector->record(1);

  double timeInstate = -0.854 * log(1-uniform(0,1,x1));
  tsStat->collect(timeInstate);

  //get caller to start a talk spurt
  if (!this->rtpTimeout->isScheduled())
    scheduleAt(simTime() + packetisationInterval, this->rtpTimeout);

  if (callerPause != 0)
    callerPauseStat->collect(callerPause);

  callerPause = 0;
  calleePause += timeInstate;

  //get callee to stop talking

  //check which state we transition to now
  if (uniform(0,1,stateTrans) > P1)
    (*p59cb)=boost::bind(&RTPVoip::P59MS, this, callee);
  else
    (*p59cb)=boost::bind(&RTPVoip::P59DT, this, callee);

  p59cb->rescheduleDelay(timeInstate);

  OPP_Global::ContextSwitcher switchContext(callee);
  if (callee->rtpTimeout->isScheduled())
    callee->cancelEvent(callee->rtpTimeout);
}

void RTPVoip::P59DT(RTP* callee)
{
  assert(!p59cb->isScheduled());
  talkStatesVector->record(4);

  //get  both to talk
  if (!this->rtpTimeout->isScheduled())
    scheduleAt(simTime() + packetisationInterval, this->rtpTimeout);

  if (callerPause != 0)
    callerPauseStat->collect(callerPause);    
  callerPause = 0;
  if (calleePause != 0)
    calleePauseStat->collect(calleePause);
  calleePause = 0;

  double timeInstate = -0.226 * log(1-uniform(0,1,x2));
  dtStat->collect(timeInstate);

  //check which state we transition to now
  if (uniform(0,1,stateTrans) > P3)
    (*p59cb)=boost::bind(&RTPVoip::P59TS, this, callee);
  else
    (*p59cb)=boost::bind(&RTPVoip::P59ST, this, callee);

  p59cb->rescheduleDelay(timeInstate);

  OPP_Global::ContextSwitcher switchContext(callee);
  if (!callee->rtpTimeout->isScheduled())
    callee->scheduleAt(simTime() + packetisationInterval, callee->rtpTimeout);
}

void RTPVoip::P59ST(RTP* callee)
{
  assert(!p59cb->isScheduled());
  talkStatesVector->record(2);

  //get callee to talk and caller to shut up
  if (this->rtpTimeout->isScheduled())
    cancelEvent(this->rtpTimeout);

  double timeInstate = -0.854 * log(1-uniform(0,1,x1));
  stStat->collect(timeInstate);

  callerPause += timeInstate;
  if (calleePause != 0)
    calleePauseStat->collect(calleePause);
  calleePause = 0;

  //check which state we transition to now
  if (uniform(0,1,stateTrans) > P1)
    (*p59cb)=boost::bind(&RTPVoip::P59MS, this, callee);
  else
    (*p59cb)=boost::bind(&RTPVoip::P59DT, this, callee);

  p59cb->rescheduleDelay(timeInstate);

  OPP_Global::ContextSwitcher switchContext(callee);
  if (!callee->rtpTimeout->isScheduled())
    callee->scheduleAt(simTime() + packetisationInterval, callee->rtpTimeout);
}

void RTPVoip::P59MS(RTP* callee)
{
  assert(!p59cb->isScheduled());
  talkStatesVector->record(3);

  if (this->rtpTimeout->isScheduled())
    cancelEvent(this->rtpTimeout);

  double timeInstate = -0.456 * log(1-uniform(0,1,x3));
  msStat->collect(timeInstate);

  callerPause += timeInstate;
  calleePause += timeInstate;

  if (uniform(0,1,stateTrans) > P2)
    (*p59cb)=boost::bind(&RTPVoip::P59TS, this, callee);
  else
    (*p59cb)=boost::bind(&RTPVoip::P59ST, this, callee);

  p59cb->rescheduleDelay(timeInstate);

  OPP_Global::ContextSwitcher switchContext(callee);
  if (callee->rtpTimeout->isScheduled())
    callee->cancelEvent(callee->rtpTimeout);
}

void RTPVoip::processSDES(RTCPSDES* sdes)
{
  UDPControlInfo *ctrl = check_and_cast<UDPControlInfo *>(sdes->controlInfo());
  IPvXAddress srcAddr = ctrl->getSrcAddr();

  //voip conversation model requires passing of callee RTP module for 2
  //party on/off state switching
  RTPVoip* caller = static_cast<RTPVoip*>(sdes->contextPointer());
  assert(caller->p59cb);
  rtpTimeout = new cMessage("callee rtpTimeout");
  destAddrs.push_back(srcAddr);
  (*(caller->p59cb)) =  boost::bind(&RTPVoip::P59TS, caller, this);
  OPP_Global::ContextSwitcher switchContext(caller);
  caller->p59cb->rescheduleDelay(SELF_SCHEDULE_DELAY);
}

void RTPVoip::processGoodBye(RTCPGoodBye* rtcp)
{
  //if bye received remove entry from members/senders map 
  //ignore reverse reconsideration 6.3.4 (code in pg 93)
  if (memberSet.count(rtcp->ssrc()))
  {
    UDPControlInfo *ctrl = check_and_cast<UDPControlInfo *>(rtcp->controlInfo());
    IPvXAddress srcAddr = ctrl->getSrcAddr();

    //Don't want to remove from member set otherwise our stats are gone.  guess
    //we could move to another set    
    AddressVector::iterator ait =
      std::find(destAddrs.begin(), destAddrs.end(),
		memberSet[rtcp->ssrc()].addr);

    if (ait != destAddrs.end())
    {
      destAddrs.erase(ait);
      //send bye to other end to remove ourselves from members list	
      RTCPPacket* bye = new RTCPGoodBye(_ssrc);
      sendToUDP(bye, port+1, srcAddr, port+1);
    }
  }
}


void RTPVoip::establishSession()
{
  if (destAddrs.empty())
  {
    resolveAddresses();
  }

  if (destAddrs.empty())
    return;

  //voip app 2 party conversation 
  RTCPSDES* sdes = new RTCPSDES(_ssrc, OPP_Global::nodeName(this));
  sdes->setContextPointer(this);
  sendToUDP(sdes, port+1, destAddrs[0], port+1);    
}

void RTPVoip::leaveSession()
{
  RTP::leaveSession();

  //cancel voice model
  if (p59cb->isScheduled())
    p59cb->cancel();
}
