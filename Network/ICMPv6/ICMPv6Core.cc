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


#include "sys.h"
#include "debug.h"

#include <memory> //auto_ptr
#include <omnetpp.h>
#include <boost/cast.hpp>

#include "ICMPv6Core.h"
#include "IPv6Datagram.h"
#include "ipv6_addr.h"
#include "IPv6InterfacePacket_m.h"
#include "IPv6Headers.h"
#include "ICMPv6Message.h"

#include "Ping6InterfacePacket_m.h"
#include "opp_utils.h"
#include "InterfaceTableAccess.h"
#include "RoutingTable6Access.h" //required for counters only
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
  QueueBase::initialize();
  ift = InterfaceTableAccess().get();
  rt = RoutingTable6Access().get();

  icmpRecordStats = par("icmpRecordRequests");
  replyToICMPRequests = par("replyToICMPRequests");
  icmpRecordStart = par("icmpRecordStart");
  stat = 0;
  if (icmpRecordStats)
    stat = new cStdDev("PingRequestReceived");

  fc = check_and_cast<IPv6Forward*> (
    OPP_Global::findModuleByTypeDepthFirst(this, "IPv6Forward")); // XXX try to get rid of pointers to other modules --AV
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
}

void ICMPv6Core::endService(cMessage* msg)
{
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

    // XXX cleanup stuff must be moved to dtor! --AV
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
      delete errorMessage;
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
        dgram->encapsulate(icmpmsg); // FIXME what the hell is this? it's just been decapsulated!!! --Andras
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
  echo_int_info echo_req = check_and_cast<Ping6InterfacePacket*> (req->encapsulatedMsg())->data();

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
  Ping6InterfacePacket* app_req =
    static_cast<Ping6InterfacePacket*> (echo_reply->decapsulate());


  //As the whole ping request info was sent there is no need to reconstruct
  //another IPv6InterfacePacketWithData and filling it up with values just add the source
  //address of where the reply came from
  app_req->setSrcAddr(reply->srcAddress());
  app_req->data().hopLimit = reply->hopLimit();
  app_req->data().seqNo = echo_reply->seqNo();
  app_req->data().id = echo_reply->identifier();

  delete reply;

  //Notify upper layer
#if !defined TESTIPv6PING
  send(app_req, "pingOut");
#else
  echo_int_info echo_req = app_req->data();
  cout<<dec<<echo_req.id<<" "<<echo_req.seqNo<<" from "<<app_req->srcAddress()<<endl;
  delete app_req;
#endif //TESTIPv6PING
}

void ICMPv6Core::sendEchoRequest(cMessage *msg)
{
  Ping6InterfacePacket*  app_req =
    static_cast< Ping6InterfacePacket* > (msg);
  echo_int_info echo_req = app_req->data();

  ICMPv6Message *request = new ICMPv6Echo(echo_req.id, echo_req.seqNo, true);
  request->setName("PING6_REQ");
  //Send the upper layer req across as test
  request->encapsulate(app_req);

  sendInterfacePacket(request, app_req->destAddr(), app_req->srcAddr(),
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

