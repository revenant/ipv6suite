// -*- C++ -*-
// Copyright (C) 2002, 2003, 2004 CTIE, Monash University
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//

/**
   @file ICMPv6Core.h
   @brief Definition of ICMPv6Core Simple Module class.
   @author Johnny Lai
*/

#if !defined ICMPv6CORE_H
#define ICMPv6CORE_H

#ifndef __CPACKET_H
#include "cpacket.h"
#endif //__CPACKET_H



#ifndef IPv6_ADDR_H
#include "ipv6_addr.h"
#endif //IPv6_ADDR_H


/**
   @class ICMPv6Core
   @brief Module for Processing of ICMPv6 messages
   Processing of received ICMP messages from network nodes and local modules
*/

class ICMPv6Message;
class IPv6Datagram;
class RoutingTable6;
class IPv6Forward;

class ICMPv6Core : public cSimpleModule
{
public:
  Module_Class_Members(ICMPv6Core, cSimpleModule, 0);//ACTIVITY_STACK_SIZE);

  virtual void initialize();
  virtual void activity();
  void handleMessage(cMessage* theMsg);
  virtual void finish();

private:
      ///Process local errors and send ICMP messages back to
      //originator of errored PDU
  void processError (cMessage *);

      /// process received ICMP messages
  void processICMPv6Message(IPv6Datagram *);

  /// Notify upper layer of ICMP error messages received
  void errorOut(ICMPv6Message *);

      ///@name Echo messages
      //@{
      ///received echo requests are processed
  void recEchoRequest (IPv6Datagram *);
      ///received echo replies are processed
  void recEchoReply (IPv6Datagram* reply);
      ///Initiating an echo dialogue
  void sendEchoRequest(cMessage *);
      //@}

      ///Send ICMP message to dest
  void sendInterfacePacket(ICMPv6Message *, const ipv6_addr& dest,
                           const ipv6_addr& src = IPv6_ADDR_UNSPECIFIED,
                           size_t hopLimit = 0);

  ///@name Ned Parameters
  //@{
  simtime_t delay;
  bool icmpRecordStats;
  bool replyToICMPRequests;
  simtime_t icmpRecordStart;
  cStdDev* stat;
  //@}

  cOutVector* pingDelay;
  cOutVector* pingDrop;
  cOutVector* handoverLatency;
  int dropCount;
  unsigned short nextEstSeqNo;
  simtime_t lastReceiveTime;

  RoutingTable6* rt;
  IPv6Forward* fc;
  cMessage* curMessage;
  cMessage* waitTmr;
  ///Arriving packets are placed in queue first if another packet is awaiting
  ///processing
  cQueue waitQueue;

  unsigned int ctrIcmp6OutEchoReplies;
  unsigned int ctrIcmp6InMsgs;
  unsigned int ctrIcmp6InEchos, ctrIcmp6InEchoReplies;
  unsigned int ctrIcmp6OutDestUnreachable, ctrIcmp6OutPacketTooBig;
  unsigned int ctrIcmp6OutTimeExceeded, ctrIcmp6OutParamProblem;
  unsigned int ctrIcmp6InDestUnreachable, ctrIcmp6InPacketTooBig;
  unsigned int ctrIcmp6InTimeExceeded, ctrIcmp6InParamProblem;

};

#endif

