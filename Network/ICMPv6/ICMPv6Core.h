// -*- C++ -*-
// Copyright (C) 2002, 2003, 2004 CTIE, Monash University
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//

/**
   @file ICMPv6Core.h
   @brief Definition of ICMPv6Core Simple Module class.
   @author Johnny Lai
*/

#ifndef ICMPv6CORE_H
#define ICMPv6CORE_H

#include <map>

#include "QueueBase.h"
#include "ipv6_addr.h"

class ICMPv6Message;
class IPv6Datagram;
class RoutingTable6;
class InterfaceTable;
class PingPayload;

/**
   @class ICMPv6Core
   @brief Module for Processing of ICMPv6 messages
   Processing of received ICMP messages from network nodes and local modules
*/
class ICMPv6Core : public QueueBase
{
public:
  Module_Class_Members(ICMPv6Core, QueueBase, 0);

  ~ICMPv6Core();

  virtual void initialize();
  virtual void endService(cMessage* msg);
  virtual void finish();

private:
  ///Process local errors and send ICMP messages back to
  //originator of errored PDU
  void processError (cMessage *);

  /// process received ICMP messages
  void processICMPv6Message(IPv6Datagram *);

  /// Notify upper layer of ICMP error messages received
  void sendToErrorOut(ICMPv6Message *);

  ///@name Echo messages
  //@{
  ///received echo requests are processed
  void processEchoRequest (ICMPv6Message *request,
                           const ipv6_addr& src,
                           const ipv6_addr& dest,
                           int hopLimit);
  ///keep statistics
  void updatePingStats(PingPayload *payload, const ipv6_addr& src);
  ///received echo replies are processed
  void processEchoReply (ICMPv6Message *reply,
                         const ipv6_addr& src,
                         const ipv6_addr& dest,
                         int hopLimit);
  ///Initiating an echo dialogue
  void sendEchoRequest(cMessage *);
  //@}

  ///Send ICMP message to dest
  void sendToIPv6(ICMPv6Message *, const ipv6_addr& dest,
                           const ipv6_addr& src = IPv6_ADDR_UNSPECIFIED,
                           size_t hopLimit = 0);

  template <class ForwardIterator>
  void recordStats(ForwardIterator first, ForwardIterator last);

  ///@name Ned Parameters
  //@{
  bool icmpRecordStats;
  bool replyToICMPRequests;
  simtime_t icmpRecordStart;
  //@}

  struct PingRecord{
    cOutVector* pingDelay;
    //Differs from the ping apps drop count which is cumulative
    cOutVector* pingDrop;
    cStdDev* stat;
    unsigned int dropCount;
    unsigned short nextEstSeqNo;
    unsigned int outOfOrderArrivalCount;
  };

  typedef std::map<ipv6_addr, PingRecord> PingRecords;
  //Record statistics of all srcs that sent a ping request to us
  PingRecords pingRecords;

  InterfaceTable *ift;
  RoutingTable6 *rt;

  unsigned int ctrIcmp6OutEchoReplies;
  unsigned int ctrIcmp6InMsgs;
  unsigned int ctrIcmp6InEchos, ctrIcmp6InEchoReplies;
  unsigned int ctrIcmp6OutDestUnreachable, ctrIcmp6OutPacketTooBig;
  unsigned int ctrIcmp6OutTimeExceeded, ctrIcmp6OutParamProblem;
  unsigned int ctrIcmp6InDestUnreachable, ctrIcmp6InPacketTooBig;
  unsigned int ctrIcmp6InTimeExceeded, ctrIcmp6InParamProblem;

};

#endif

