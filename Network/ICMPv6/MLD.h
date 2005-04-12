//
// Copyright (C) 2001, 2002 CTIE, Monash University
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

/**
 *  @file MLD.h
 *  @brief Manage requests to the routing table and the interface table
 *
 *  @author Chainarong Kit-Opas
 *
 *  @date    28/11/2002
 *
 */


#include <list>

#include <omnetpp.h>
#include "ipv6_addr.h"
#include "MLDv2Record.h"

class MLD;
class MLDMessage;
class MLDv2Message;
class MLDv2Record;
class IPv6Address;
class cTimerMessage;
class RoutingTable6;
class InterfaceTable;

template< class Result, class T, class Arg>
class cTTimerMessageA;

namespace
{
  // global constants
  const unsigned int robustVar = 2;
  const unsigned int queryInt = 125;
  const unsigned int responseInt = 10;
  const unsigned int lastlistenerInt = 1;

  const unsigned int mul_listener_Int = (robustVar*queryInt)+responseInt;

  const unsigned int MAX_RESPONES_CODE = 10000;
  const unsigned int LLQI_RESPONES_CODE = 1000;

  //Router Timers
  const int Tmr_SendGenQuery = 1;
  const int Tmr_MulticastListenerInterval = 2;
  //Host Timer
  typedef cTTimerMessageA<void, MLD, IPv6Address> ReportTmr;
  const int Tmr_Report = 3;
}

/**
 * Multicast Listener Discovery (MLD) for IPv6.
 * RFC 2710, updated by RFC 3590, RFC 3810.
 */
class MLD: public cSimpleModule
{
public:
  Module_Class_Members(MLD,cSimpleModule,0);
  virtual void initialize();
  virtual void handleMessage(cMessage* msg);
  virtual void finish();

  void sendToIPv6(MLDMessage *msg, const ipv6_addr& dest, const ipv6_addr& src=IPv6_ADDR_UNSPECIFIED, size_t hopLimit=1);

  void startReportTimer(const ipv6_addr& addr, unsigned int responseInterval = responseInt);
  void removeRtEntry(const ipv6_addr& addr);
  void sendReport(const ipv6_addr& addr);
  void sendDone(const ipv6_addr& addr);
  void sendOwnReportMsg(const ipv6_addr& addr);
  void processRouterMLDMessages(cMessage* msg);
  void processNodeMLDMessages(cMessage* msg);

//  void MLDv2NodeParsing(cMessage* Qmsg);

  void MLDv2GenQuery();
  void MLDv2MASQ();
  void MLDv2MASSQ(ipv6_addr MA, SARecord_t* headSAR);
  void MLDv2ReportGQ();
  void MLDv2ReportMASQ();
  void ProcessMASSQ(MLDv2Message* mldmsg);
  void MLDv2ReportMASSQ();
  void MLDv2ChangeReport(MLDv2Record* headChange);
  void MLDv2RouterParsingMsg(MLDv2Message* mldmsg,ipv6_addr SrcAddr);
  void MLDv2CheckTimer();
  void MLDv2QueueMASSQ();
  void MLDv2sendIPdgram(MLDv2Message *msg, const ipv6_addr& dest, const ipv6_addr& src, size_t hopLimit);

  // MASSQ task List Operation
/*  QueueMASSQ_t* searchMASSQ(ipv6_addr MA);
  bool addMASSQ(ipv6_addr MA, SARecord_t* ListSA);
  bool delMASSQ(ipv6_addr MA);
  void destroyMASSQ();*/

  void SendDummy();
  void SendJoin();
  void SendRandJoin();
  void SendBlock();
  void SendAllBlock();
private:
  InterfaceTable *ift;
  RoutingTable6 *rt;

  void sendMASQuery(const ipv6_addr& addr);
//  void sendGenQuery(cTimerMessage* queryTmr);
  void sendGenQuery();

  int reportCounter;
  int masqueryCounter;
  int genqueryCounter;

  cTimerMessage *findTmr(int MLDTimerType, const ipv6_addr& multicast_addr);
  cTimerMessage *resetTmr(int MLDTimerType);
  typedef std::list<cTimerMessage*> TimerMsgs;
  TimerMsgs tmrs;

 private:
  simtime_t CheckTimerInterval;

  // flag
  bool MLDv2_On;

//  QueueMASSQ_t* MASSQtask;
//  int nMASSQtask;

  // Router type
  char RouterType;

  // MLDv2 Record
  int LSTsize;               // Listener State Table Size
  MLDv2Record* LStable;      // Listener State Table
  MLDv2Record* MASSQtable;   // for Router
  MLDv2Record* ReportTable;  // for Nodes

  // for application flow emulating
  double kbs;
  double SampleingRate;
  int AppDataSize;
};

