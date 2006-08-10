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


//keep no. of participants in map (keyed on ssrc) but really only one
//until bye is received. but don't delete until x seconds after
//in case out of order packets would recreate it.
//Delete after 5 rtcp intervals no packet from ssrc
	
//rtcp send/rec rules 6.3

void RTP::processReceivedPacket(cMessage* msg)
{
  //ssrc not in member map then add 
  //may wait for many before consider valid but we'll skip that
  //update members+=1;
  //if packet is rtp add ssrc to sender map
  //senders += 1;
  //else
  //meanRtcpSize = (1/16)*dgram->length() + 15/16* meanRtcpSize;
  //if bye received remove entry from members/senders map 
  //ignore reverse reconsideration 6.3.4
  
}

//6.3.5 time out ssrc (ignore for now) perform at least once per rtcp tx interval

void RTP::leaveSession()
{
  //leaving session 6.3.7 (ignore members more than 50)
  //send bye when finished
  simtime_t now = simTime();
  tp = now;
  members = pmembers = 1;
  initial = true;
  weSent = false;
  senders = 0;
  simtime_t T = calculateTxInterval();
  //schedule bye for now + T (is tc now?)
  //create sendBye fn and do a sendonce etc.
  //meanRtcpSize = dgram->size();
}

void RTP::establishSession()
{
  //send a cname packet (i.e. user@fullpath etc.) inside first rtcp packet
  //so 

  //if rtp packet sent
  weSent = true;
  senders+=1;
  //add self to senders map
}

simtime_t RTP::calculateTxInterval()
{
  //alg for rtcp tx interval in 6.3 and A.7 of rfc3550
  //rtcp tx time alg Sec. 6.3.1
  int C = 0, n = 0;
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
  T= T*1.21828; //e-3/2
  return T;
}








int RTP::numInitStages() const
{
  return 3;
}

void RTP::initialize(int stageNo)
{
  if (stageNo != 3)
    return;

  port = par("port");
  if (port%2 > 0)
    port = port - 1;



  const char *destAddresses = par("destAddrs");
  cStringTokenizer tokenizer(destAddresses);
  const char *token;
  while ((token = tokenizer.nextToken())!=NULL)
    destAddrs.push_back(IPAddressResolver().resolve(token));

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
  rtcpBw = 0.05; 
  //assuming 1 SR Report Block from other side if we
  //received RTP otherwise just 28 if we initiate
  meanRtcpSize = 28 + 24;
  //calc T
  tn = calculateTxInterval();
  cMessage* rtcpTimeout = new cMessage("rtcpTimeout");
  //schedule RTCP tx at tn = T
  scheduleAt(tn, rtcpTimeout);

  //add self ssrc to member table

  //establish session if destAddr is true otherwise stay in listen state
  if (!destAddrs.empty())
  {
    establishSession();
  }
}

void RTP::finish()
{
}

void RTP::handleMessage(cMessage* msg)
{
  if (msg->isSelfMessage())
  {
    //handle self message i.e. rtcp tx timeout
    simtime_t T = calculateTxInterval();
    simtime_t now = simTime();
    if (tp + T <= now) 
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
      sendToUDP(rtcpPayload, port+1, destAddrs[0], port+1);
      int dgramSize = rtcpPayload->length() + 8 + 40;
      meanRtcpSize = (1/16)*dgramSize + 15/16* meanRtcpSize;
      //pg. 42/43 - how to use sender reports to calculate loss rates/ no. of packets received
      //keep count of whether any RTP sent if after 2 intervals none sent set
      //weSent = false;
      initial = false;
      tp = now;
      T = calculateTxInterval();
    }
    scheduleAt(now + T, msg);
    pmembers = members;
    //up there or here?
    //meanRtcpSize = (1/16)*dgram->length() + 15/16* meanRtcpSize;
  }
  else
  {
    processReceivedPacket(msg);
    delete msg;
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
