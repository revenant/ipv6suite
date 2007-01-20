// -*- C++ -*-
// Copyright (C) 2006 Johnny Lai
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
 * @file RTP.h
 * @author 
 * @date 29 Jul 2006
 *
 * @brief Definition of class RTP
 *
 */

#ifndef RTP_H
#define RTP_H

#ifndef __OMNETPP_H
#include <omnetpp.h>
#endif //__OMNETPP_H

#ifndef TYPES_TYPEDEF_H
#include "types_typedef.h"
#endif
#ifndef __UDPAPPBASE_H__
#include "UDPAppBase.h"
#endif
#ifndef MAP
#define MAP
#include <map>
#endif
#ifndef _RTPPACKET_M_H_
#include "RTCPPacket_m.h"
#endif

#ifndef __INOTIFIABLE_H
#include "INotifiable.h"
#endif

struct RTPMemberEntry
{
  RTPMemberEntry();
  bool sender;
  //send reports to these guys
  IPvXAddress addr;
  //maximum seqNo seen so far
  u_int16 maxSeq;
  //baseSeq is 0 since we do not randomise starting seqNo
  //shifted count of seqNo cycles
  u_int32 cycles;
  //last 'bad' seqNo + 1
  u_int32 badSeq; 
  u_int32 received;
  u_int32 expectedPrior;
  u_int32 receivedPrior;
  //transit time of last RTP data packet received i.e.
  //time at arrival - packet's rtp timestamp
  simtime_t transit;
  //last jitter calculated
  simtime_t jitter;
  //time when last SR was received from this sender
  simtime_t lastSR;
  cOutVector* transVector;
  cStdDev* transStat;
  //recording likely loss just like ping app (which cannot accept out of order
  //arrivals but rtp can up to playout buffer size).  Cumulative loss as
  //determined by expected - received in reports is more accurate
  cOutVector* lossVector;
  cStdDev* handStat;
};

std::ostream& operator<<(std::ostream& os, const RTPMemberEntry& rme);

class RTCPReports;
class NotificationBoard;

/**
 * @class RTP
 *
 * @brief 
 *
 * detailed description
 */

class RTP: public UDPAppBase, INotifiable
{
 public:
#ifdef USE_CPPUNIT
  friend class RTPTest;
#endif //USE_CPPUNIT

  ///@name Overidden cSimpleModule functions
  //@{
  int numInitStages() const;

  virtual void initialize(int stageNo);

  virtual void finish();

  virtual void handleMessage(cMessage* msg);

  virtual void receiveChangeNotification(int category, cPolymorphic *details);
  //@}

  //@name constructors, destructors and operators
  //@{
  
  RTP();

  ~RTP();
  
  //@}

  virtual void processReceivedPacket(cMessage* msg);
  virtual void leaveSession();
  virtual void establishSession();
  virtual simtime_t calculateTxInterval();
  virtual bool sendRTPPacket();
 protected:

  void resolveAddresses();

  void fillReports(RTCPReports* rtcpPayload);
  void rtcpTxTimeout();

  //ned params storage
  //@{
  unsigned short port;
  std::vector<IPvXAddress> destAddrs;
  simtime_t startTime;
  //@}

  //Keyed on SSRC
  typedef std::map<unsigned int, RTPMemberEntry> MemberSet;
  MemberSet memberSet;
  typedef MemberSet::iterator MSI;


  unsigned int _ssrc;
  unsigned short _seqNo;
  //no of rtp data packets sent
  unsigned int packetCount;
  //no. of rtp data octets sent
  unsigned int octetCount;

  cMessage* rtcpTimeout;
  cMessage* rtpTimeout;

  //variables for calculation of T i.e. RTCP Report transmission
  //@{
  //last time rtcp tx
  simtime_t tp;
  //next schedule tx of rtcp
  simtime_t tn; //prob. in rtcp timer
  //estimate no. of participants at tn computation
  unsigned short pmembers; 
  //current no. of members
  unsigned short members;
  unsigned short senders;
  //fraction of sessionBandwidth used by all
  float rtcpBw;
  //true when data sent after 2nd last rtcp report tx
  bool weSent;
  //average of all rtcp packets sent & received (i.e. datagram size)
  float meanRtcpSize; // avg_rtcp_size
  //true if no rtcp sent
  bool initial;

 private:

  std::vector<RTCPReportBlock> incomingBlocks;
  NotificationBoard* nb;
 public:
  simtime_t l2down;
  //@}

  
};

std::ostream& operator<<(std::ostream& os, const RTCPReportBlock& rb);

#endif /* RTP_H */

