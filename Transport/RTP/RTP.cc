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
 * @todo dgramSize, Separation of RTP Apps like VoIP from RTP primitives
 *  
 */

#include <iomanip>
#include <cassert>

#include "RTP.h"
#include "IPAddressResolver.h"
#include "UDPControlInfo_m.h"
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

typedef std::vector<IPvXAddress> AddressVector;

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

//this version records it as lost packets
void RTP::handleMisorderedOrDroppedPackets(RTPMemberEntry *s, u_int16 udelta)
{
  initialiseStats(s, this);
  s->lossVector->record(udelta);
  recordHOStats(s, this);
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
      //drop. 
      rtp->handleMisorderedOrDroppedPackets(s, udelta);
    }
      
    s->maxSeq = seq;
  } else if (udelta <= RTP_SEQ_MOD - MAX_MISORDER) {
    /* the sequence number made a very large jump */
    if (seq == s->badSeq) {
      //don't want a total resync because we know otherside does not do a real
      //resync in my sim and we want to preserve our stats

      /*
       * Two sequential packets -- assume that the other side
       * restarted without telling us so just re-sync
       * (i.e., pretend this was the first packet).
       */
      init_seq(s, seq);
      EV<<"sequence reinitialised as two sequential packets from other side after huge jump so resyncing";
      assert(false);
    }
    else {
      s->badSeq = (seq + 1) & (RTP_SEQ_MOD-1);
      return 0;
    }
  } else {
    /* duplicate or reordered packet */
    assert(false);
  }
  s->received++;
  return 1;
}

void RTP::processRTPData(RTPPacket* rtpData, RTPMemberEntry &rme)
{
  //empty
}

void RTP::processRTP(RTPPacket* rtpData)
{
  UDPControlInfo *ctrl = check_and_cast<UDPControlInfo *>(rtpData->controlInfo());

  IPvXAddress srcAddr = ctrl->getSrcAddr();
  IPvXAddress destAddr = ctrl->getDestAddr();

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

  processRTPData(rtpData, rme);

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
}

void RTP::processGoodBye(RTCPGoodBye* rtcp)
{
  //if bye received remove entry from members/senders map 
  //ignore reverse reconsideration 6.3.4 (code in pg 93)
  if (memberSet.count(rtcp->ssrc()))
  {
    UDPControlInfo *ctrl = check_and_cast<UDPControlInfo *>(rtcp->controlInfo());
    IPvXAddress srcAddr = ctrl->getSrcAddr();

    members-=1;
    if (memberSet[rtcp->ssrc()].sender)
      senders-=1;
    
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
    memberSet.erase(rtcp->ssrc());
  }
}

void RTP::processReports(RTCPReports* rep)
{
  RTCPSR* sr = dynamic_cast<RTCPSR*> (rep);
  if (sr)
  {
    memberSet[rep->ssrc()].lastSR = sr->timestamp();
    incomingBlocks.resize(0);
    for (u_int32 i = 0; i < sr->reportBlocksArraySize(); ++i)
      incomingBlocks.push_back(sr->reportBlocks(i));
  }
  else
  {
    RTCPRR* rr = dynamic_cast<RTCPRR*>(rep);
    incomingBlocks.resize(0);
    for (u_int32 i = 0; i < rr->reportBlocksArraySize(); ++i)
      incomingBlocks.push_back(rr->reportBlocks(i));
  }	
}

void RTP::processSDES(RTCPSDES* sdes)
{
  //does nothing
}

void RTP::processRTCP(RTCPPacket* rtcp)
{
  UDPControlInfo *ctrl = check_and_cast<UDPControlInfo *>(rtcp->controlInfo());
  IPvXAddress srcAddr = ctrl->getSrcAddr();

  if (!memberSet.count(rtcp->ssrc()))
    {
      //may wait for many before consider valid but we'll skip that
      members+=1;
      memberSet[rtcp->ssrc()].sender = false;
      memberSet[rtcp->ssrc()].addr = srcAddr;
    }

    //further rtcp types like reports what to do etc.
    //(Sec 6.4 for analysing reports)

    if (dynamic_cast< RTCPGoodBye*> (rtcp))
    {
      processGoodBye(static_cast<RTCPGoodBye*>(rtcp));
    }
    else if (dynamic_cast<RTCPReports*> (rtcp))
    {
      processReports(static_cast<RTCPReports*>(rtcp));
    }
    else if (dynamic_cast<RTCPSDES*> (rtcp))
    {
      processSDES(static_cast<RTCPSDES*> (rtcp));
    }
    else
    {
      opp_error("Unknown/Unhandled RTCP type name=%s", rtcp->name());
    }
}

void RTP::processReceivedPacket(cMessage* msg)
{
  UDPControlInfo *ctrl = check_and_cast<UDPControlInfo *>(msg->controlInfo());

  //  int srcPort = ctrl->getSrcPort();
  int destPort = ctrl->getDestPort();

  if (destPort == port) //RTP
  {
    assert( dynamic_cast<RTPPacket*>(msg));
    RTPPacket* rtpData = static_cast<RTPPacket*> (msg);
    processRTP(rtpData);
  } //RTP message
  else //RTCP
  {
    assert(dynamic_cast< RTCPPacket*>(msg));
    meanRtcpSize = ((double)1/16)*(double)dgramSize(msg) + ((double)15/16)*meanRtcpSize;
    RTCPPacket* rtcp = static_cast< RTCPPacket*>(msg);
    processRTCP(rtcp);   
  } //RTCP message
}

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
    sendBye();
  }
  else
  {
    cCallbackMessage* bye = new cCallbackMessage("send bye leak");
    (*bye) = boost::bind(&RTP::sendBye, this);
    bye->rescheduleDelay(T);
  }

  if (rtcpTimeout->isScheduled())
    cancelEvent(rtcpTimeout);
  if (rtpTimeout->isScheduled())
    cancelEvent(rtpTimeout);
}

void RTP::sendBye()
{
  RTCPPacket* bye = new RTCPGoodBye(_ssrc);
  for (unsigned int i =0; i < destAddrs.size(); ++i)
  {
    sendToUDP(bye, port+1, destAddrs[i], port+1);
  }
  //doesn't look right
  //meanRtcpSize = dgramSize(bye);
}

void RTP::establishSession()
{
  if (destAddrs.empty())
  {
    resolveAddresses();
  }

  if (destAddrs.empty())
    return;

  //conference type of app
  //send a cname packet (i.e. user@fullpath etc.) inside first rtcp packet  
  for (unsigned int i =0; i < destAddrs.size(); ++i)
    sendToUDP(new RTCPSDES(_ssrc, OPP_Global::nodeName(this)),
	      port+1, destAddrs[i], port+1);

  scheduleAt(simTime() + packetisationInterval, rtpTimeout);
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




RTP::RTP():rtpTimeout(0),rtcpTimeout(0),nb(0),l2down(0),mobileNode(false)
{}

RTP::~RTP()
{
  if (rtpTimeout->isScheduled())
    cancelEvent(rtpTimeout);
  if (rtcpTimeout->isScheduled())
    cancelEvent(rtcpTimeout);
  delete rtpTimeout;
  delete rtcpTimeout;
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

  simtime_t startTime = par("startTime");

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

 
  payloadLength = par("payloadLength").longValue();
  if (payloadLength > 900)
    opp_error("payload length is greater than 900, not sure if fits MUT");

  packetisationInterval = par("packetisationInterval").doubleValue();

  rtcpBw = (double)par("rtcpBandwidth"); 
  double bitrate = (1.0/(double)packetisationInterval)*payloadLength;
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
  scheduleAt(randomStart < 4? 4 + randomStart:randomStart, rtcpTimeout);

  if (startTime)
  {    
    rtpTimeout = new cMessage("rtpTimeout");
    
    cCallbackMessage* startSession = new cCallbackMessage("sessStart leak");
    (*startSession) = boost::bind(&RTP::establishSession, this);
    scheduleAt(startTime, startSession);
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
  assert(rtpTimeout);

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

//create a fn in Network layer that will simulate the sending process and return
//calculated bytes without actually sending message?
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
    IPvXAddress addr;
    //fill rdns cache in case no one has requested us otherwise labels will
    //appear blank when IPAddressResolver::hostname called
    if (initial)
      if (IPAddressResolver().tryResolve(OPP_Global::nodeName(this), addr))
	IPAddressResolver().resolve(OPP_Global::nodeName(this));
    
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
