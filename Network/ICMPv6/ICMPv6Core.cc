//
// Copyright (C) 2001, 2004 Eric Wu (eric.wu@eng.monash.edu.au)
// Copyright Copyright (C) 2001, 2004 Johnny Lai
//
// Monash University, Melbourne, Australia
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


#include "sys.h"
#include "debug.h"

#include <memory> //auto_ptr
#include <omnetpp.h>
#include <boost/cast.hpp>

#include "ICMPv6Core.h"
#include "IPv6Datagram.h"
#include "ipv6_addr.h"
#include "IPv6ControlInfo_m.h"
#include "ipv6addrconv.h"
#include "IPv6Headers.h"
#include "ICMPv6Message_m.h"
#include "ICMPv6MessageUtil.h"

#include "PingPayload_m.h"
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

ICMPv6Core::~ICMPv6Core()
{
  delete pingDelay;
  delete pingDrop;
}

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

  dropCount = 0;
  nextEstSeqNo = 0;
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

    if (isICMPv6Error((ICMPv6Type)recICMPv6Msg->type()))
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
  sendToIPv6(errorMessage, pdu->srcAddress());
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

*/
void ICMPv6Core::processICMPv6Message(IPv6Datagram* dgram)
{

  ICMPv6Message* icmpmsg = check_and_cast<ICMPv6Message*>(dgram->decapsulate());

  switch (icmpmsg->type())
  {
      case ICMPv6_DESTINATION_UNREACHABLE:
        sendToErrorOut(icmpmsg);
        ctrIcmp6InDestUnreachable++;
        break;

      case ICMPv6_PACKET_TOO_BIG:
        sendToErrorOut(icmpmsg);
        ctrIcmp6InPacketTooBig++;
        break;

      case ICMPv6_TIME_EXCEEDED:
        sendToErrorOut(icmpmsg);
        ctrIcmp6InTimeExceeded++;
        break;

      case ICMPv6_PARAMETER_PROBLEM:
        sendToErrorOut(icmpmsg);
        ctrIcmp6InParamProblem++;
        break;

      case ICMPv6_ECHO_REQUEST:
        processEchoRequest(icmpmsg, dgram->srcAddress(), dgram->destAddress(), dgram->hopLimit());
        delete dgram;
        dgram = 0;
        ctrIcmp6InEchos++;
        break;

      case ICMPv6_ECHO_REPLY:
        processEchoReply(icmpmsg, dgram->srcAddress(), dgram->destAddress(), dgram->hopLimit());
        delete dgram;
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
  }
  delete dgram;
}

///
void ICMPv6Core::sendToErrorOut(ICMPv6Message *icmpmsg)
{
  send(icmpmsg, "errorOut");
}

/*  ----------------------------------------------------------
       Echo request and reply ICMPv6 messages
    ----------------------------------------------------------  */

void ICMPv6Core::processEchoRequest (ICMPv6Message *request, const ipv6_addr& src,
                                   const ipv6_addr& dest, int hopLimit)
{
  // Echo Request may contain anything, but if we recognize it,
  // we can do some statistics
  PingPayload *payload = dynamic_cast<PingPayload *>(request->encapsulatedMsg());

  if (payload && icmpRecordStats)
  {
    long seqNo = payload->seqNo();
    simtime_t sendingTime = payload->creationTime();

    if (seqNo > nextEstSeqNo)
    {
      Dout(dc::ping6, rt->nodeName() <<" pingecho_req: "<<simTime()<<" expected seqNo mismatch, dropCount="
           <<dropCount<<" echo_req.seqNo="<<seqNo<<" nextEstSeqNo="<<nextEstSeqNo
           <<" just dropped="<<seqNo - nextEstSeqNo);
      dropCount += (unsigned short)(seqNo - nextEstSeqNo);
      if (simTime() >= icmpRecordStart)
        pingDrop->record((unsigned short)(seqNo - nextEstSeqNo));
    }

    if (seqNo < nextEstSeqNo)
    {
      Dout(dc::statistic|flush_cf, rt->nodeName()<<" "<<simTime()<<" out of order ping echo_request"
         <<" with seq="<<seqNo<<" when nextEstSeqNo="<<nextEstSeqNo);
    }
    else
    {
      nextEstSeqNo=seqNo+1;

      if (simTime() >= icmpRecordStart)
      {
        pingDelay->record(simTime() -  sendingTime);
        stat->collect(simTime() - sendingTime);
      }
    }

    Dout(dc::statistic|flush_cf, rt->nodeName()<<" icmpCorePingReqEED \t"
         <<simTime()<<"\t"<<simTime() - sendingTime);

    Dout(dc::ping6, (request->length()/BITS)<<dec<<" bytes from " <<src
         <<" icmp_seq="<<seqNo<<" ttl="<<hopLimit
         <<" eed="<< (simTime() - sendingTime) * 1000 << " msec"
         <<"  (ping request received at "<< rt->nodeName() << ")");
  }

  if (!replyToICMPRequests)
  {
    delete request;
    return;
  }

  // send echo reply to the source
  ICMPv6Message *reply = request;
  reply->setName((std::string(request->name())+"-reply").c_str());
  reply->setType(ICMPv6_ECHO_REPLY);

  //Use request->destAddress as src Address as there could be two addresses with
  //same scope when determineSrcAddress is used.
  int inputPort = -1; //FIXME where to get it from?????????????????????????!!!!!!!!!!!!!!!!!!!1
//__asm int 3;
  ipv6_addr src1 = src;
  if (dest.isMulticast())
  {
    src1 = fc->determineSrcAddress(src, inputPort);
    if (src1 == IPv6_ADDR_UNSPECIFIED)
    {
      delete request;
      return;
    }
  }

  Dout(dc::ping6, rt->nodeName()<<":"<<inputPort<<" "<<simTime()
       <<" sending echo reply to "<<src);
  sendToIPv6(reply, src1, dest, hopLimit);
  ctrIcmp6OutEchoReplies++;
  rt->ctrIcmp6OutMsgs++;
}

void ICMPv6Core::processEchoReply (ICMPv6Message *reply, const ipv6_addr& src,
                                   const ipv6_addr& dest, int hopLimit)
{
  // decapsulate ping payload (data)...
  cMessage *payload = reply->decapsulate();
  delete reply;

  // attach extra info to it ...
  IPv6ControlInfo *ctrl = new IPv6ControlInfo();
  ctrl->setProtocol(IP_PROT_IPv6_ICMP);
  ctrl->setSrcAddr(mkIPv6Address_(src));
  ctrl->setDestAddr(mkIPv6Address_(dest));
  ctrl->setTimeToLive(hopLimit);
  payload->setControlInfo(ctrl);

  // ... then it send up
  send(payload, "pingOut");
}

void ICMPv6Core::sendEchoRequest(cMessage *msg)
{
  IPv6ControlInfo *ctrl = check_and_cast<IPv6ControlInfo*>(msg->removeControlInfo());
  ICMPv6Message *request = createICMPv6Message(msg->name(), ICMPv6_ECHO_REQUEST, 0, msg);
  sendToIPv6(request, mkIpv6_addr(ctrl->destAddr()), mkIpv6_addr(ctrl->srcAddr()), ctrl->timeToLive());
  delete ctrl;
}

void ICMPv6Core::sendToIPv6(ICMPv6Message *msg, const ipv6_addr& dest,
                            const ipv6_addr& src, size_t hopLimit)
{
  assert(dest != IPv6_ADDR_UNSPECIFIED);

  IPv6ControlInfo *ctrl = new IPv6ControlInfo();
  ctrl->setProtocol(IP_PROT_IPv6_ICMP);
  ctrl->setSrcAddr(mkIPv6Address_(src));
  ctrl->setDestAddr(mkIPv6Address_(dest));
  if (hopLimit != 0)
    ctrl->setTimeToLive(hopLimit);
  msg->setControlInfo(ctrl);

  send(msg, "sendOut");
}

