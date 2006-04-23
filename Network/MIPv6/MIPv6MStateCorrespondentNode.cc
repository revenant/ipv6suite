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


/**
 * @file MIPv6MStateCorrespondentNode.cc
 * @author Eric Wu, Johnny Lai
 * @date 16.4.2002

 * @brief Implements functionality of Correspondent Node
 *
 */

#include "sys.h"
#include "debug.h"

#include <boost/cast.hpp>
#include <boost/bind.hpp>

#include "MIPv6MStateCorrespondentNode.h"
#include "IPv6Mobility.h"
#include "IPv6Datagram.h"
#include "MIPv6MobilityHeaders.h"
#include "MIPv6Entry.h"
#include "MIPv6DestOptMessages.h"
#include "MIPv6CDS.h"
#include "RoutingTable6.h" //setHopLimit
#include "InterfaceTable.h"
#include "IPv6InterfaceData.h"
#include "MIPv6Timers.h"
#include "HdrExtRteProc.h"

namespace MobileIPv6
{

MIPv6MStateCorrespondentNode::MIPv6MStateCorrespondentNode(IPv6Mobility* mod):
  MIPv6MobilityState(mod),
  periodTmr(new MIPv6PeriodicCB(mob, MIPv6_PERIOD))
{
  mob->rt->mipv6cds = mipv6cds;
  ((MIPv6PeriodicCB*)(periodTmr))->connect(boost::bind(&MIPv6CDS::expireLifetimes, mipv6cds, periodTmr));		       				 
}

MIPv6MStateCorrespondentNode::~MIPv6MStateCorrespondentNode()
{
  if (periodTmr && !periodTmr->isScheduled())
    delete periodTmr;
  periodTmr = 0;
}


bool MIPv6MStateCorrespondentNode::processMobilityMsg(IPv6Datagram* dgram)
{
  MIPv6MobilityHeaderBase* mhb = mobilityHeaderExists(dgram);

  if (!mhb)
    return false;

  switch ( mhb->header_type() )
  {
    case MIPv6MHT_BU:
    {
      BU* bu = check_and_cast<BU*>(mhb);
      processBU(dgram, bu);
    }
    break;
    case MIPv6MHT_CoTI: case MIPv6MHT_HoTI:
    {
      TIMsg* ti = check_and_cast<TIMsg*>(mhb);
      processTI(ti, dgram);
    }
    break;
    default:
      defaultResponse(dgram, mhb);
      return false;
    break;
  }

  return true;
}

bool MIPv6MStateCorrespondentNode::cnSendPacketCheck(IPv6Datagram& dgram)
{
  //Could possibly place this into sendCore

  boost::weak_ptr<bc_entry> bce;

  // In MN-MN communications, it is possible that the mobility
  // messages are sent in reverse tunnel. Therefore, we need to
  // check if the packet is encapsulated with another IPv6 header.

  MobileIPv6::MIPv6MobilityHeaderBase* ms = 0;
  IPv6Datagram* tunPacket = 0;
  if (dgram.transportProtocol() == IP_PROT_IPv6)
    tunPacket = check_and_cast<IPv6Datagram*>(dgram.encapsulatedMsg());

  if (tunPacket && tunPacket->transportProtocol() == IP_PROT_IPv6_MOBILITY)
    ms = check_and_cast<MIPv6MobilityHeaderBase*>(
      tunPacket->encapsulatedMsg());
  else if (dgram.transportProtocol() == IP_PROT_IPv6_MOBILITY)
    ms = check_and_cast<MIPv6MobilityHeaderBase*>(dgram.encapsulatedMsg());

  // no mobility message is allowed to have type 2 rh except BA
  if (ms == 0 || (ms && ms->header_type() ==  MIPv6MHT_BA))
  {
    bce = mipv6cds->findBinding(dgram.destAddress());

    if (bce.lock().get())
    {
      assert(bce.lock()->home_addr == dgram.destAddress());
      dgram.setDestAddress(bce.lock()->care_of_addr);

      HdrExtRteProc* rtProc = dgram.acquireRoutingInterface();
      //Should only be one t2 header per datagram (except for inner
      //tunneled packets)
      assert(rtProc->routingHeader(IPv6_TYPE2_RT_HDR) == 0);
      MIPv6RteOpt* rt2 = new MIPv6RteOpt(bce.lock()->home_addr);
      rtProc->addRoutingHeader(rt2);
      dgram.addLength(rtProc->lengthInUnits()*BITS);
      Dout(dc::mipv6, mob->nodeName()<<" Found binding for destination "
           <<bce.lock()->home_addr<<" swapping to "<<bce.lock()->care_of_addr);

      return true;
    }
  }
  return false;
}

// protected member functions

bool MIPv6MStateCorrespondentNode::processBU(IPv6Datagram* dgram, BU* bu)
{
  ipv6_addr hoa, coa;

  if (!preprocessBU(dgram, bu, hoa, coa))
    return false;

  // BU in which H bit is set to the CN, we should send the BA back to
  // the MN

  if (bu->homereg())
  {
    BA* ba = new BA(BA::BAS_HR_NOT_SUPPORTED, UNDEFINED_SEQ,
                    UNDEFINED_EXPIRES, UNDEFINED_REFRESH);

    sendBA(dgram->destAddress(), dgram->srcAddress(), ba);
    Dout(dc::warning|dc::mipv6, mob->nodeName()<<" BU received with homereg bit set but in"
         <<"CN mode so check config BA sent with seq no "<<UNDEFINED_SEQ);
    return false;
  }

  //Pg. 83 rev. 24
  // home nonce indices mob opt present
  //regnerate home keygen token and coa token. generate kBM and verify authenticator in bu
  //binding authorizatino data mob opt must be present and last opt and no padding

  // if the lifetime of BU is zero OR the MN returns to its home
  // subnet, delete the its binding update, accordingly
  if (bu->expires() == 0 || dgram->srcAddress() == hoa)
  {
    // sec 5.1.9, signal an inappropriate attempt to use the home
    // address option without existing binding; NOTE that the home
    // address option has already checked for its existence by calling
    // MIPv6MobilityState::processBU()
    if(!MIPv6MobilityState::deregisterBCE(bu, 0))
    {
      BM* bm = new BM(bu->ha());
      sendBM(dgram->destAddress(), dgram->srcAddress(), bm);
      Dout(dc::warning, mob->nodeName()<<" CN deregistration failed from hoa="
           <<hoa);
      return false;
    }
    return true;
  }

/*
   check for coa nonce index exists too
   check both coa/hoa nonce indices are recent!

   If the receiving node no longer recognizes the Home Nonce Index
   value, Care-of Nonce Index value, or both values from the Binding
   Update, then the receiving node MUST send back a Binding
   Acknowledgement with status code 136, 137, or 138, respectively.
*/

  if (mob->earlyBindingUpdate())
  {
    boost::weak_ptr<bc_entry> bce =
      mipv6cds->findBinding(hoa);

    // binding cache entry has already been created
    if (bce.lock() &&  bce.lock()->care_of_addr == coa)
    {
      bce.lock()->expires = bu->expires();
      bce.lock()->setSeqNo(bu->sequence());

      if ( bu->ack() )
      {
        BA* ba = new BA(BA::BAS_ACCEPTED, bu->sequence(), bu->expires(),
                        bu->expires());

        sendBA(dgram->destAddress(), dgram->srcAddress(), ba, dgram->timestamp());
      }
      
      check_and_cast<IPv6Mobility*>(bu->senderModule())->recordHODelay(mob->simTime()-dgram->timestamp(), dgram->destAddress());
      return true;
    }
  }

  MIPv6MobilityState::registerBCE(dgram, bu);
  Dout(dc::mipv6|dc::rrprocedure|flush_cf, "At " << mob->simTime() << ", " << mob->nodeName()
       <<" CN registering BU from src="<<dgram->srcAddress()
       <<" hoa="<<hoa<<" coa="<<coa);
  // bu is legal, if bu in which A bit is set, send the BA back to the
  // sending MN
  if ( bu->ack() )
  {
    BA* ba = new BA(BA::BAS_ACCEPTED, bu->sequence(), bu->expires(),
                    bu->expires());

    sendBA(dgram->destAddress(), dgram->srcAddress(), ba, dgram->timestamp());
  }

  if (!mob->earlyBindingUpdate())
  {
    assert( dgram->timestamp() );
    check_and_cast<IPv6Mobility*>(bu->senderModule())->recordHODelay(mob->simTime()-dgram->timestamp(), dgram->destAddress());
  }
  return true;
}

void MIPv6MStateCorrespondentNode::processTI(TIMsg* ti, IPv6Datagram* dgram)
{
  // MUST NOT carry destination option
  // check if the packet contains home address option
  HdrExtProc* proc = dgram->findHeader(EXTHDR_DEST);
  if (proc)
  {

    IPv6TLVOptionBase* opt = boost::polymorphic_downcast<HdrExtDestProc*>(proc)->
      getOption(IPv6TLVOptionBase::MIPv6_HOME_ADDRESS_OPT);
    if (opt)
    {
      Dout(dc::rrprocedure|flush_cf, "At "<< mob->simTime() << ", RR procedure ERROR: " << mob->nodeName()<<" receives a "<< ti->className() << ", which contains an home address destination option");
      return;
    }
  }
  MIPv6MobilityHeaderType replyType;
  if (ti->header_type() == MIPv6MHT_HoTI)
    replyType = MIPv6MHT_HoT;
  else if (ti->header_type() == MIPv6MHT_CoTI)
    replyType = MIPv6MHT_CoT;

  // send back the HoT to the mobile node
  // TODO: not implement the token for now
  TMsg* testMsg = new TMsg(replyType, 0, ti->cookie, mipv6cds->generateToken(replyType));
  IPv6Datagram* reply = new IPv6Datagram(dgram->destAddress(), dgram->srcAddress(), testMsg);
  reply->setHopLimit(mob->ift->interfaceByPortNo(0)->ipv6()->curHopLimit);
  reply->setTransportProtocol(IP_PROT_IPv6_MOBILITY);
  
  assert(dgram->timestamp());
  reply->setTimestamp(dgram->timestamp());

  Dout(dc::rrprocedure|flush_cf, "RR procedure: At " <<mob->simTime() << "sec, " << mob->nodeName()<<" sending " << testMsg->className() << " src= " << dgram->destAddress() <<" to "<<dgram->srcAddress());

  mob->send(reply, "routingOut");
}

void MIPv6MStateCorrespondentNode::sendBM(const ipv6_addr& srcAddr,
                                          const ipv6_addr& destAddr,
                                          BM* bm)
{
  IPv6Datagram* reply = new IPv6Datagram(srcAddr, destAddr, bm);
  reply->setTransportProtocol(IP_PROT_IPv6_MOBILITY);
  reply->setHopLimit(mob->ift->interfaceByPortNo(0)->ipv6()->curHopLimit);
  // TODO: include routing header

  mob->send(reply, "routingOut");
}


} // end namespace MobileIPv6
