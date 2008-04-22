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
 * P.59 state transitions occur only within the caller. They notify the callee via
 * the pointer passed in. Simple constant buffer implemented so far.
 *
 */

#include <cassert>

#include "RTPVoip.h"
#include "UDPControlInfo_m.h"
#include "IPv6Address.h"
#include "opp_utils.h" //nodename
#include "IPv6Utils.h"
//#include "NotificationBoard.h"
//#include "NotifierConsts.h"
#include <boost/bind.hpp>
#include "TimerConstants.h"
#include <boost/circular_buffer.hpp>
#include <algorithm>
#include <numeric> //accumulate
#include <boost/tuple/tuple_comparison.hpp>
#include <boost/tuple/tuple_io.hpp>
#include <stlwatch.h>

typedef std::vector<IPvXAddress> AddressVector;
using IPv6Utils::printRoutingInfo;

bool caller(cMessage* p59cb)
{
  return p59cb != 0;
}

Define_Module(RTPVoip);

///For non omnetpp csimplemodule derived classes
RTPVoip::RTPVoip():p59cb(0),talkStatesVector(0),stStat(0),dtStat(0), msStat(0),
                   tsStat(0),callerPause(0), calleePause(0),callerPauseStat(0),
                   calleePauseStat(0),playoutTimer(0),networkDelay(0),cb(0), 
		   lastPlayedFrame(boost::make_tuple(0.0, 0)), discarded(false),
		   emeanDelay(0),elastReceived(0),elastExpected(0),emodelTimer(0),
		   erfactorVector(0)
{
}

RTPVoip::~RTPVoip()
{
 if (p59cb && p59cb->isScheduled())
  {
    p59cb->cancel();
    delete p59cb;
  }
 delete cb;
 cb = 0;
}

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

  packetisationInterval = frameLength * (double)framesPerPacket;

  //ned param for constant buffer
  jitterDelay = 0.1;
  //or jitter buffer in terms of number of packets stored
  //jitterDelay = packetsBuffered/packetspersecond = packetsBuffered* packetisationInterval
  //buffer an extra packet that way packets will not be discarded due to buffer too full
  cb = new JitterBuffer((unsigned int )(jitterDelay/(framesPerPacket * frameLength)) * 2);
  assert(cb->empty());
  assert(!cb->full());
  WATCH_RINGBUFFER(*cb);
  //assert(jitterDelay/(framesPerPacket * frameLength) == cb->capacity());

  EV<<"voip: "<<OPP_Global::nodeName(this)<<" buffer capacity="<<cb->capacity()<<endl;
  std::ostream& os = printRoutingInfo(true, 0, 0, true);
  os<<"voip: "<<OPP_Global::nodeName(this)<<" buffer capacity="<<cb->capacity()<<endl;
  rtcpBw = (double)par("rtcpBandwidth"); 
  if (rtcpBw < 1)
    rtcpBw = rtcpBw * (double)bitrate * 8.0;

  Ie = par("Ie").doubleValue();
  Bpl = par("Bpl").doubleValue();
  lookahead = par("lookahead").doubleValue();
  
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

  ecalculateRFactor();

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
  if (caller(p59cb))
  {
    stStat->recordScalar((std::string("ST time of ") + OPP_Global::nodeName(this)).c_str());
    dtStat->recordScalar((std::string("DT time of ") + OPP_Global::nodeName(this)).c_str());  
    msStat->recordScalar((std::string("MS time of ") + OPP_Global::nodeName(this)).c_str());  
    tsStat->recordScalar((std::string("TS time of ") + OPP_Global::nodeName(this)).c_str());
    callerPauseStat->recordScalar((std::string("caller pause of ") + OPP_Global::nodeName(this)).c_str());
    calleePauseStat->recordScalar((std::string("callee pause of ") + OPP_Global::nodeName(this)).c_str());
  }
}

//virtual bool sendRTPPacket();
//need frame message type in packet since buffering implies putting multples
//"frames" per packet and sometimes the timestamps are not consecuitve due to
//losses at wired part


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

  emodelTimer->rescheduleDelay(10);
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

  ///TODO ned param for period?
  emodelTimer->rescheduleDelay(10);
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

void RTPVoip::attachData(RTPPacket* rtpData)
{
  //use payloadLength to indicate whether this includes more than
  //framesperpacket in case zfa used. hence cp can either be a Frame
  //or an array of Frames.
  rtpData->setContextPointer(new VoipFrame(boost::make_tuple(rtpData->timestamp(), rtpData->seqNo())));
}

void RTPVoip::processRTPData(RTPPacket* rtpData, RTPMemberEntry &rme)
{
  bool talkspurtBegin = false;
  simtime_t now = simTime();
  std::ostream& os = printRoutingInfo(true, 0, 0, true);

  if (!playoutTimer || !playoutTimer->isScheduled())
  {
    if (!playoutTimer)
    {
      playoutTimer = new cCallbackMessage("playout");
      (*playoutTimer) = boost::bind(&RTPVoip::playoutBufferedPacket, this);
      playoutTimer->setContextPointer(&rme);
    }
    talkspurtBegin = true;
  }
  //sendingTime is same as last playoutTime as we only schedule
  //playoutTimer from its callback fn or later on below
  else if (playoutTimer->sendingTime() < now - networkDelay - jitterDelay)
  {
    talkspurtBegin = true;
  }

  if (talkspurtBegin)
  {
    //even though constant jitter buffer we adjust mean network delay based on
    //first packet in talkspurt
    networkDelay = now - rtpData->timestamp();

    EV<<"voip: "<<OPP_Global::nodeName(this)<<":"<<simTime()<<" new talkSpurt "
      <<networkDelay<<"s baseline"<<endl;
    os<<"voip: "<<OPP_Global::nodeName(this)<<":"<<simTime()<<" new talkSpurt "
      <<networkDelay<<"s baseline"<<endl;

    playoutTimer->rescheduleDelay(jitterDelay);
    assert(playoutTimer->sendingTime() == now);
  }

  VoipFrame* frames = 0;
  frames = (VoipFrame*) rtpData->contextPointer();

  unsigned int frameCount = 1;
  if (rtpData->payloadLength() > payloadLength)
  {
    frameCount = (unsigned int)( rtpData->payloadLength()/ payloadLength);
    assert(frameCount > 1);
  }

  for (unsigned int i = 0; i < frameCount; i++)
  {
    VoipFrame& thisFrame = frames[i];

    os<<"voiptracerec:"<<OPP_Global::nodeName(this)<<"\t"<<simTime()<<"\t"
      <<thisFrame.get<0>()<<"\t"<<thisFrame.get<1>()<<endl;

  if (playoutTime(thisFrame.get<0>()) < playoutTimer->arrivalTime())
  {
    EV<<"voip: "<<OPP_Global::nodeName(this)<<":"<<simTime()<<" discarded packet deadline="
      <<playoutTimer->arrivalTime()<<" incoming rtp timestamp="<<thisFrame.get<0>()
      <<" playoutTime="<<playoutTime(thisFrame.get<0>())<<endl;
    os<<"voip: "<<OPP_Global::nodeName(this)<<":"<<simTime()<<" discarded packet deadline="
      <<playoutTimer->arrivalTime()<<" incoming rtp timestamp="<<thisFrame.get<0>()
      <<" playoutTime="<<playoutTime(thisFrame.get<0>())<<endl;
    VoipFrame& discard = thisFrame;

    compareToPreviousVoipFrame(discard);

    db.push_back(thisFrame.get<1>());
    if (discarded)
      //if previous event was a discard then continue this trend
      elossEvents.back() += 1;
    else
      elossEvents.push_back(1);
    discarded = true;
    delete &thisFrame;
    continue;
  }

  if (cb->full())
  {
    std::sort(cb->begin(), cb->end());

    VoipFrame& discard = cb->front();
    simtime_t nextPlayoutTime = playoutTime(discard.get<0>());
    assert(nextPlayoutTime > now);

    EV<<"voip: "<<OPP_Global::nodeName(this)<<":"<<simTime()<<" discarded packet "
      <<discard<<" as jitter buffer full "<<cb->size()<<endl;
    os<<"voip: "<<OPP_Global::nodeName(this)<<":"<<simTime()<<" discarded packet "
      <<discard<<" as jitter buffer full ("<<cb->size()<<endl;

    compareToPreviousVoipFrame(discard);

    db.push_back(discard.get<1>());
    if (discarded)
      //if previous event was a discard then continue this trend
      elossEvents.back() += 1;
    else
      elossEvents.push_back(1);
    discarded = true;
    cb->push_back(thisFrame);
    delete &thisFrame;
    continue;
  }
  cb->push_back(thisFrame);
  discarded = false;
  } //end for loop

  std::sort(cb->begin(), cb->end());
  //doing this makes it never play back for much longer especially when
  //buffers are full. Perhaps should have buffer size twice jitter delay?
  //if (discarded)
  //  playoutTimer->reschedule(playoutTime(cb->front().get<0>()));
  if (frameCount > 1)
    delete [] frames;
}

simtime_t RTPVoip::playoutTime(simtime_t timestamp)
{
  return timestamp + networkDelay + jitterDelay;
}

#define RTP_SEQ_MOD (1<<16)

void RTPVoip::recordLosses(RTPMemberEntry& rme, u_int16 udelta)
{
  const int MAX_MISORDER = 100;
  const int MAX_DROPOUT = 3000;
   bool misordered = false;
  if (udelta > MAX_DROPOUT && udelta > RTP_SEQ_MOD - MAX_MISORDER)
    misordered = true;
  if (!misordered && udelta < MAX_DROPOUT)
  {
    initialiseStats(&rme, this);
    rme.lossVector->record(udelta);
    elossEvents.push_back(udelta);
  }
}

void RTPVoip::compareToPreviousVoipFrame(const VoipFrame& thisFrame)
{
  ///compare to previous playout time
  if (!discarded && (thisFrame.get<1>() - lastPlayedFrame.get<1>() > 1))
  {
    //assuming 2-party conf not a conference otherwise buff needs to store the
    //full rtp packet for its ssrc
    RTPMemberEntry& rme = *(static_cast<RTPMemberEntry*>(playoutTimer->contextPointer()));
    u_int16 udelta = thisFrame.get<1>() - lastPlayedFrame.get<1>();
    recordLosses(rme, udelta);
  }
  else if (discarded)
  {
    //compare to previous discarded packet (should have used ints for seqNo that
    //way never enter if branch anyway and handles misordered packets automatically)

    if (thisFrame.get<1>()-db.back() > 1)
    {
      RTPMemberEntry& rme = *(static_cast<RTPMemberEntry*>(playoutTimer->contextPointer()));
      //prevent misordered packets from generating packet losses
      //unsigned int = abs((long int) (thisFrame.get<1>() - db.back())) - 1;
      u_int16 udelta = thisFrame.get<1>() - db.back();
      recordLosses(rme, udelta);

    }
  }
}

void RTPVoip::playoutBufferedPacket()
{
  assert(!cb->empty());

  simtime_t now = simTime();
  std::sort(cb->begin(), cb->end());
  EV<<"voip: "<<OPP_Global::nodeName(this)<<":"<<now<<" play back samples timestamp="
    <<cb->front()<<endl;
  std::ostream& os = printRoutingInfo(true, 0, 0, true);

  VoipFrame thisFrame = cb->front();

  os<<"voiptraceplay:"<<OPP_Global::nodeName(this)<<"\t"<<simTime()<<"\t"
    <<thisFrame.get<0>()<<"\t"<<thisFrame.get<1>()<<endl;

  //ignore discarded packets that arrive too late
  ecalculateMeanTotalDelay(thisFrame.get<0>());

  compareToPreviousVoipFrame(thisFrame);

  discarded = false;
  cb->pop_front();
  if (cb->empty())
  {
    os<<"voip: "<<OPP_Global::nodeName(this)<<":"<<now<<" silence starting or packets dropped\n";
  }
  else
  {
    simtime_t nextPlayoutTime = playoutTime(cb->front().get<0>());
    playoutTimer->reschedule(nextPlayoutTime);
    os<<" difference in times? nextplayoutDelay="<<nextPlayoutTime-now
      <<" packetisationInterval="<< packetisationInterval<<endl;
    //playoutTimer->rescheduleDelay(packetisationInterval);
  }
  lastPlayedFrame = thisFrame;
}

void RTPVoip::handleMisorderedOrDroppedPackets(RTPMemberEntry *s,
						       u_int16 udelta)
{
  //we don't detect drops this way at all let jitter buffer handle and record  
}


double RTPVoip::ecalculateRFactor()
{
  double rfactor = 93.2;

  if (!erfactorVector)
  {
    erfactorVector = new cOutVector((std::string("Rfactor of ") + OPP_Global::nodeName(this)).c_str());
    emodelTimer = new cCallbackMessage("rfactorcalc");
    (*emodelTimer) = boost::bind(&RTPVoip::ecalculateRFactor, this);
    return 0;
  }

  emodelTimer->rescheduleDelay(10);
  RTPMemberEntry& rme = *(static_cast<RTPMemberEntry*>(playoutTimer->contextPointer()));
  unsigned int expected = rme.cycles + rme.maxSeq - elastExpected;
  unsigned int received = rme.received - elastReceived;

  double Id = 0;
  //convert to ms as required by e-model
  emeanDelay*=1000.0;
  //just do simplified Idd for now
  if (emeanDelay < 177.3)
    Id = 0.024 * emeanDelay;
  else
    Id = 0.024 * emeanDelay + 0.11*(emeanDelay - 177.3);

  //work out Ie
  double p = 0, q =  0;
  if (received > expected)
  {
    cerr<<"voip: "<<OPP_Global::nodeName(this)<<":"<<simTime()<<" received > expected="
	<< received <<"/" << expected<<endl;
  }

  if (elossEvents.size())
  {
    p = (double)elossEvents.size()/received;
    q = (double)elossEvents.size()/std::accumulate(elossEvents.begin(), elossEvents.end(), 0);
  }
  //double burstR = 1.0/(p+q);
  //alternative below in case elastReceived is bogus as may count duplicated packets
  double ppl = (double)std::accumulate(elossEvents.begin(), elossEvents.end(), 0)/expected;

  if (ppl != 1-((double)received/expected))
    cout<<"voip: "<<OPP_Global::nodeName(this)<<":"<<simTime()<<" mismatch ppl from lost/expected="<<ppl
	<<" from 1 - (received/expect)="<<1.0-(double)received/expected<<endl;
  double Ppl = 100.0*ppl; //in percentage points 
  double burstR = (1.0 - ppl)*((double)std::accumulate(elossEvents.begin(), elossEvents.end(), 0)/(double)elossEvents.size());
  if (p + q != 0 && burstR != (1.0/(p+q)))
    cout<<"voip: "<<OPP_Global::nodeName(this)<<":"<<simTime()<<"mismatch (1-ppl)e(k) burstR="
	<<burstR<<" whilst using 1/(p+q) burstR="<<(1.0/(p+q))<<endl;
  double Ieff = Ie + (95.0 - Ie)*(Ppl/((Ppl/burstR)+Bpl));

  rfactor+= -Id - Ieff;
  
  erfactorVector->record(rfactor);
  
  emeanDelay = 0;
  elossEvents.clear();
  elastReceived = rme.received;
  elastExpected = rme.cycles + rme.maxSeq;
  return rfactor;
}

///timestamp is at sending end
void RTPVoip::ecalculateMeanTotalDelay(simtime_t timestamp)
{
  //codec Delay includes both encoder/decoder
  //minimum according to G.114 Table I.4
  double codecDelay = frameLength * ((double)framesPerPacket + 1.0) + lookahead;
  //maximum according to G.114 Table I.4
  //codecDelay = frameLength * (2.0*(double)framesPerPacket + 1.0) + par("lookahead");

  double playoutDelay = simTime() - timestamp;
  emeanDelay = (emeanDelay + playoutDelay + codecDelay)/2;
}
