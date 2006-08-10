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
#include "RTPPacket_m.h"
#include "IPAddressResolver.h"
#include "RTCPSR.h"

Define_Module(RTP);

unsigned int dgramSize(cMessage* msg);

//keep no. of participants in map (keyed on ssrc) but really only one
//until bye is received. but don't delete until x seconds after
//in case out of order packets would recreate it.
//Delete after 5 rtcp intervals no packet from ssrc
	

//Last section of rfc 3550 has jitter estimation

//rtcp send/rec rules 6.3

void RTP::processReceivedPacket(cMessage* msg)
{
  EV<<"hooray received msg "<<msg;
  //ssrc not in member map then add 
  //may wait for many before consider valid but we'll skip that
  //update members+=1;
  //if packet is rtp add ssrc to sender map
  //senders += 1;
  //else
  //meanRtcpSize = (1/16)*dgram->length() + 15/16* meanRtcpSize;
  //if bye received remove entry from members/senders map 
  //ignore reverse reconsideration 6.3.4 (code in pg 93)
  
}

//6.3.5 time out ssrc (ignore for now) perform at least once per rtcp tx interval

void RTP::leaveSession()
{
  //leaving session 6.3.7 (ignore part about members more than 50)
  //send bye when finished
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
    RTCPPacket* bye = new RTCPGoodBye();
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
  //send a cname packet (i.e. user@fullpath etc.) inside first rtcp packet
  //so 
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
  if (port%2 > 0)
    port = port - 1;

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

  //RTP port
  bindToPort(port);
  //RTCP port?
  bindToPort(port + 1);
  

  //initialise 6.3.2 (when joining session)
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
  //schedule RTCP tx at tn = T
  scheduleAt(tn, rtcpTimeout);

  //add self ssrc to member table

  if (startTime && sendRTPPacket() && !weSent)
  {
      weSent = true;
      senders+=1;
      //add self to senders map
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

  unsigned int seqNo = 0;
  unsigned int ssrc = 0; //retrieve interfaceId and use lower 32 bits

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
  RTPPacket* rtpData = new RTPPacket;
  rtpData->setSeqNo(seqNo++);
  rtpData->setSsrc(ssrc);
  rtpData->setPayloadLength(payloadLength);
  sendToUDP(rtpData, port+1, destAddrs[0], port+1);
  return true;
}

void RTP::finish()
{
}

unsigned int dgramSize(cMessage* msg)
{
  //8 is UDP header and 40 is IPv6 header
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
      //tx rtcp (Sec 6.4 for analysing reports)
      RTCPRR* rtcpPayload = 0;
      if (weSent)
      {
	rtcpPayload = new RTCPSR();
      }
      else 
      {
	rtcpPayload = new RTCPRR();
      }
      //ssrc? what to use
      RTCPReportBlock b;
      //damn ctors in the msg generation prevents this
      //RTCPReportBlock b = {0, 0, 0, 0, 0, 0, 0};
      rtcpPayload->addBlock(b);
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

    //self msg can also be a bye
  }
  else if (msg == rtpTimeout)
  {
    if (sendRTPPacket() && !weSent)
    {
      weSent = true;
      senders+=1;
      //add self to senders map
    }
  }
  else if (!msg->isSelfMessage())
  {
    processReceivedPacket(msg);
    delete msg;
  }
  else
  {
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
