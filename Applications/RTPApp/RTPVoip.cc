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
#include "IPAddressResolver.h"
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
using OPP_Global::relativeDiff;

static const double RTPTimeEpsilon  = 0.000001;

bool caller(cMessage* p59cb)
{
  return p59cb != 0;
}

unsigned int frameCountCalc(unsigned int payloadLength, RTPPacket* rtpData);

Define_Module(RTPVoip);

///For non omnetpp csimplemodule derived classes
RTPVoip::RTPVoip():p59cb(0),talkStatesVector(0),stStat(0),dtStat(0), msStat(0),
                   tsStat(0),callerPause(0), calleePause(0),callerPauseStat(0),
                   calleePauseStat(0),playoutTimer(0),networkDelay(0),cb(0), 
		   lastPlayedFrame(boost::make_tuple(0.0, 0)), discarded(false),
		   emeanDelay(0),elastReceived(0),elastExpected(0),elossEventsCount(0),
		   elostPackets(0), emodelTimer(0),
		   erfactorVector(0), totalDelayVector(0), totalDelayStat(0)
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
    payloadLength = ceil(payloadLength);
    //opp_error("Frational amounts when tyring to determine payload length %d, is this normal?", payloadLength);
  }

  packetisationInterval = frameLength * (double)framesPerPacket;

  //ned param for constant buffer
  jitterDelay = par("jitterDelay");
  //or jitter buffer in terms of number of packets stored
  //jitterDelay = packetsBuffered/packetspersecond = packetsBuffered* packetisationInterval
  //buffer an extra packet that way packets will not be discarded due to buffer too full
  cb = new JitterBuffer((unsigned int )(jitterDelay/(framesPerPacket * frameLength)) * 2);
  assert(cb->empty());
  assert(!cb->full());
  WATCH_RINGBUFFER(*cb);
  const int Misordered_Max = 50;
  misordered = new JitterBuffer(Misordered_Max);
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

  simtime_t stopTime = par("stopTime");
  if (stopTime > 0)
  {
    cCallbackMessage* quitApp = new cCallbackMessage("quitApp leak");
    (*quitApp) = boost::bind(&RTPVoip::leaveSession, this);  
    scheduleAt(stopTime, quitApp);
  }
}

//from rfactor.cc and verified same as approx. answer in R
double Rfactor(double emeanDelay, double Bpl, double Ie, unsigned int received, 
	       unsigned int expected, unsigned int elossEventsSum, unsigned int elossEvents)
{
  emeanDelay*=1000.0;
  double  Id = 0;
  //just do simplified Idd for now
  if (emeanDelay < 177.3)
    Id = 0.024 * emeanDelay;
  else
    Id = 0.024 * emeanDelay + 0.11*(emeanDelay - 177.3);
  double p = 0, q =  0;

  if (elossEvents > 0)
  {
    if (received != 0)
      p = (double) elossEvents/received;
    q = (double) elossEvents/ elossEventsSum;
  }

  double ppl = (double)elossEventsSum/expected;
  double burstR = 1.0;

  if (ppl > 0)
    burstR = (1.0 - ppl)*((double) elossEventsSum /(double)elossEvents);

  double Ppl = 100.0*ppl; //in percentage points 

  double Ieff = Ie ;
  if (Ppl > 0)
    Ieff += (95.0 - Ie)*(Ppl/((Ppl/burstR)+Bpl));

  double rfactor = 93.2;
  rfactor+= -Id - Ieff;

  std::ostream& os = printRoutingInfo(true, 0, 0, true);
  os<<"emeanDelay="<< emeanDelay<<" bpl="<< Bpl<<" Ie="<< Ie<<" received="<<received
	   <<" expected="<<expected<<" lossEventsSum="<<(double) elossEventsSum
    <<" lossEventsCount="<<(double)elossEvents<<" Ppl="<<Ppl<<" rfactor="<<rfactor<<endl;
  cout<<"emeanDelay="<< emeanDelay<<" bpl="<< Bpl<<" Ie="<< Ie<<" received="<<received
	   <<" expected="<<expected<<" lossEventsSum="<<(double) elossEventsSum
	   <<" lossEventsCount="<<(double)elossEvents<<" Ppl="<<Ppl<<" rfactor="<<rfactor<<endl;
  return rfactor;
}

void RTPVoip::finish()
{
  //update the counters one last time otherwise counters have old values :(
  ecalculateRFactor();

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
  //as first seq no received starts at 0
  elastExpected++;
  RTPMemberEntry& rme = *(static_cast<RTPMemberEntry*>(playoutTimer->contextPointer()));
  totalDelayStat->recordScalar((std::string("totalDelay from ") + IPAddressResolver().hostname(rme.addr)).c_str());
  recordScalar((std::string("elostPackets from ") + IPAddressResolver().hostname(rme.addr)).c_str(), elostPackets);
  recordScalar((std::string("elossEvents from ") + IPAddressResolver().hostname(rme.addr)).c_str(), elossEventsCount);
  recordScalar((std::string("ePpl from ")  + IPAddressResolver().hostname(rme.addr)).c_str(), (double)elostPackets/elastExpected * 100.0);
  //elastReceived is 0 anyway arg is not used in calc
  recordScalar((std::string("eRfactor from ")  + IPAddressResolver().hostname(rme.addr)).c_str(), 
	       Rfactor(totalDelayStat->mean(), Bpl, Ie, elastReceived,  elastExpected, elostPackets, elossEventsCount));
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
      //initialise timers
      ecalculateRFactor();      
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

  VoipFrame* frames = (VoipFrame*) rtpData->contextPointer();

  unsigned int frameCount = frameCountCalc((unsigned int)payloadLength, rtpData);

  for (unsigned int i = 0; i < frameCount; i++)
  {
    VoipFrame& thisFrame = frames[i];

    os<<"voiptracerec:"<<OPP_Global::nodeName(this)<<"\t"<<simTime()<<"\t"
      <<thisFrame.get<0>()<<"\t"<<thisFrame.get<1>()<<"\n";

  if (playoutTime(thisFrame.get<0>()) < playoutTimer->arrivalTime())
  {
    EV<<"voip: "<<OPP_Global::nodeName(this)<<":"<<simTime()<<" discarded packet deadline="
      <<playoutTimer->arrivalTime()<<" incoming rtp timestamp="<<thisFrame.get<0>()
      <<" playoutTime="<<playoutTime(thisFrame.get<0>())<<"\n";
    os<<"voip: "<<OPP_Global::nodeName(this)<<":"<<simTime()<<" discarded packet deadline="
      <<playoutTimer->arrivalTime()<<" incoming rtp timestamp="<<thisFrame.get<0>()
      <<" playoutTime="<<playoutTime(thisFrame.get<0>())<<"\n";

    //VoipFrame& discard = thisFrame;
    //compareToPreviousVoipFrame(discard);

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
      <<discard<<" as jitter buffer full "<<cb->size()<<"\n";
    os<<"voip: "<<OPP_Global::nodeName(this)<<":"<<simTime()<<" discarded packet "
      <<discard<<" as jitter buffer full ("<<cb->size()<<"\n";

//    compareToPreviousVoipFrame(discard);

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
  delete &thisFrame;
  discarded = false;
  elastReceived++;
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

void RTPVoip::handleDroppedPackets(RTPMemberEntry *s, u_int16 udelta)
{
  //record seq dropped (delete this record or update after packets arrive out of
  //order). record into vec file at emodel calc time and clear. emodel does not
  //need to know when dropped just how many dropped at a particular loss event

  RTP::handleDroppedPackets(s, udelta);
  u_int16 origSeq = udelta + s->maxSeq;
  DiscardBuffer newrange;
  for (unsigned int start = s->maxSeq + 1; start != origSeq; start++)
    newrange.push_back(start);
  probableDropped.push_back(newrange);
}

/* repeating what was in update_seq and getting confused between
 * misordered and dropped packets and discarded packets too

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

*/

void RTPVoip::playoutBufferedPacket()
{
  assert(!cb->empty());

  simtime_t now = simTime();
  std::sort(cb->begin(), cb->end());
  EV<<"voip: "<<OPP_Global::nodeName(this)<<":"<<now<<" play back samples timestamp="
    <<cb->front()<<"\n";
  std::ostream& os = printRoutingInfo(true, 0, 0, true);

  VoipFrame thisFrame = cb->front();

  os<<"voiptraceplay:"<<OPP_Global::nodeName(this)<<"\t"<<simTime()<<"\t"
    <<thisFrame.get<0>()<<"\t"<<thisFrame.get<1>()<<"\n";

  //ignore discarded packets that arrive too late
  ecalculateMeanTotalDelay(thisFrame.get<0>());

  //compareToPreviousVoipFrame(thisFrame);

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
    if (relativeDiff(nextPlayoutTime - now, packetisationInterval) > RTPTimeEpsilon)
      os<<"voip: "<<OPP_Global::nodeName(this)<<":"<<now<<" difftimes? nextplayoutDelay="<<nextPlayoutTime-now
	<<" timestamp="<<cb->front().get<0>()<<" packetisationInterval="<< packetisationInterval<<endl;
    //playoutTimer->rescheduleDelay(packetisationInterval);
  }
  lastPlayedFrame = thisFrame;
}

unsigned int frameCountCalc(unsigned int payloadLength, RTPPacket* rtpData)
{
  unsigned int frameCount = 1;
   if (rtpData->payloadLength() > payloadLength)
   {
     frameCount = (unsigned int)( rtpData->payloadLength()/ payloadLength);
     assert(frameCount > 1);
   }
   return frameCount;
}

void RTPVoip::handleMisorderedPackets(RTPMemberEntry *s, RTPPacket* rtpData)
{
  std::ostream& os = printRoutingInfo(true, 0, 0, true);
  if (std::find(misordered->begin(), misordered->end(), boost::make_tuple(rtpData->timestamp(), rtpData->seqNo())) !=
      misordered->end())
  {
    os<<"voip: "<<OPP_Global::nodeName(this)<<":"<<" duplicate detected "
      <<boost::make_tuple(rtpData->timestamp(), rtpData->seqNo())
      <<" discarding.. "<<endl;

    //duplicate received so clear received count
    s->received--;

    //silently discard
    unsigned int frameCount = frameCountCalc((unsigned int)payloadLength, rtpData);
    VoipFrame* frames = (VoipFrame*) rtpData->contextPointer();
    for (unsigned int i = 0; i < frameCount; i++)
    {
      delete &(frames[i]);
    }
    if (frameCount > 1)
      delete [] frames;

    rtpData->setContextPointer(0);
    return;
  }

  assert( frameCountCalc((unsigned int)payloadLength, rtpData) == 1);
  VoipFrame* thisFrame = (VoipFrame*) rtpData->contextPointer();

  for (SRRI it = probableDropped.begin(); it != probableDropped.end();
       it++)
  {
    DiscardBuffer& range = *it;
    //return value does not tell us whether a match was found and hence removed or not
    //range.erase(std::remove(range.begin(), range.end(), thisFrame->get<1>()), range.end());
    DBI dit = std::find(range.begin(), range.end(), thisFrame->get<1>());
    if (dit != range.end())
    {
      range.erase(dit);
      if (range.empty())
	probableDropped.erase(it);
      break;
    }   
  }

  if (misordered->full())
  {
    //didn't envisage so many out of order packets
    //misordered->set_capacity(misordered->size() + 10);
    //os<<"voip: "<<OPP_Global::nodeName(rtp)<<":"<<" many misordered packets capacity of misorderd is now "
    //  <<misordered->capacity()<<endl;
    
    //just let them be overwritten as when seq number rolls around may not be
    //duplicate
    os<<"voip: "<<OPP_Global::nodeName(this)<<":"<<" many misordered packets discarding head="
      <<misordered->front()<<" capacity="<<misordered->capacity()<<endl;
  }
  misordered->push_back(*thisFrame);
}


double RTPVoip::ecalculateRFactor()
{
  RTPMemberEntry& rme = *(static_cast<RTPMemberEntry*>(playoutTimer->contextPointer()));

  if (!erfactorVector)
  {
    erfactorVector = new cOutVector((std::string("Rfactor of ") + IPAddressResolver().hostname(rme.addr)).c_str());
    emodelTimer = new cCallbackMessage((std::string("rfactorcalc ") + OPP_Global::nodeName(this)).c_str());
    (*emodelTimer) = boost::bind(&RTPVoip::ecalculateRFactor, this);
    totalDelayVector = new cOutVector((std::string("totalDelay of ") + IPAddressResolver().hostname(rme.addr)).c_str());
    totalDelayStat = new cStdDev((std::string("totalDelay of ") + IPAddressResolver().hostname(rme.addr)).c_str());
    emodelTimer->rescheduleDelay(30);
    return 0;
  }

  emodelTimer->rescheduleDelay(30);

  unsigned int expected = rme.cycles + rme.maxSeq - elastExpected;
  if (elastExpected == 0)
    //because first seq no. received is 0 not 1 
    expected ++;
  unsigned int received = elastReceived;

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

  for (SRRI it = probableDropped.begin(); it != probableDropped.end();
       it++)
  {
    elossEvents.push_back((*it).size());
  }

  if (elossEvents.size())
  {
    p = (double)elossEvents.size()/received;
    q = (double)elossEvents.size()/std::accumulate(elossEvents.begin(), elossEvents.end(), 0);
  }

  double ppl = (double)std::accumulate(elossEvents.begin(), elossEvents.end(), 0)/expected;

  if (relativeDiff(ppl, 1-((double)received/expected)) > RTPTimeEpsilon)
    cout<<"voip: "<<OPP_Global::nodeName(this)<<":"<<simTime()<<" mismatch ppl from lost/expected="
	<<std::accumulate(elossEvents.begin(), elossEvents.end(), 0)<<"/"<<expected
	<<" ppl="<<ppl<<" from 1 - (received/expect)="<<received<<"/"<<expected<<" ppl="
	<<1.0-(double)received/expected<<endl;
  double Ppl = 100.0*ppl; //in percentage points 
  double burstR = 1.0;
  if (Ppl > 0)
    burstR = (1.0 - ppl)*((double)std::accumulate(elossEvents.begin(), elossEvents.end(), 0)/(double)elossEvents.size());
  if (p + q != 0 && relativeDiff(burstR , (1.0/(p+q))) > RTPTimeEpsilon)
    cout<<"voip: "<<OPP_Global::nodeName(this)<<":"<<simTime()<<"mismatch (1-ppl)e(k) "
	<<" e(k)="<<(double)std::accumulate(elossEvents.begin(), elossEvents.end(), 0)/(double)elossEvents.size()
	<<" burstR="<<burstR<<" whilst using 1/(p+q) p="<<p<<" q="<<q<<" burstR="<<(1.0/(p+q))<<endl;
  if (relativeDiff(burstR, 1) < RTPTimeEpsilon)
    burstR = 1.0;

  double Ieff = Ie ;
  if (Ppl > 0)
    Ieff += (95.0 - Ie)*(Ppl/((Ppl/burstR)+Bpl));

  double rfactor = 93.2;
  rfactor+= -Id - Ieff;

  erfactorVector->record(rfactor);
  std::ostream& os = printRoutingInfo(true, 0, 0, true);
  os<<"emeanDelay="<< emeanDelay<<" bpl="<< Bpl<<" Ie="<< Ie<<" received="<<received
      <<" expected="<<expected<<" lossEventsSum="<<(double)std::accumulate(elossEvents.begin(), elossEvents.end(), 0)
      <<" lossEventsCount="<<(double)elossEvents.size()<<" rfactor="<<rfactor<<endl;
  
  emeanDelay = 0;
  elossEventsCount += elossEvents.size();
  elostPackets = std::accumulate(elossEvents.begin(), elossEvents.end(), elostPackets);
  elossEvents.clear();
  probableDropped.clear();
  elastReceived = 0;
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
  totalDelayVector->record(playoutDelay + codecDelay);
  totalDelayStat->collect(playoutDelay + codecDelay);
  emeanDelay = (emeanDelay + playoutDelay + codecDelay)/2;
}
