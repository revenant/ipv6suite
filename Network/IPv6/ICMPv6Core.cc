//
// Copyright (C) 2001, 2004 Eric Wu (eric.wu@eng.monash.edu.au)
// Copyright Copyright (C) 2001, 2004 Johnny Lai
//
// Monash University, Melbourne, Australia
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
   @file ICMPv6Core.cc
   @brief ICMPv6 Processing in section 2.4 of RFC 2463
   @date 13.09.01
*/

#include "sys.h"
#include "debug.h"

#include <memory> //auto_ptr
#include <omnetpp.h>
#include <boost/cast.hpp>

#include "ICMPv6Core.h"
#include "IPv6Datagram.h"
#include "ipv6_addr.h"
#include "IPv6InterfacePacket.h"
#include "IPv6Headers.h"
#include "ICMPv6Message.h"

#include "IPv6InterfaceData.h"
#include "Ping6Iface.h"
#include "opp_utils.h"
#include "RoutingTable6.h" //required for counters only
#include "IPv6Forward.h"

#ifdef TESTIPv6
#undef NDEBUG
#include <cassert>
#include <memory>
#include <iostream>
#endif //TESTIPv6

static const char* NDGATE = "NDiscOut";
static const char* MLDGATE = "MLDOut";


Define_Module( ICMPv6Core );

void ICMPv6Core::initialize()
{

  delay = par("procdelay");
  icmpRecordStats = par("icmpRecordRequests");
  replyToICMPRequests = par("replyToICMPRequests");
  icmpRecordStart = par("icmpRecordStart");
  stat = 0;
  if (icmpRecordStats)
    stat = new cStdDev("PingRequestReceived");

  rt = RoutingTable6Access().get();
  fc = check_and_cast<IPv6Forward*> (
    OPP_Global::findModuleByTypeDepthFirst(this, "IPv6Forward")); // XXX why pointers to other modules? why???? --AV
  assert(fc != 0);
  ctrIcmp6OutEchoReplies = 0;
  ctrIcmp6InMsgs = 0;
  ctrIcmp6InEchos = ctrIcmp6InEchoReplies = 0;

  ctrIcmp6OutDestUnreachable = ctrIcmp6OutPacketTooBig = 0;
  ctrIcmp6OutTimeExceeded = ctrIcmp6OutParamProblem = 0;

  ctrIcmp6InDestUnreachable = ctrIcmp6InPacketTooBig = 0;
  ctrIcmp6InTimeExceeded = ctrIcmp6InParamProblem = 0;

  pingDelay = new cOutVector("pingEED");
  //Per handover
  pingDrop = new cOutVector("pingDrop");
  handoverLatency = new cOutVector("handoverLatency");
  dropCount = 0;
  nextEstSeqNo = 0;
  lastReceiveTime = 0;


  curMessage = 0;
  waitTmr = new cMessage("ICMPv6CoreWait");
  waitQueue.setName("ICMPv6WaitQ");

  //Can't really see from proc module
  std::string display(parentModule()->displayString());
  display += ";q=ICMPv6WaitQ";
  parentModule()->setDisplayString(display.c_str());

  //so display at self
  display = displayString();
  display += ";q=ICMPv6WaitQ";
  setDisplayString(display.c_str());
}

void ICMPv6Core::handleMessage(cMessage* theMsg)
{

#if defined TESTIPv6PING
///Test ping packets and traversing across network/ decapsulate etc.
  IPv6InterfaceData<echo_int_info>* app_req = 0;
  echo_int_info echo_req;
  int increment = 20;
  int pingCount = 1;

  if ( strcmp("client1", OPP_Global::findNetNodeModule(this)->name()) == 0)
  {
    for (int j = 0; j < pingCount; j++)
    {
      echo_req.id = 12345;
      echo_req.seqNo = 88 + j;
      //This dest is probably redundent.
      //echo_req.dest = IPv6Address("fe80:0:0:0:260:97ff:0:4");
      echo_req.dest = c_ipv6_addr("fec0:0:0:ABCD:260:97ff:0:4");
      echo_req.custom_data = "My ping data";
      echo_req.length = (strlen("My ping data"));
      //Source Address should be filled in by Routing rules or by app.  Dest
      //has to be filled in.
      app_req =
        new IPv6InterfaceData<echo_int_info> (echo_req, 0,
                                              "fe80:0:0:0:260:97ff:0:4");

      scheduleAt(simTime() + j + increment, app_req);
      //Simulate real app request
      //sendDirect(app_req, simTime() + i*increment, this, "pingIn");
    }

  }
#endif //TESTIPv6PING

  if (!theMsg->isSelfMessage())
  {
    if (!waitTmr->isScheduled() && curMessage == 0 )
    {
      scheduleAt(delay + simTime(), waitTmr);
      curMessage = theMsg;
      return;
    }
    else if (waitTmr->isScheduled())
    {
      Dout(dc::custom, fullPath()<<" "<<simTime()<<" received new mesage "
           <<" when message was scheduled at waitTmr="<<waitTmr->arrivalTime());
      waitQueue.insert(theMsg);
      return;
    }
    assert(false);
    delete theMsg;
    return;
  }

  assert(curMessage);

  cMessage* msg = curMessage;
  if (waitQueue.empty())
    curMessage = 0;
  else
  {
    curMessage = check_and_cast<cMessage*>(waitQueue.pop());
    scheduleAt(delay + simTime(), waitTmr);
  }

  //checksum testing
  cGate *arrivalGate = msg->arrivalGate();

  /* error message from Routing, PreRouting, Fragmentation
     or IPOutput: send ICMPv6 message */
  if (!strcmp(arrivalGate->name(), "preRoutingIn")
      || !strcmp(arrivalGate->name(), "routingIn")
      || !strcmp(arrivalGate->name(), "fragmentationIn")
      || !strcmp(arrivalGate->name(), "ipOutputIn")
      || !strcmp(arrivalGate->name(), "addrReslnIn")
      || !strcmp(arrivalGate->name(), "localErrorIn"))
  {
    ctrIcmp6InMsgs++;
    processError(msg);
    return;
  }

  // process arriving ICMPv6 message
  if (!strcmp(arrivalGate->name(), "localIn"))
  {
    ctrIcmp6InMsgs++;
    processICMPv6Message(check_and_cast<IPv6Datagram*>(msg));
    return;
  }

  // request from application
  if (!strcmp(arrivalGate->name(), "pingIn"))
  {

    // packing informational request from app layer and
    // tx across the network
    sendEchoRequest(msg);
    return;
  }

#if !defined TESTIPv6PING
  assert(false);
  delete msg;
#else //!defined TESTIPv6PING
  //TEST ICMP PING and Addr Res

  cout<<"Sending Echo request at "<<simTime()<<" "<<hex<<app_req<<endl;
  assert(app_req->destAddress() != IPv6_ADDR_UNSPECIFIED);


  // packing informational request from app layer and
  // tx across the network
  sendEchoRequest(msg);
#endif //!defined TESTIPv6PING

}

void ICMPv6Core::activity()
{
  cMessage *msg;
  cGate *arrivalGate;

#if defined TESTIPv6PING
///Test ping packets and traversing across network/ decapsulate etc.
  IPv6InterfaceData<echo_int_info>* app_req = 0;
  echo_int_info echo_req;
  int increment = 20;
  int pingCount = 1;

  if ( strcmp("client1", OPP_Global::findNetNodeModule(this)->name()) == 0)
  {
    for (int j = 0; j < pingCount; j++)
    {
      echo_req.id = 12345;
      echo_req.seqNo = 88 + j;
      //This dest is probably redundent.
      //echo_req.dest = IPv6Address("fe80:0:0:0:260:97ff:0:4");
      echo_req.dest = c_ipv6_addr("fec0:0:0:ABCD:260:97ff:0:4");
      echo_req.custom_data = "My ping data";
      echo_req.length = (strlen("My ping data"));
      //Source Address should be filled in by Routing rules or by app.  Dest
      //has to be filled in.
      app_req =
        new IPv6InterfaceData<echo_int_info> (echo_req, 0,
                                              "fe80:0:0:0:260:97ff:0:4");

      scheduleAt(simTime() + j + increment, app_req);
      //Simulate real app request
      //sendDirect(app_req, simTime() + i*increment, this, "pingIn");
    }

  }
#endif //TESTIPv6PING


  while(true)
  {
    msg = receive();

    if (!msg->isSelfMessage())
    {
      //Self Messages don't come through a gate
      arrivalGate = msg->arrivalGate();

      wait(delay);

      //checksum testing

      /* error message from Routing, PreRouting, Fragmentation
           or IPOutput: send ICMPv6 message */
      if (!strcmp(arrivalGate->name(), "preRoutingIn")
          || !strcmp(arrivalGate->name(), "routingIn")
          || !strcmp(arrivalGate->name(), "fragmentationIn")
          || !strcmp(arrivalGate->name(), "ipOutputIn")
          || !strcmp(arrivalGate->name(), "addrReslnIn")
          || !strcmp(arrivalGate->name(), "localErrorIn"))
      {
        ctrIcmp6InMsgs++;
        processError(msg);

        continue;
      }

      // process arriving ICMPv6 message
      if (!strcmp(arrivalGate->name(), "localIn"))
      {
        ctrIcmp6InMsgs++;
        processICMPv6Message(check_and_cast<IPv6Datagram*>(msg));

        continue;
      }

      // request from application
      if (!strcmp(arrivalGate->name(), "pingIn"))
      {

        // packing informational request from app layer and
        // tx across the network
        sendEchoRequest(msg);

        continue;
      }
    }
#if defined TESTIPv6PING
    else //TEST ICMP PING and Addr Res
    {
      cout<<"Sending Echo request at "<<simTime()<<" "<<hex<<app_req<<endl;
      assert(app_req->destAddress() != IPv6_ADDR_UNSPECIFIED);


      // packing informational request from app layer and
      // tx across the network
      sendEchoRequest(msg);

      continue;
    }
#endif //TESTIPv6PING
  } // end while
}

void ICMPv6Core::finish()
{
  if (icmpRecordStats)
  {
    streamsize orig_prec = cout.precision(3);

    cout <<"--------------------------------------------------------" <<endl;
    cout <<"\t"<<fullPath()<<endl;
    cout <<"--------------------------------------------------------" <<endl;
    cout<<"eed min/avg/max = "<<stat->min()*1000.0<<"ms/"
        <<stat->mean()*1000.0<<"ms/"<<stat->max()*1000.0<<"ms"<<endl;
    cout<<"stddev="<<stat->stddev()*1000.0<<"ms variance="<<stat->variance()*1000.0<<"ms\n";
    cout <<"--------------------------------------------------------" <<endl;
    cout.precision(orig_prec);

    stat->recordScalar("ping stream eed");

    delete stat;
    stat = 0;
  }
  recordScalar("Icmp6InMsgs", ctrIcmp6InMsgs);
  recordScalar("Icmp6InEchos", ctrIcmp6InEchos);
  recordScalar("Icmp6InEchoReplies",ctrIcmp6InEchoReplies);

  recordScalar("Icmp6OutMsgs", rt->ctrIcmp6OutMsgs);
  recordScalar("Icmp6OutEchoReplies", ctrIcmp6OutEchoReplies);

  recordScalar("Icmp6OutDestinationUnreachable", ctrIcmp6OutDestUnreachable);
  recordScalar("Icmp6OutPacketTooBig", ctrIcmp6OutPacketTooBig);
  recordScalar("Icmp6OutTimeExceeded", ctrIcmp6OutTimeExceeded);
  recordScalar("Icmp6OutParameterProblem", ctrIcmp6OutParamProblem);

  recordScalar("Icmp6InDestinationUnreachable", ctrIcmp6InDestUnreachable);
  recordScalar("Icmp6InPacketTooBig", ctrIcmp6InPacketTooBig);
  recordScalar("Icmp6InTimeExceeded", ctrIcmp6InTimeExceeded);
  recordScalar("Icmp6InParameterProblem", ctrIcmp6InParamProblem);

}

/**
   Process internally detected error conditions. These are represented with ICMP
   messages.  Forward the ICMP message to originator of errored pdu.

   In order to preserve as much information as possible can encapsulate the
   PDU that caused error inside the ICMPv6Message object

   msg should be of type ICMPv6Message

   Process messages according to section 2.4 (e) of RFC2463
*/
void ICMPv6Core::processError(cMessage* msg)
{
  ICMPv6Message* errorMessage = static_cast<ICMPv6Message*> (msg);

  IPv6Datagram *pdu =  check_and_cast<IPv6Datagram *>(errorMessage->encapsulatedMsg());

  // don't send ICMPv6 if the following conditions are met
  if (pdu->destAddress().isMulticast() &&
      errorMessage->type() != ICMPv6_PACKET_TOO_BIG &&
      errorMessage->type() != ICMPv6_PARAMETER_PROBLEM)
  {
    delete msg;
    return;
  }

  // do not reply with error message to error message
  if (pdu->transportProtocol() == IP_PROT_IPv6_ICMP)
  {

    ICMPv6Message* recICMPv6Msg = check_and_cast<ICMPv6Message*> (pdu->encapsulatedMsg());

    if (recICMPv6Msg->isErrorMessage())
    {
      delete( errorMessage );
      return;
    }

  }

  switch( errorMessage->type())
  {
    case ICMPv6_DESTINATION_UNREACHABLE:
      ctrIcmp6OutDestUnreachable++;
      break;

    case ICMPv6_PACKET_TOO_BIG:
      ctrIcmp6OutPacketTooBig++;
      break;

    case ICMPv6_TIME_EXCEEDED:
      ctrIcmp6OutTimeExceeded++;
      break;

    case ICMPv6_PARAMETER_PROBLEM:
      ctrIcmp6OutParamProblem++;
      break;

    default:
      break;
  }

  // notify sender
  assert(pdu->srcAddress() != IPv6_ADDR_UNSPECIFIED);
  sendInterfacePacket(errorMessage, pdu->srcAddress());
  rt->ctrIcmp6OutMsgs++;

  Dout(dc::notice|flush_cf, rt->nodeName()<<" "<<simTime()
       <<" ICMPv6: send error message to "<<pdu->srcAddress()<<" type/code="
       <<(int)errorMessage->type()<<"/"<<errorMessage->code());
}

/**
   Process received ICMP messages
   Do ICMPv6 Processing according to RFC2463 Sec 2.4

   Upper layer is notified of error ICMP messages by forwarding to errorOut
   gate.

   Clause d. If required to pass ICMPv6 error to uppper layer then original
   upper layer protocol is extracted from original PDU (contained inside ICMP
   packet) and used to select appropriate upper layer process to handle error.

   Perhaps the full interfacePacket should be passed to upper layers instead of
   just the ICMPPacket as the src/dest addr can be reused to find out which
   interface to send from.  Even though this information should be contained
   inside the errored PDU which is inside all error ICMP messages.
*/
void ICMPv6Core::processICMPv6Message(IPv6Datagram* dgram)
{

  ICMPv6Message* icmpmsg = check_and_cast<ICMPv6Message*>(dgram->decapsulate());

  switch (icmpmsg->type())
  {
      case ICMPv6_DESTINATION_UNREACHABLE:
        errorOut(icmpmsg);
        ctrIcmp6InDestUnreachable++;
        break;

      case ICMPv6_PACKET_TOO_BIG:
        errorOut(icmpmsg);
        ctrIcmp6InPacketTooBig++;
        break;

      case ICMPv6_TIME_EXCEEDED:
        errorOut(icmpmsg);
        ctrIcmp6InTimeExceeded++;
        break;

      case ICMPv6_PARAMETER_PROBLEM:
        errorOut(icmpmsg);
        ctrIcmp6InParamProblem++;
        break;

      case ICMPv6_ECHO_REQUEST:
        dgram->encapsulate(icmpmsg);
        dgram->setTransportProtocol(IP_PROT_IPv6_ICMP);
        dgram->setName(icmpmsg->name());
        recEchoRequest(dgram);
        dgram = 0;
        ctrIcmp6InEchos++;
        break;

      case ICMPv6_ECHO_REPLY:
        dgram->encapsulate(icmpmsg);
        dgram->setTransportProtocol(IP_PROT_IPv6_ICMP);
        dgram->setName(icmpmsg->name());
        recEchoReply(dgram);
        dgram = 0;
        ctrIcmp6InEchoReplies++;
        break;

      case ICMPv6_MLD_QUERY: case ICMPv6_MLD_REPORT: case ICMPv6_MLD_DONE: case ICMPv6_MLDv2_REPORT:
//      cerr<<"MLD message of type "<<icmpmsg->type()<<endl;
        dgram->encapsulate(icmpmsg);
        dgram->setTransportProtocol(IP_PROT_IPv6_ICMP);
        dgram->setName(icmpmsg->name());
        send(dgram, MLDGATE);
        dgram=0;
        break;

      case ICMPv6_ROUTER_SOL: case ICMPv6_ROUTER_AD:
      case ICMPv6_NEIGHBOUR_SOL:case ICMPv6_NEIGHBOUR_AD:
      case ICMPv6_REDIRECT:
        icmpmsg->encapsulate(dgram);
        //Very important otherwise packet is deleted
        dgram = 0;
        send(icmpmsg, NDGATE);
        break;

      default:
        error("wrong ICMP type %d", (int)icmpmsg->type());
/* XXX changed to error --Andras
        Dout(dc::warning|flush_cf,"*** ICMPv6: type not found! "
             << (int)icmpmsg->type());
        if (icmpmsg->isErrorMessage())
            //Notify Upper layer
          errorOut(icmpmsg);
        else //Discard
          delete(icmpmsg);
*/
  }
  delete dgram;
}

///
void ICMPv6Core::errorOut(ICMPv6Message *icmpmsg)
{
  send(icmpmsg, "errorOut");
}

/*  ----------------------------------------------------------
       Echo request and reply ICMPv6 messages
    ----------------------------------------------------------  */

void ICMPv6Core::recEchoRequest(IPv6Datagram* theRequest)
{
  auto_ptr<IPv6Datagram> request(theRequest);

  assert(request->encapsulatedMsg() != 0);
  ICMPv6Echo* req = check_and_cast<ICMPv6Echo*> (request->encapsulatedMsg());
  assert(req);
  echo_int_info echo_req = check_and_cast<
    IPv6InterfaceData<echo_int_info>* > (req->encapsulatedMsg())->data();

  if (icmpRecordStats)
  {
    if (echo_req.seqNo > nextEstSeqNo)
    {
      Dout(dc::ping6, rt->nodeName() <<" pingecho_req: "<<simTime()<<" expected seqNo mismatch, dropCount="
           <<dropCount<<" echo_req.seqNo="<<echo_req.seqNo<<" nextEstSeqNo="<<nextEstSeqNo
           <<" just dropped="<<echo_req.seqNo - nextEstSeqNo);
      dropCount = dropCount + (unsigned short)(echo_req.seqNo - nextEstSeqNo);
      if (simTime() >= icmpRecordStart)
      {
        pingDrop->record((unsigned short)(echo_req.seqNo - nextEstSeqNo));
        handoverLatency->record(simTime() - lastReceiveTime);
      }
    }

    if (echo_req.seqNo < nextEstSeqNo)
    {
      Dout(dc::statistic|flush_cf, rt->nodeName()<<" "<<simTime()<<" out of order ping echo_request"
         <<" with seq="<<echo_req.seqNo<<" when nextEstSeqNo="<<nextEstSeqNo);
    }
    else
    {
      nextEstSeqNo=echo_req.seqNo+1;

      lastReceiveTime = simTime();
      if (simTime() >= icmpRecordStart)
      {
        pingDelay->record(simTime() -  echo_req.sendingTime);
        stat->collect(simTime() - echo_req.sendingTime);
      }
    }

    Dout(dc::statistic|flush_cf, rt->nodeName()<<" icmpCorePingReqEED \t"
         <<simTime()<<"\t"<<simTime() - echo_req.sendingTime);

    Dout(dc::ping6, req->length()<<dec<<" bytes from " <<request->srcAddress()
         <<" icmp_seq="<<echo_req.seqNo<<" ttl="<<(size_t)request->hopLimit()
         <<" eed="<< (simTime() - echo_req.sendingTime ) * 1000 << " msec"
         <<"  ( ping request received at "<< rt->nodeName() << ")");
  }

  if (!replyToICMPRequests)
    return;

  ICMPv6Message* reply = new ICMPv6Echo(req->identifier(), req->seqNo());
  req->encapsulatedMsg()->setName("PING6_REPLY");

  //Retrieve internal data inside ping packet and put into reply
  reply->encapsulate(req->decapsulate());

  // send echo reply to the source

  //Use request->destAddress as src Address as there could be two addresses with
  //same scope when determineSrcAddress is used.
  ipv6_addr src =  request->destAddress();
  if (request->destAddress().isMulticast())
  {
    src = fc->determineSrcAddress(request->srcAddress(), request->inputPort());
    if (src == IPv6_ADDR_UNSPECIFIED)
      return;
  }

  Dout(dc::ping6, rt->nodeName()<<":"<<request->inputPort()<<" "<<simTime()
       <<" sending echo reply to "<<request->srcAddress());
  sendInterfacePacket(reply, request->srcAddress(), src, request->hopLimit());
  ctrIcmp6OutEchoReplies++;
  rt->ctrIcmp6OutMsgs++;
}

void ICMPv6Core::recEchoReply (IPv6Datagram* reply)
{

  ICMPv6Echo* echo_reply = static_cast<ICMPv6Echo*> (reply->encapsulatedMsg());
  IPv6InterfaceData<echo_int_info>* app_req =
    static_cast<IPv6InterfaceData<echo_int_info>*> (echo_reply->decapsulate());


  //As the whole ping request info was sent there is no need to reconstruct
  //another IPv6InterfaceData and filling it up with values just add the source
  //address of where the reply came from
  app_req->setSrcAddr(ipv6_addr_toString(reply->srcAddress()).c_str());
  app_req->data().hopLimit = reply->hopLimit();
  app_req->data().seqNo = echo_reply->seqNo();
  app_req->data().id = echo_reply->identifier();

  delete reply;

  //Notify upper layer
#if !defined TESTIPv6PING
  send(app_req, "pingOut");
#else
  echo_int_info echo_req = app_req->data();
  cout<<dec<<echo_req.id<<" "<<echo_req.seqNo<<" from "<<app_req->srcAddress()
      <<" "<<echo_req.custom_data<<endl;
  delete app_req;
#endif //TESTIPv6PING
}

void ICMPv6Core::sendEchoRequest(cMessage *msg)
{
  IPv6InterfaceData<echo_int_info>*  app_req =
    static_cast< IPv6InterfaceData<echo_int_info>* > (msg);
  echo_int_info echo_req = app_req->data();

  ICMPv6Message *request = new ICMPv6Echo(echo_req.id, echo_req.seqNo, true);
  request->setName("PING6_REQ");
  //Send the upper layer req across as test
  request->encapsulate(app_req);

  sendInterfacePacket(request, app_req->destAddress(), app_req->srcAddress(),
                      app_req->data().hopLimit);
}

void ICMPv6Core::sendInterfacePacket(ICMPv6Message *msg, const ipv6_addr& dest,
                                     const ipv6_addr& src, size_t hopLimit)
{
  IPv6InterfacePacket* interfacePacket = new IPv6InterfacePacket;

  assert(dest != IPv6_ADDR_UNSPECIFIED);

  interfacePacket->encapsulate(msg);
  interfacePacket->setDestAddr(dest);
  interfacePacket->setSrcAddr(src);
  interfacePacket->setProtocol(IP_PROT_IPv6_ICMP);
  interfacePacket->setName(msg->name());

  if (hopLimit != 0)
    interfacePacket->setTimeToLive(hopLimit);

  send(interfacePacket, "sendOut");
}

