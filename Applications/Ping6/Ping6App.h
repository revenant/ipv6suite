// -*- C++ -*-
//
// Copyright Copyright (C) 2001, 2003, 2004 Johnny Lai
// Monash University, Melbourne, Australia
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

/**
   @file Ping6App.h

   @brief Simple ping process that generates ping requests and calculates the
   packet loss and round trip parameters.
   @date 02.02.02
*/


#include "ipv6_addr.h"
#include <omnetpp.h>

extern const int TRANSMIT_PING;

class RoutingTable6;

class Ping6App: public cSimpleModule
{
 public:
  Module_Class_Members(Ping6App, cSimpleModule, 0);
  virtual void initialize();
  virtual void handleMessage(cMessage* msg);
  virtual void finish();
 private:

  RoutingTable6* rt;

  ///@name Ned Parameters
  //@{
  ipv6_addr dest;
  ///Choose outgoing source ip address, default is to determine based on
  ///destination address. All '0's will set src to default
  ipv6_addr src;
  ///Wait for x seconds before sending next ping request default is 1 second
  double interval;
  ///Default is 56 bytes of payload
  size_t packetSize;
  ///For multicast packets
  size_t hopLimit;
  ///Stop after count ping requests are sent, 0 means continuously
  size_t count;
  ///Exit this process after deadline seconds have elapsed, 0 means never stop
  size_t deadline;
  ///Begin pinging at startTime according to simulation clock, 0 means disable
  double startTime;
  ///Enable ping command like output to stdout?
  unsigned int printPing;
  //@}

  void sendPing(void);
  void receivePing(cMessage* msg);
  void scheduleSendPing(void);

  void printSummery(void);

private:
  cStdDev* stat;
  cOutVector* pingDelay;
  cOutVector* pingDrop;
  cOutVector* handoverLatency;

  int dropCount;
  bool isFinish;
  unsigned long seqNo;
  unsigned long nextEstSeqNo;
  simtime_t lastReceiveTime;
  unsigned int hostid;

  //roundtrip
  double min;
  double max;
  double avg;

  cMessage* schSend;
};


