// -*- C++ -*-
// Copyright (C) 2006 
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
 * @file   RTP.cc
 * @author Johnny Lai
 * @date   30 Jul 2006
 *
 * @brief  Implementation of RTP
 *
 * @todo session leaving and subsequent removal from members list 
 *   Separation of RTP Apps like VoIP from RTP primitives
 *
 */

//Headers for libcwd debug streams have to be first (remove if not used)
#include "sys.h"
#include "debug.h"

#include <iomanip>
#include <cassert>

#include "RTP.h"
#include "RTPPacket.h"
#include "RTCPPacket.h"
#include "IPAddressResolver.h"
#include "UDPControlInfo_m.h"
#include "opp_utils.h"
#include "RTCPSR.h"
#include "IPv6Address.h"
#include "opp_utils.h" //nodename
#include "NotificationBoard.h"
#include "NotifierConsts.h"
#include <boost/bind.hpp>
#include "TimerConstants.h"



Define_Module(RTP);

#define RTP_SEQ_MOD (1<<16)

const static int txCalc = 0;

unsigned int dgramSize(cMessage* msg);

using std::setprecision;
using std::ios_base;
using std::setiosflags;
std::ostream& operator<<(std::ostream& os, const RTPMemberEntry& rme)
{
  if (rme.badSeq == rme.maxSeq && rme.maxSeq == 0)
  {
    os <<" self";
  }
  else if (rme.sender)
  {
    os<<rme.maxSeq<<" cycles="<<rme.cycles<<" badSeq="<<rme.badSeq<<" received="
      <<rme.received<<" transit="<<setiosflags(ios_base::scientific)<<rme.transit
      <<" jitter="<<rme.jitter<<" lastSR="<<rme.lastSR<<" addr:"<<rme.addr;
  }
  else
    os<<" receiver only "<<rme.addr;
  return os;      
}

std::ostream& operator<<(std::ostream& os, const RTCPReportBlock& rb)
{
  os << "ssrc="<<rb.ssrc<<" fraction="<<rb.fractionLost<<" lost="
     <<rb.cumPacketsLost<<" seqNo="<<rb.seqNoReceived<<" jitter="<<rb.jitter
     <<" lastSR="<<rb.lastSR<<" dlSR="<<rb.delaySinceLastSR;
  return os;
}

//keep no. of participants in map (keyed on ssrc) but really only one
//until bye is received. but don't delete until x seconds after
//in case out of order packets would recreate it.
//Delete after 5 rtcp intervals no packet from ssrc
	

//6.3.5 time out ssrc (ignore for now) perform at least once per rtcp tx interval
//rtcp send/rec rules 6.3

//From A.1 of rfc3550
void init_seq(RTPMemberEntry *s, u_int16 seq)
{
  s->maxSeq = seq;
  s->badSeq = RTP_SEQ_MOD + 1;   /* so seq == bad_seq is false */
  s->cycles = 0;
  s->received = 1;
  s->receivedPrior = 0;
  s->expectedPrior = 0;

  s->jitter = 0;
  s->transit = 0;
  s->lastSR = 0;
}

RTPMemberEntry::RTPMemberEntry():transVector(0),  transStat(0), lossVector(0),
				 handStat(0), handVector(0), l2handStat(0),
				 l3handStat(0)
{
  
}

void initialiseStats(RTPMemberEntry *s, RTP* rtp)
{
  if (!s->lossVector)
    s->lossVector = new cOutVector((std::string("rtpDrop ") + IPAddressResolver().hostname(s->addr)).c_str());

  if (rtp->isMobileNode())
  {
  if (!s->handStat)
    s->handStat = new cStdDev((std::string("rtpHandover ") + OPP_Global::nodeName(rtp)).c_str());
  if (!s->handVector)
    s->handVector = new cOutVector((std::string("rtpHandover ") + OPP_Global::nodeName(rtp)).c_str());
  if (!s->l2handStat)
    s->l2handStat = new cStdDev((std::string("rtpl2Handover ") + OPP_Global::nodeName(rtp)).c_str());
  if (!s->l3handStat)
    s->l3handStat = new cStdDev((std::string("rtpl3Handover ") + OPP_Global::nodeName(rtp)).c_str());
  }
}

void recordHOStats(RTPMemberEntry *s, RTP* rtp)
{
  if (!rtp->isMobileNode())
    return;

  //handover even when no l2 change? but from what?
  if (rtp->l2down!=0)
  {
    s->handStat->collect(rtp->simTime() - rtp->l2down);
    s->handVector->record(rtp->simTime() - rtp->l2down);

    assert(rtp->l2up);
    s->l2handStat->collect(rtp->l2up - rtp->l2down);
    s->l3handStat->collect(rtp->simTime() - rtp->l2up);
    rtp->l2down = rtp->l2up = 0;
  }
  else
    EV<<OPP_Global::nodeName(rtp)<<" "<<rtp->simTime()
      <<" missedRA triggered handover recording\n";
}

//From A.1 of rfc3550 with probation removed
//with added results collection and modified to assume no resyncing
int update_seq(RTPMemberEntry *s, u_int16 seq, RTP* rtp)
{
  u_int16 udelta = seq - s->maxSeq;
  //modify these two constants if we envisage severe loss due to incorrect handover or
  //just assume never resync like code does below
  const int MAX_DROPOUT = 3000;
  const int MAX_MISORDER = 100;

  if (udelta < MAX_DROPOUT) {
    /* in order, with permissible gap */
    if (seq < s->maxSeq) {
      /*
       * Sequence number wrapped - count another 64K cycle.
       */
      s->cycles += RTP_SEQ_MOD;
      //assuming no drops here when we wrap altough we should really test how far
      //seq is from 0 or baseSeq etc.
    } else if (udelta > 1)
    {
      //record this as no. of packets lost although in reality could just be out
      //of order packet (at this stage we are simply recording likely
      //drop. Actually cumulative loss as determined by expected - received is
      //more accurate
      initialiseStats(s, rtp);
      s->lossVector->record(udelta);
      recordHOStats(s, rtp);
    }
      
    s->maxSeq = seq;
  } else if (udelta <= RTP_SEQ_MOD - MAX_MISORDER) {
    /* the sequence number made a very large jump */
    if (seq == s->badSeq) {
      /*
       * Two sequential packets -- assume that the other side
       * restarted without telling us so just re-sync
       * (i.e., pretend this was the first packet).
       */
      //init_seq(s, seq);
      //EV<<"sequence reinitialised as two sequential packets from other side after huge jump so resyncing";

      //don't want a total resync because we know otherside does not do a real
      //resync in my sim and we want to preserve our stats
      initialiseStats(s, rtp);
      s->lossVector->record(udelta);
      s->maxSeq = seq;
      recordHOStats(s, rtp);
    }
    else {
      s->badSeq = (seq + 1) & (RTP_SEQ_MOD-1);
      return 0;
    }
  } else {
    /* duplicate or reordered packet */
  }
  s->received++;
  return 1;
}

void RTP::processReceivedPacket(cMessage* msg)
{
  UDPControlInfo *ctrl = check_and_cast<UDPControlInfo *>(msg->controlInfo());

  IPvXAddress srcAddr = ctrl->getSrcAddr();
  IPvXAddress destAddr = ctrl->getDestAddr();
  //  int srcPort = ctrl->getSrcPort();
  int destPort = ctrl->getDestPort();

  if (destPort == port) //RTP
  {
    assert( dynamic_cast<RTPPacket*>(msg));
    RTPPacket* rtpData = static_cast<RTPPacket*> (msg);
    if (!memberSet.count(rtpData->ssrc()))
    {
      //may wait for many before consider valid but we'll skip that
      members+=1;
      memberSet[rtpData->ssrc()].sender = false;
      memberSet[rtpData->ssrc()].addr = srcAddr;
    }

    RTPMemberEntry& rme = memberSet[rtpData->ssrc()];
    //add ssrc to sender map if not a sender
    if (!rme.sender)
    {
      rme.sender = true;
      senders += 1;
      init_seq(&rme, rtpData->seqNo());
    }
    else if (!update_seq(&rme, rtpData->seqNo(), this))
    {
      EV<<"huge jump and bad sequence rec. should reset stats?? No unless we have multiple sessions with same peer";
      return;
    }

    //further processing of RTP
    //jitter calc (cheating as should be all ints i.e. rtp time units)
    simtime_t transit = simTime() - rtpData->timestamp();
    simtime_t instJitter = transit - rme.transit;
    rme.transit = transit;
    if (instJitter < 0) 
      instJitter = -instJitter;
    rme.jitter += ((double)1/16)*(instJitter - rme.jitter);

    if (!rme.transVector)
      rme.transVector = new cOutVector((std::string("transitTimes ") + IPAddressResolver().hostname(rme.addr)).c_str());
    rme.transVector->record(transit);
    if (!rme.transStat)
      rme.transStat = new cStdDev((std::string("rtpTransitTimes ") + IPAddressResolver().hostname(rme.addr)).c_str());
    rme.transStat->collect(transit);

    return;
  } //RTP message
  else //RTCP
  {
    //EV<<"received rtcp msg "<<msg;

    assert(dynamic_cast< RTCPPacket*>(msg));
    meanRtcpSize = ((double)1/16)*(double)dgramSize(msg) + ((double)15/16)*meanRtcpSize;
    RTCPPacket* rtcp = static_cast< RTCPPacket*>(msg);

    if (!memberSet.count(rtcp->ssrc()))
    {
      //may wait for many before consider valid but we'll skip that
      members+=1;
      memberSet[rtcp->ssrc()].sender = false;
      memberSet[rtcp->ssrc()].addr = srcAddr;
    }

    //further rtcp types like reports what to do etc.
    //(Sec 6.4 for analysing reports)

    if (dynamic_cast< RTCPGoodBye*> (msg))
    {
      //if bye received remove entry from members/senders map 
      //ignore reverse reconsideration 6.3.4 (code in pg 93)
      if (memberSet.count(rtcp->ssrc()))
      {
        members-=1;
        if (memberSet[rtcp->ssrc()].sender)
          senders-=1;
        memberSet.erase(rtcp->ssrc());
      }
    }
    else if (dynamic_cast<RTCPReports*> (msg))
    {
      RTCPSR* sr = dynamic_cast<RTCPSR*> (msg);
      if (sr)
      {
        memberSet[rtcp->ssrc()].lastSR = sr->timestamp();
        incomingBlocks.resize(0);
        for (u_int32 i = 0; i < sr->reportBlocksArraySize(); ++i)
          incomingBlocks.push_back(sr->reportBlocks(i));
      }
      else
      {
        RTCPRR* rr = dynamic_cast<RTCPRR*>(msg);
        incomingBlocks.resize(0);
        for (u_int32 i = 0; i < rr->reportBlocksArraySize(); ++i)
          incomingBlocks.push_back(rr->reportBlocks(i));
      }	
    }
    else if (dynamic_cast<RTCPSDES*> (msg))
    {
      //voip conversation model requires passing of callee RTP module for 2
      //party on/off state switching
      RTP* caller = static_cast<RTP*>(msg->contextPointer());
      if (caller->p59cb)
      {
        rtpTimeout = new cMessage("callee rtpTimeout");
        destAddrs.push_back(srcAddr);
        (*(caller->p59cb)) =  boost::bind(&RTP::P59TS, caller, this);
        OPP_Global::ContextSwitcher switchContext(caller);
        caller->p59cb->rescheduleDelay(SELF_SCHEDULE_DELAY);
      }
      else
      {
        //other app. once we migrate this to its own class/module will not need
        //so many branching bits
      }
    }
  } //RTCP message
}

//send bye when finished
void RTP::leaveSession()
{
  //leaving session 6.3.7 (ignore part about members more than 50)
  simtime_t now = simTime();
  tp = now;
  members = pmembers = 1;
  initial = true;
  weSent = false;
  senders = 0;
  simtime_t T = calculateTxInterval();

  //timer reconsideration is simply like the sending of reports (see pg 91 of rfc)
  tn = tp + T;
  if (tp + T <= now )
  {
    //create sendBye fn and do a sendonce etc.
    RTCPPacket* bye = new RTCPGoodBye(_ssrc);
    sendToUDP(bye, port+1, destAddrs[0], port+1);
    meanRtcpSize = dgramSize(bye);
  }
  else
  {
    //scheduleAt(tn, leaveSession);
  }
}

//establish session for each dest?
void RTP::establishSession()
{
  assert(!destAddrs.empty());
  if (!p59cb)
    //conference type of app
  //send a cname packet (i.e. user@fullpath etc.) inside first rtcp packet  
  for (unsigned int i =0; i < destAddrs.size(); ++i)
    sendToUDP(new RTCPSDES(_ssrc, OPP_Global::nodeName(this)),
	      port+1, destAddrs[i], port+1);
  else
  {
    //voip app 2 party conversation 
    RTCPSDES* sdes = new RTCPSDES(_ssrc, OPP_Global::nodeName(this));
    sdes->setContextPointer(this);
    sendToUDP(sdes, port+1, destAddrs[0], port+1);
  }
}

//hard coded 25% and 75% of rtcpBW (hence the factor of 0.75 and division by 4)
simtime_t RTP::calculateTxInterval()
{
  assert(rtcpBw);
  //alg for rtcp tx interval in 6.3 and A.7 of rfc3550

  //watch these values for txtimeout calc
  float C = 0;
  int n = 0;

  if (senders > members/4)
    {
      //most likely execute this case all the time
      C = meanRtcpSize/(rtcpBw);
      n = members;
    }
  else
    {
      if (weSent)
	{
	  C = meanRtcpSize/(rtcpBw/4.0);
	  n = senders;
	}
      else
	{
	  C = meanRtcpSize/(rtcpBw*0.75);
	  n = members - senders;
	} 
    }

  //reduced min is 360/session bandwidth
  float Td = max(initial?2.5:5.0, n*C);
  // randomly [0.5,1.5] times calculated interval to avoid sync
  simtime_t T = (uniform(0,1, txCalc) + 0.5)*Td;
  T= T/1.21828; //e-3/2
  return T;
}




RTP::RTP():rtcpTimeout(0), rtpTimeout(0),p59cb(0),talkStatesVector(0),stStat(0),
	   dtStat(0), msStat(0), tsStat(0),nb(0),l2down(0),mobileNode(false)
{}

RTP::~RTP()
{
  if (rtpTimeout->isScheduled())
    cancelEvent(rtpTimeout);
  if (rtcpTimeout->isScheduled())
    cancelEvent(rtcpTimeout);
  delete rtpTimeout;
  delete rtcpTimeout;
  if (p59cb && p59cb->isScheduled())
  {
    p59cb->cancel();
    delete p59cb;
  }
}

int RTP::numInitStages() const
{
  return 4;
}

void RTP::initialize(int stageNo)
{
  if (stageNo != 3)
    return;

  nb = NotificationBoardAccess().get();
  nb->subscribe(this, NF_L2_BEACON_LOST);
  nb->subscribe(this, NF_L2_ASSOCIATED);

  port = par("port");
  //RTP port is always even so if specified odd port then
  //the base port will be modified
  if (port%2 > 0)
    port -= 1;

  startTime = par("startTime");
  simtime_t stopTime = par("stopTime");

  WATCH(senders);
  WATCH(members);
  WATCH(pmembers);
  WATCH(tp);
  WATCH(tn);
  WATCH(meanRtcpSize);
  WATCH(initial);
  WATCH(weSent);

  WATCH_MAP(memberSet);
  WATCH_VECTOR(incomingBlocks);  

  WATCH(packetCount);
  WATCH(octetCount);
  
  //RTP port
  bindToPort(port);
  //RTCP port
  bindToPort(port + 1);
  

  //initialise 6.3.2 (when joining session)
  _ssrc = id();
  _seqNo = 0; //supposed to be random to prevent encryption attacks
  packetCount = octetCount = 0;
  initial = true;
  tp = 0; 
  tn = 0; 
  senders = 0;
  pmembers = members = 1;
  weSent = false;

  memberSet[_ssrc].sender = false;
  //Identify ourselves as self for the operator<< fn
  init_seq(&memberSet[_ssrc], 0);
  memberSet[_ssrc].badSeq = 0;

 
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
  WATCH(rtcpBw);

  //assuming 1 SR Report Block from other side if we
  //received RTP otherwise just 28 if we initiate
  meanRtcpSize = 28 + 24;
  tn = calculateTxInterval();
  rtcpTimeout = new cMessage("rtcpTimeout");

  //Want to make sure we give enough time for global addresses to be assigned to
  //nodes otherwise the simulation will abort instead of sending stuff to link
  //local address
  simtime_t randomStart = startTime?(startTime - 2*tn):0;
  scheduleAt(randomStart < 7? 8 + randomStart:randomStart, rtcpTimeout);

  if (startTime && sendRTPPacket() && !weSent)
  {
      weSent = true;
      senders+=1;
      memberSet[_ssrc].sender = true;
  }

  if (startTime)
  {
    p59cb = new cCallbackMessage("p.59");
    talkStatesVector =
      new cOutVector((std::string("talkState of ") + OPP_Global::nodeName(this)).c_str());
    stStat = new cStdDev((std::string("ST duration of ") + OPP_Global::nodeName(this)).c_str());
    msStat = new cStdDev((std::string("MS duration of ") + OPP_Global::nodeName(this)).c_str());
    dtStat = new cStdDev((std::string("DT duration of ") + OPP_Global::nodeName(this)).c_str());
    tsStat = new cStdDev((std::string("TS duration of ") + OPP_Global::nodeName(this)).c_str());
  }

  if (stopTime > 0)
  {
    cCallbackMessage* quitApp = new cCallbackMessage("quitApp leak");
    //(*quitApp) = boost::bind(&RTP::exitVoipSession, this);  
    scheduleAt(stopTime, quitApp);
  }
}

//remove self from destAddrs i.e. nodename matches
void RTP::resolveAddresses()
{
  assert(destAddrs.empty());
  const char *destAddresses = par("destAddrs");
  cStringTokenizer tokenizer(destAddresses);
  const char *token;
  while ((token = tokenizer.nextToken())!=NULL)
  {
    if (strcmp(OPP_Global::nodeName(this), token) == 0)
      continue;
    IPvXAddress addr = IPAddressResolver().resolve(token);
    if (IPv6Address(addr.get6().str().c_str()).scope()  !=
	ipv6_addr::Scope_Global)
    {
      cerr<<OPP_Global::nodeName(this)<<" Address of dest "<<token<<" is not "
	  <<"global at "<<simTime()<<endl;
      //assert(false);
    }
    destAddrs.push_back(IPAddressResolver().resolve(token));

  }
}

bool RTP::sendRTPPacket()
{  
  if (!rtpTimeout)
  {
    rtpTimeout = new cMessage("rtpTimeout");
    scheduleAt(startTime, rtpTimeout);

    //prob need dest port if we want multiple rtp apps of same type
    return false;
  }

  if (destAddrs.empty())
  {
    resolveAddresses();
  }

  if (startTime && !destAddrs.empty() && !weSent)
  {
    if (p59cb && !p59cb->isScheduled())
    {
      establishSession();
      return false;
    }
  }

  if (destAddrs.empty())
    return false;

  scheduleAt(simTime() + packetisationInterval, rtpTimeout);
  RTPPacket* rtpData = new RTPPacket(_ssrc, _seqNo++);
  //rtpData->setKind(1);
  rtpData->setTimestamp(simTime());
  rtpData->setPayloadLength((unsigned int)payloadLength);

  //These should stay as they are because these operations are usually assumed
  //to be multicast and even if they're unicast they should not be multiplied by
  //number of receivers because that's not the number of packets sent to a
  //recipient.
  octetCount += rtpData->payloadLength();
  packetCount++;

  for (unsigned int i =0; i < destAddrs.size(); ++i)
  {
    sendToUDP(static_cast<cMessage*>(rtpData->dup()), port, destAddrs[i], port);
  }
  delete rtpData;
  return true;
}

unsigned int dgramSize(cMessage* msg)
{
  //8 is UDP header and 40 is IPv6 header
  //needs to include i.e. calculation of mobility headers and tunnelling etc.
  return msg->byteLength() + 8 + 40;
}

void RTP::fillReports(RTCPReports* rtcpPayload)
{
      RTCPReportBlock b;
      for (MSI msi = memberSet.begin(); msi != memberSet.end(); ++msi)
      {
	if (!msi->second.sender)
	  continue;
	
	RTPMemberEntry& rme = msi->second;
	//don't send reception report for self
	if (msi->first == _ssrc)
	  continue;

	b.ssrc = msi->first;

	//See A.3 for computing packet loss 
	u_int32 extended = rme.cycles + rme.maxSeq;
	//expected = extended + 1 since baseSeq assumed to be 0 as we do not
	//randomise seqNo
	extended++;
	b.cumPacketsLost = extended - rme.received;
	b.seqNoReceived = extended;
	u_int32 expectedInt = extended - rme.expectedPrior;
	rme.expectedPrior = extended;
	u_int32 receivedInt = rme.received - rme.receivedPrior;
	rme.receivedPrior = rme.received;
	u_int32 lostInt = expectedInt - receivedInt;
	if (expectedInt == 0 || lostInt <= 0) 
	  b.fractionLost = 0;
	else
	  b.fractionLost = (lostInt <<8)/ expectedInt;	

	b.jitter = rme.jitter;
	b.lastSR = rme.lastSR;
	b.delaySinceLastSR = simTime() - b.lastSR;

	rtcpPayload->addBlock(b);
      }
}

void RTP::rtcpTxTimeout()
{
  simtime_t T = calculateTxInterval();
  simtime_t now = simTime();
  tn = tp + T;
  if (tn <= now) 
  {
    //tx rtcp 
    RTCPReports* rtcpPayload = 0;
    if (weSent)
    {
      rtcpPayload = new RTCPSR(_ssrc, packetCount, octetCount);
    }
    else 
    {
      rtcpPayload = new RTCPRR(_ssrc);
    }
    //ntp time
    rtcpPayload->setTimestamp(simTime());
    //rtcpPayload->setKind(1);

    fillReports(rtcpPayload);

    if (destAddrs.empty())
      resolveAddresses();

    //should send to all members 
    for (MSI msi = memberSet.begin(); msi != memberSet.end(); ++msi)
    {
      RTPMemberEntry& rme = msi->second;
      //don't send reception report for self
      if (msi->first == _ssrc)
	continue;

      sendToUDP(static_cast<cMessage*>(rtcpPayload->dup()), port+1, rme.addr, port+1);
      meanRtcpSize = ((double)1/16)*(double)dgramSize(rtcpPayload) + ((double)15/16)* meanRtcpSize;	
    }
    delete rtcpPayload;

    //keep count of whether any RTP sent if after 2 intervals none sent set
    //weSent = false;

    initial = false;
    tp = now;
    T = calculateTxInterval();
    scheduleAt(now + T, rtcpTimeout);
  }
  else
    scheduleAt(tn, rtcpTimeout);
  pmembers = members;
}

void RTP::handleMessage(cMessage* msg)
{
  if (msg == rtcpTimeout)
  {
    //fill rdns cache in case no one has requested us otherwise labels will
    //appear blank when IPAddressResolver::hostname called
    /* //appears to be too early i.e. global addr not configured yet
       //need a version that does not abort and returns null addr so we try later
    if (initial)
      IPAddressResolver().resolve(OPP_Global::nodeName(this));
    */
    rtcpTxTimeout();
  }
  else if (msg == rtpTimeout)
  {
    if (sendRTPPacket() && !weSent)
    {
      weSent = true;
      senders+=1;
      memberSet[_ssrc].sender = true;
    }
  }
  else if (!msg->isSelfMessage())
  {
    processReceivedPacket(msg);
    delete msg;
  }
  else
  {
    assert(msg->isSelfMessage());
    (static_cast<cTimerMessage *> (msg) )->callFunc();
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

void RTP::P59TS(RTP* callee)
{
  assert(!p59cb->isScheduled());
  talkStatesVector->record(1);

  //get caller to start a talk spurt
  if (!this->rtpTimeout->isScheduled())
    scheduleAt(simTime() + packetisationInterval, this->rtpTimeout);
  //get callee to stop talking

  double timeInstate = -0.854 * log(1-uniform(0,1,x1));
  tsStat->collect(timeInstate);

  //check which state we transition to now
  if (uniform(0,1,stateTrans) > P1)
    (*p59cb)=boost::bind(&RTP::P59MS, this, callee);
  else
    (*p59cb)=boost::bind(&RTP::P59DT, this, callee);

  p59cb->rescheduleDelay(timeInstate);

  OPP_Global::ContextSwitcher switchContext(callee);
  if (callee->rtpTimeout->isScheduled())
    callee->cancelEvent(callee->rtpTimeout);
}

void RTP::P59DT(RTP* callee)
{
  assert(!p59cb->isScheduled());
  talkStatesVector->record(4);

  //get  both to talk
  if (!this->rtpTimeout->isScheduled())
    scheduleAt(simTime() + packetisationInterval, this->rtpTimeout);

    
  double timeInstate = -0.226 * log(1-uniform(0,1,x2));
  dtStat->collect(timeInstate);

  //check which state we transition to now
  if (uniform(0,1,stateTrans) > P3)
    (*p59cb)=boost::bind(&RTP::P59TS, this, callee);
  else
    (*p59cb)=boost::bind(&RTP::P59ST, this, callee);

  p59cb->rescheduleDelay(timeInstate);

  OPP_Global::ContextSwitcher switchContext(callee);
  if (!callee->rtpTimeout->isScheduled())
    callee->scheduleAt(simTime() + packetisationInterval, callee->rtpTimeout);
}

void RTP::P59ST(RTP* callee)
{
  assert(!p59cb->isScheduled());
  talkStatesVector->record(2);

  //get callee to talk and caller to shut up
  if (this->rtpTimeout->isScheduled())
    cancelEvent(this->rtpTimeout);
  
  double timeInstate = -0.854 * log(1-uniform(0,1,x1));
  stStat->collect(timeInstate);

  //check which state we transition to now
  if (uniform(0,1,stateTrans) > P1)
    (*p59cb)=boost::bind(&RTP::P59MS, this, callee);
  else
    (*p59cb)=boost::bind(&RTP::P59DT, this, callee);

  p59cb->rescheduleDelay(timeInstate);

  OPP_Global::ContextSwitcher switchContext(callee);
  if (!callee->rtpTimeout->isScheduled())
    callee->scheduleAt(simTime() + packetisationInterval, callee->rtpTimeout);
}

void RTP::P59MS(RTP* callee)
{
  assert(!p59cb->isScheduled());
  talkStatesVector->record(3);

  if (this->rtpTimeout->isScheduled())
    cancelEvent(this->rtpTimeout);

  double timeInstate = -0.456 * log(1-uniform(0,1,x3));
  msStat->collect(timeInstate);

  if (uniform(0,1,stateTrans) > P2)
    (*p59cb)=boost::bind(&RTP::P59TS, this, callee);
  else
    (*p59cb)=boost::bind(&RTP::P59ST, this, callee);

  p59cb->rescheduleDelay(timeInstate);

  OPP_Global::ContextSwitcher switchContext(callee);
  if (callee->rtpTimeout->isScheduled())
    callee->cancelEvent(callee->rtpTimeout);
}

bool RTP::isMobileNode()
{
  return mobileNode;
}

void RTP::finish()
{
  using std::cout;
  for (MSI msi = memberSet.begin(); msi != memberSet.end(); ++msi)
  {
    if (!msi->second.sender)
      continue;
	
    RTPMemberEntry& rme = msi->second;
    if (msi->first == _ssrc)
      continue;

    if (!rme.transStat)
    {
      cout<<"WARNING " <<OPP_Global::nodeName(this)<<" not one packet's eed was recorded for member "<<IPAddressResolver().hostname(rme.addr);
      continue;
    }

    u_int32 extended = rme.cycles + rme.maxSeq;
    extended ++;
    u_int32 cumPacketsLost = extended - rme.received;
    cout <<"--------------------------------------------------------" <<endl;
    cout <<"\t"<<OPP_Global::nodeName(this)<<" from "<< IPAddressResolver().hostname(rme.addr)<<endl;    
    cout <<"--------------------------------------------------------" <<endl; 
    cout <<"drop rate (%): "<<100 * (double)cumPacketsLost/(double)extended;
    cout<<" eed of "<<rme.transStat->samples()<<"/"<<extended<<" recorded/\"expected\"\n";
    cout<<"rtp transit min/avg/max = "
	<<rme.transStat->min()*1000.0<<"ms/"<<rme.transStat->mean()*1000.0<<"ms/"<<rme.transStat->max()*1000.0<<"ms"<<endl;
    cout<<"stddev="<<rme.transStat->stddev()*1000.0<<"ms variance="<<rme.transStat->variance()*1000.0<<"ms\n";

    rme.transStat->recordScalar((std::string("rtpTransitTime of ") + IPAddressResolver().hostname(rme.addr)).c_str());

    
    if (isMobileNode())
    {
      rme.handStat->recordScalar((std::string("rtpHandover of ") + OPP_Global::nodeName(this)).c_str());
      rme.l2handStat->recordScalar((std::string("rtpl2Handover of ") + OPP_Global::nodeName(this)).c_str());
      rme.l3handStat->recordScalar((std::string("rtpl3Handover of ") + OPP_Global::nodeName(this)).c_str());
    }

    recordScalar((std::string("rtp dropped from ") + IPAddressResolver().hostname(rme.addr)).c_str(), cumPacketsLost);
    recordScalar((std::string("rtp % dropped from ") + IPAddressResolver().hostname(rme.addr)).c_str(),
		 100 * (double)cumPacketsLost/(double)extended);
    recordScalar((std::string("rtp received from ") + IPAddressResolver().hostname(rme.addr)).c_str(), rme.received);
    recordScalar("rtpOctetCount", octetCount);

    if (p59cb)
    {
      stStat->recordScalar((std::string("ST time of ") + OPP_Global::nodeName(this)).c_str());
      dtStat->recordScalar((std::string("DT time of ") + OPP_Global::nodeName(this)).c_str());  
      msStat->recordScalar((std::string("MS time of ") + OPP_Global::nodeName(this)).c_str());  
      tsStat->recordScalar((std::string("TS time of ") + OPP_Global::nodeName(this)).c_str());
    }
  }

}

void RTP::receiveChangeNotification(int category, cPolymorphic *details)
{
  Enter_Method_Silent();
  printNotificationBanner(category, details);

  mobileNode = true;
  if (category == NF_L2_BEACON_LOST)
    l2down = simTime();
  else if (category == NF_L2_ASSOCIATED)
    l2up = simTime();
}
