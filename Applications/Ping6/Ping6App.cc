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
   @file Ping6App.cc

   Purpose: Implementation of Simple ping process that generates ping requests
   and calculates the packet loss and round trip parameters.
   @date 02.02.02
*/

#include "sys.h"
#include "debug.h"
#include "config.h"

#include <climits>
#include <cstdlib>
#include <iomanip>
#include <boost/cast.hpp>
#include <iostream>

#include "Ping6App.h"
#include "Ping6Iface.h"
#include "IPv6InterfacePacketWithData.h"
#include "IPv6Headers.h"
#include "opp_utils.h"
#include "RoutingTable6.h"
#include "TimerConstants.h"

#include "opp_akaroa.h"
#if USE_AKAROA
#include <akaroa/ak_message.H>
#endif //USE_AKAROA
const int SCH_TRANSMIT_PING = 1001;
const int TRANSMIT_PING = 1002;
const int SCH_END = 1003;

Define_Module( Ping6App );

void Ping6App::initialize()
{
  dest = c_ipv6_addr(static_cast<const char*> (par("destination")));
  src = c_ipv6_addr(static_cast<const char*> (par("source")));

  interval = par("interval");
  packetSize = par("packetSize");
  hopLimit = par("hopLimit");
  assert(hopLimit <= MAX_HOPLIMIT);
  count = par("count");
  deadline = par("deadline");
  startTime = par("startTime");
  printPing = par("printPing");

  rt = static_cast<RoutingTable6*> (OPP_Global::findModuleByName(this, "routingTable6"));
  assert( rt != 0);

  dropCount = 0;

  stat = new cStdDev("PingStat");
  pingDelay = new cOutVector("pingRTT");
  //Per handover
  pingDrop = new cOutVector("pingDrop");
//  handoverLatency = new cOutVector("handoverLatency");

  isFinish = false;
  seqNo = 0;
  min = 0;
  max = 0;
  avg = 0;
  nextEstSeqNo = 0;
  lastReceiveTime = 0;
  hostid = id(); // get unique number for Id

  if ( deadline == 0)
    return;

  cMessage* msg = new cMessage;
  msg->setKind(SCH_TRANSMIT_PING);
  msg->setName("PING6_STARTER");
  scheduleAt(ZERO_WAIT_DELAY, msg);

  schSend = new cMessage;
  schSend->setKind(TRANSMIT_PING);
  schSend->setName("PING_REQUEST_SCHEDULER");
}

void Ping6App::finish()
{
  if (!isFinish && nextEstSeqNo > 0)
  {
    isFinish = true;
    printSummery();
  }
}

void Ping6App::handleMessage(cMessage* msg)
{
  if ( msg->isSelfMessage() && msg->kind() == SCH_TRANSMIT_PING )
    scheduleSendPing();
  else if ( msg->isSelfMessage() && msg->kind() == TRANSMIT_PING )
  {
    sendPing();
    return;
  }
  else if ( msg->isSelfMessage() && msg->kind() == SCH_END)
  {
    if (nextEstSeqNo > 0)
      printSummery();
    //delete schSend;
    //Fix opp 3 warning message about drop but drop is protected?
    //simulation.msgQueue.dropAndDelete(schSend);
    //but opp will delete for us anyway
  }
  else
    receivePing(msg);

  delete msg;
}

void Ping6App::scheduleSendPing(void)
{
  Dout(dc::notice, "Ping: starting at "<<startTime<<" from "<<rt->nodeName()<<" to "
       << dest );

  assert(schSend && !schSend->isScheduled());
  scheduleAt(startTime, schSend);

  cMessage* end = new cMessage;
  end->setKind(SCH_END);
  end->setName("PING6_TERMINATOR");
  scheduleAt(deadline, end);
}

void Ping6App::sendPing(void)
{
  echo_int_info echo_req;

  //Should be one per host. Leave it at this for now
  //int identifier
  echo_req.id = hostid;
  echo_req.custom_data = "My ping data";
  echo_req.setLength(packetSize);
  echo_req.hopLimit = static_cast<unsigned char> (hopLimit);
  echo_req.seqNo = seqNo;
  echo_req.sendingTime = simTime();

  //Source Address should be filled in by Routing rules or by omnet.ini
  //paramter.  Dest has to be filled in.
  IPv6InterfacePacketWithData<echo_int_info>*
    app_req = new IPv6InterfacePacketWithData<echo_int_info> (echo_req, src, dest);
  app_req->setName("PING6_REQ");
  app_req->addPar("ipv6");
  send(app_req, "pingOut");

  Dout(dc::ping6, rt->nodeName()<<" "<<simTime()<<" sending ping packet dropCount="<<dropCount
       <<" echo_resp.seqNo="<<echo_req.seqNo<<" nextEstSeqNo="<<nextEstSeqNo);
  seqNo++;

  if ( (simTime() + interval) > deadline)
    return;

  if (count && seqNo > count)
    return;

  assert(schSend && !schSend->isScheduled());
  scheduleAt(simTime()+interval, schSend);
}

void Ping6App::receivePing(cMessage* msg)
{
  if ( isFinish )
  {
    Dout(dc::notice, rt->nodeName()<<" "<<simTime()
         <<" ping packets arrived after deadline seq="
         <<(check_and_cast<IPv6InterfacePacketWithData<echo_int_info>* >(msg))->data().seqNo);
    return;
  }

  IPv6InterfacePacketWithData<echo_int_info> *app_reply = 0;
  echo_int_info echo_resp;

  app_reply = check_and_cast<IPv6InterfacePacketWithData<echo_int_info>* >
    (msg);
  app_reply->setName("PING6_REPLY");

  echo_resp = app_reply->data();

  if (echo_resp.seqNo > nextEstSeqNo)
  {
    Dout(dc::ping6, rt->nodeName() <<" "<<simTime()<<" expected seqNo mismatch, droppedCount="<<dropCount<<" echo_resp.seqNo="<<echo_resp.seqNo
         <<" nextEstSeqNo="<<nextEstSeqNo );

    //@todo fix duplicate ping responses and the hack for dropCount below Don't
    //know why I'm getting two ping responses sometimes when I only send 1 ping
    //request. This happens around seqNo 4020 for MIPv6FastRANetwork. Perhaps
    //some node has buffered and sent it twice or other module has resent the
    //same message twice or duplicated message and sent both?

//    assert(echo_resp.seqNo != nextEstSeqNo - 1);
    if (echo_resp.seqNo == nextEstSeqNo - 1)
    {
      Dout(dc::ping6, rt->nodeName()<<" Duplicate packet detected and dropped " << "\t"
       << simTime() << "\t echo_resp.seqNo=" <<echo_resp.seqNo
           <<" badDropCount="<< (unsigned short)(echo_resp.seqNo - nextEstSeqNo));
      return;
    }

    //This simple algorithm does not handle out of order packets though and will
    //instead give a very big difference. I believe the simulation would not run
    //for a long time past the 2^16 mark without receiving a packet so perhaps I
    //just ignore all packets that have seqNo < nextEstSeqNo now unless we are close
    //to the rollover mark(not handling this case yet)?

    dropCount = dropCount + (unsigned short)(echo_resp.seqNo - nextEstSeqNo);
    Dout(dc::ping6, rt->nodeName() << "\t" << simTime() << "\tdroppedPackets="
         << (unsigned short)(echo_resp.seqNo - nextEstSeqNo));
    Dout(dc::statistic|flush_cf, rt->nodeName()<<" "<<simTime()<<" PingHandoverDropped="<<(unsigned short)(echo_resp.seqNo - nextEstSeqNo));
    pingDrop->record((unsigned short)(echo_resp.seqNo - nextEstSeqNo));
    //estimate as possible that packets are dropped when not handing over
    //(foolproof method is to test the default router to see if it has changed
    //before deciding to record)
//    handoverLatency->record(simTime() - lastReceiveTime);
//    Dout(dc::statistic|flush_cf, rt->nodeName()<<" "<<simTime()<<" HandoverLatency="
//         <<simTime() - lastReceiveTime);
#if USE_AKAROA
//    AkObservation(1, (unsigned short)(echo_resp.seqNo - nextEstSeqNo));
    //AkObservation((unsigned short)(echo_resp.seqNo - nextEstSeqNo));
    AkParamObservation(2, (double)((unsigned short)(echo_resp.seqNo - nextEstSeqNo)));
    //AkMessage("Why not recording this observation Akaroa! %d",
//              (unsigned short)(echo_resp.seqNo - nextEstSeqNo));
    AkParamObservation(1, simTime() - lastReceiveTime);
#endif //USE_AKAROA
  }

  if (echo_resp.seqNo <  nextEstSeqNo)
  {
    Dout(dc::statistic|flush_cf, rt->nodeName()<<" "<<simTime()<<" out of order reply arrived"
         <<" with seq="<<echo_resp.seqNo<<" when nextEstSeqNo="<<nextEstSeqNo);
  }
  else
  {
    stat->collect(simTime() - echo_resp.sendingTime);
    pingDelay->record(simTime() - echo_resp.sendingTime);
    nextEstSeqNo = echo_resp.seqNo + 1;
  }

  //Check for min/max transmission
  if (min == 0)
  {
    min = simTime() - echo_resp.sendingTime;
    max = simTime() - echo_resp.sendingTime;
    avg = min;
  }
  else
  {
    if (min > simTime() - echo_resp.sendingTime)
      min = simTime() - echo_resp.sendingTime;
    else if (max < simTime() - echo_resp.sendingTime)
      max = simTime() - echo_resp.sendingTime;
    //Running average calc?
    avg = (avg + simTime() - echo_resp.sendingTime)/2;
  }

//   cout << this << "\t" << simTime() << "\t"
//        << simTime() - echo_resp.sendingTime << endl;

  Dout(dc::statistic|flush_cf, rt->nodeName()<<" pingAppRTTDelay \t"
    <<simTime()<<"\t"<<simTime() - echo_resp.sendingTime);
#if USE_AKAROA
  AkObservation(3, simTime() - echo_resp.sendingTime);
#endif //USE_AKAROA

  lastReceiveTime = simTime();

  if (printPing)
    cout<<echo_resp.length() + 8<<dec<<" bytes from " <<app_reply->srcAddr()
        <<" icmp_seq="<<echo_resp.seqNo<<" ttl="<<(size_t)echo_resp.hopLimit
        <<" time="<< (simTime() - echo_resp.sendingTime ) * 1000 << " msec"
        <<" ( ping request by "<< fullPath() << ")"<<endl;
#if defined CWDEBUG
  Dout(dc::ping6, echo_resp.length() + 8<<dec<<" bytes from " <<app_reply->srcAddr()
       <<" icmp_seq="<<echo_resp.seqNo<<" ttl="<<(size_t)echo_resp.hopLimit
       <<" time="<< (simTime() - echo_resp.sendingTime ) * 1000 << " msec"
       <<" ( ping request by "<< rt->nodeName() << ")");
#endif //defined CWDEBUG
}

void Ping6App::printSummery(void)
{
  cout <<"--------------------------------------------------------" <<endl;
  cout <<"\t"<<fullPath()<<endl;
  cout <<"--------------------------------------------------------" <<endl;

  if (!isFinish)
    cout <<"Ping: deadline reached at "<<simTime()<<"s\n";
  else
    cout <<"Ping: deadline "<<deadline<<"s NOT reached at "<<simTime()<<"s\n";

  //Print out statistics
  cout<<fullPath()<<" ";
  streamsize orig_prec = cout.precision(3);

#if defined __GNUC__ && __GNUC__ >= 3 || defined CXX
  cout <<"%drop rate "<< fixed <<showpoint;
#else
  cout<<"%drop rate ";
#endif //defined __GNUC__ && __GNUC__ >= 3 || defined CXX

  double totalPingPkt = (double)(deadline - startTime)/interval;

  //isFinish means that finish() was called before deadline reached since we set
  //isFinish at finish()
  if (isFinish)
    totalPingPkt = (double)(simTime() - startTime)/interval;

  ///Get the last dropped packets if it is more than 2 since deadline does not
  ///wait for outstanding ping packets. However do not use the approximation if
  ///nextEstSeq is 0 i.e. never received a ping packet so should have 100% drop
  if (nextEstSeqNo != 0)
    if (seqNo > nextEstSeqNo + 2)
    {
      Dout(dc::ping6, rt->nodeName() <<" "<<simTime()
           <<" Outstanding packets at end droppedCount="<<dropCount<<" next sent seqNo="<<seqNo <<" nextEstSeqNo="<<nextEstSeqNo );
      dropCount = dropCount + (unsigned short)(seqNo - (nextEstSeqNo+2));
      pingDrop->record((unsigned short)(seqNo - (nextEstSeqNo + 2)));
    }


  cout<<((double) dropCount/(double) totalPingPkt)*100.0<<'%' <<'\n';
  //cout<<"round-trip min/avg/max = "<<min*1000.0<<"ms/"<<avg*1000.0<<"ms/"<<max*1000.0<<"ms"<<endl;
  cout<<"round-trip min/avg/max = "<<stat->min()*1000.0<<"ms/"
      <<stat->mean()*1000.0<<"ms/"<<stat->max()*1000.0<<"ms"<<endl;
  cout<<"stddev="<<stat->stddev()*1000.0<<"ms variance="<<stat->variance()*1000.0<<"ms\n";
  cout <<"--------------------------------------------------------" <<endl;
  cout.precision(orig_prec);

  stat->recordScalar("Ping roundtrip delays");

  isFinish = true;
}
