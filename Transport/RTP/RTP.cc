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
 * @author 
 * @date   30 Jul 2006
 *
 * @brief  Implementation of RTP
 *
 * @todo
 *
 */

//Headers for libcwd debug streams have to be first (remove if not used)
#include "sys.h"
#include "debug.h"

#include "RTP.h"
#include "RTPPacket.h"
#include "RTCPPacket.h"
#include "IPAddressResolver.h"
#include "UDPControlInfo_m.h"
#include "opp_utils.h"
#include "RTCPSR.h"

Define_Module(RTP);

#define RTP_SEQ_MOD (1<<16)

unsigned int dgramSize(cMessage* msg);

std::ostream& operator<<(std::ostream& os, const RTPMemberEntry& rme)
{
  if (rme.sender && rme.badSeq == rme.maxSeq && rme.maxSeq == 0)
  {
    os <<" self";
  }
  else if (rme.sender)
  {
    os<<rme.maxSeq<<" cycles="<<rme.cycles<<" badSeq="<<rme.badSeq<<" received="
      <<rme.received<<" transit="<<rme.transit<<" jitter="<<rme.jitter
      <<" lastSR="<<rme.lastSR;
  }
  else
    os<<" receiver only ";
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
}

//From A.1 of rfc3550 with probation removed
int update_seq(RTPMemberEntry *s, u_int16 seq)
{
  u_int16 udelta = seq - s->maxSeq;
  const int MAX_DROPOUT = 3000;
  const int MAX_MISORDER = 100;

  if (udelta < MAX_DROPOUT) {
    /* in order, with permissible gap */
    if (seq < s->maxSeq) {
      /*
       * Sequence number wrapped - count another 64K cycle.
       */
      s->cycles += RTP_SEQ_MOD;
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
      init_seq(s, seq);
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
  EV<<"received msg "<<msg;
  
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
    }

    RTPMemberEntry& rme = memberSet[rtpData->ssrc()];
    //add ssrc to sender map if not a sender
    if (!rme.sender)
    {
      rme.sender = true;
      senders += 1;
      init_seq(&rme, rtpData->seqNo());
    }
    else if (!update_seq(&rme, rtpData->seqNo()))
    {
      EV<<"huge jump and bad sequence rec. should reset stats??";
    }

    //further processing of RTP
    //jitter calc (cheating as should be all ints i.e. rtp time units)
    simtime_t transit = simTime() - rtpData->timestamp();
    simtime_t instJitter = transit - rme.transit;
    rme.transit = transit;
    if (instJitter < 0) 
      instJitter=-instJitter;
    rme.jitter += 1/16*(instJitter - rme.jitter);
       
  } //RTP message
  else //RTCP
  {
    assert(dynamic_cast< RTCPPacket*>(msg));
    meanRtcpSize = (1/16)*dgramSize(msg) + 15/16*meanRtcpSize;
    RTCPPacket* rtcp = static_cast< RTCPPacket*>(msg);

    if (!memberSet.count(rtcp->ssrc()))
    {
      //may wait for many before consider valid but we'll skip that
      members+=1;
      memberSet[rtcp->ssrc()].sender = false;
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
  //send a cname packet (i.e. user@fullpath etc.) inside first rtcp packet  
  for (unsigned int i =0; i < destAddrs.size(); ++i)
    sendToUDP(new RTCPSDES(_ssrc, OPP_Global::nodeName(this)),
	      port+1, destAddrs[i], port+1);
}

//hard coded 25% and 75% of rtcpBW (hence the factor of 0.75 and division by 4)
simtime_t RTP::calculateTxInterval()
{
  //alg for rtcp tx interval in 6.3 and A.7 of rfc3550
  //rtcp tx time alg Sec. 6.3.1
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
  simtime_t T = (uniform(0,1) + 0.5)*Td;
  T= T/1.21828; //e-3/2
  return T;
}








int RTP::numInitStages() const
{
  return 4;
}

void RTP::initialize(int stageNo)
{
  if (stageNo != 3)
    return;

  rtpTimeout = 0;

  port = par("port");
  //RTP port is always even so if specified odd port then
  //the base port will be modified
  if (port%2 > 0)
    port -= 1;

  startTime = par("startTime");
  
  WATCH(rtcpBw);
  WATCH(senders);
  WATCH(members);
  WATCH(pmembers);
  WATCH(tp);
  WATCH(tn);
  WATCH(meanRtcpSize);
  WATCH(initial);
  WATCH(weSent);

  C = 0;
  n = 0;

  WATCH(C);
  WATCH(n);

  WATCH_MAP(memberSet);

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
  // 5% of session bandwidth used for these packets
  rtcpBw = 0.05 * (double)par("sessionBandwidth"); 
  //assuming 1 SR Report Block from other side if we
  //received RTP otherwise just 28 if we initiate
  meanRtcpSize = 28 + 24;
  tn = calculateTxInterval();
  rtcpTimeout = new cMessage("rtcpTimeout");
  scheduleAt(tn, rtcpTimeout);

  memberSet[_ssrc].sender = false;

  if (startTime && sendRTPPacket() && !weSent)
  {
      weSent = true;
      senders+=1;
      memberSet[_ssrc].sender = true;
  }
  
}

void RTP::resolveAddresses()
{
  assert(destAddrs.empty());
  const char *destAddresses = par("destAddrs");
  cStringTokenizer tokenizer(destAddresses);
  const char *token;
  while ((token = tokenizer.nextToken())!=NULL)
    destAddrs.push_back(IPAddressResolver().resolve(token));
}

bool RTP::sendRTPPacket()
{
  double packetisationInterval = 0.02;  //20ms
  //rfc3551 G728 i.e. rtp payload type 15 uses 40bits per frame and
  //each frame encodes 2.5ms so for one 20ms packet requires 8*40 = 320 bytes
  //rtp payload and forms a 16kbs audio stream
  int payloadLength = 320;

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

  //later on add markovian modelling of on/off session
  if (!destAddrs.empty() && !weSent)
    establishSession();

  scheduleAt(simTime() + packetisationInterval, rtpTimeout);
  RTPPacket* rtpData = new RTPPacket(_ssrc, _seqNo++);
  rtpData->setTimestamp(simTime());
  rtpData->setPayloadLength(payloadLength);
  octetCount += rtpData->payloadLength();
  packetCount++;
  sendToUDP(rtpData, port, destAddrs[0], port);
  return true;
}

void RTP::finish()
{
}

unsigned int dgramSize(cMessage* msg)
{
  //8 is UDP header and 40 is IPv6 header
  //needs to include i.e. calculation of mobility headers and tunnelling etc.
  return msg->byteLength() + 8 + 40;
}

void RTP::handleMessage(cMessage* msg)
{
  if (msg == rtcpTimeout)
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

	//damn ctors in the msg generation prevents this
	//RTCPReportBlock b = {0, 0, 0, 0, 0, 0, 0};
	rtcpPayload->addBlock(b);
      }
      if (destAddrs.empty())
	resolveAddresses();
      //should send to all members and not just dests i.e. people that join with us
      sendToUDP(rtcpPayload, port+1, destAddrs[0], port+1);
      meanRtcpSize = (1/16)*dgramSize(rtcpPayload) + 15/16* meanRtcpSize;
      //pg. 42/43 - how to use sender reports to calculate loss rates/ no. of packets received
      //keep count of whether any RTP sent if after 2 intervals none sent set
      //weSent = false;
      initial = false;
      tp = now;
      T = calculateTxInterval();
      scheduleAt(now + T, msg);
    }
    else
      scheduleAt(tn, msg);
    pmembers = members;

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
    //self msg can also be a bye

    //do callback
  }
}

/*
///For non omnetpp csimplemodule derived classes
RTP::RTP()
{
}

RTP::~RTP()
{
}
*/
