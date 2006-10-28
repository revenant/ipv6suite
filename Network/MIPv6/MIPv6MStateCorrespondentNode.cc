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
#include "MobilityHeaders.h"
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
  MobilityHeaderBase* mhb = mobilityHeaderExists(dgram);

  if (!mhb)
    return false;

  switch ( mhb->kind() )
  {
    case MIPv6MHT_BU:
    {
      BU* bu = check_and_cast<BU*>(mhb);
      processBU(bu, dgram);
    }
    break;
    case MIPv6MHT_COTI: case MIPv6MHT_HOTI:
    {
      processTI(mhb, dgram);
    }
    break;
    default:
      defaultResponse(mhb, dgram);
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

  MobilityHeaderBase* ms = 0;
  IPv6Datagram* tunPacket = 0;
  if (dgram.transportProtocol() == IP_PROT_IPv6)
    tunPacket = check_and_cast<IPv6Datagram*>(dgram.encapsulatedMsg());

  if (tunPacket && tunPacket->transportProtocol() == IP_PROT_IPv6_MOBILITY)
    ms = check_and_cast<MobilityHeaderBase*>(
      tunPacket->encapsulatedMsg());
  else if (dgram.transportProtocol() == IP_PROT_IPv6_MOBILITY)
    ms = check_and_cast<MobilityHeaderBase*>(dgram.encapsulatedMsg());

  // no mobility message is allowed to have type 2 rh except BA
  if (ms == 0 || (ms && ms->kind() ==  MIPv6MHT_BA))
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

bool MIPv6MStateCorrespondentNode::processBU(BU* bu, IPv6Datagram* dgram)
{
  ipv6_addr hoa, coa;
  bool hoaOptFound;
  if (!preprocessBU(bu, dgram, hoa, coa, hoaOptFound))
    return false;

  // BU in which H bit is set to the CN, we should send the BA back to
  // the MN

  if (bu->homereg())
  {
    BA* ba = new BA;
    ba->setStatus(BAS_HR_NOT_SUPPORTED);

    sendBA(dgram->destAddress(), dgram->srcAddress(), hoa, ba);
    Dout(dc::warning|dc::mipv6, mob->nodeName()<<" BU received with homereg bit set but in"
         <<"CN mode so check config BA sent with seq no "<<0);
    return false;
  }

  //TODO Pg. 82
  //Inspect HONI and CONI options for recentness. CONI are skipped if deleting binding
  //Regnerate home keygen token and coa token. generate kBM and verify authenticator in bu
  //binding authorizatino data mob opt must be present and last opt and no padding
  //use acoa if present (part of preprocess BU)

  //send 136/137/138 if honi or coni not up to date

  // if the lifetime of BU is zero OR the MN returns to its home
  // subnet, delete the its binding update, accordingly
  if (bu->expires() == 0 || coa == hoa)
  {
    // sec 5.1.9, signal an inappropriate attempt to use the home
    // address option without existing binding; NOTE that the home
    // address option has already checked for its existence by calling
    // MIPv6MobilityState::processBU()
    //TODO assuming only 1 interface for correspondent Node instead of retrieving the correct Ifindex
    if(!MIPv6MobilityState::deregisterBCE(bu, hoa, 0))
    {
      sendBE(1, dgram);
      Dout(dc::warning, mob->nodeName()<<" CN deregistration failed from hoa="
           <<hoa);
      return false;
    }
    return true;
  }

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
        BA* ba = new BA(BAS_ACCEPTED, bu->sequence(), bu->expires());

        sendBA(dgram->destAddress(), dgram->srcAddress(), hoa, ba, dgram->timestamp());
      }
      
      check_and_cast<IPv6Mobility*>(bu->senderModule())->recordHODelay(mob->simTime()-dgram->timestamp(), dgram->destAddress());
      return true;
    }
  }

  MIPv6MobilityState::registerBCE(bu, hoa, dgram);
  Dout(dc::mipv6|dc::rrprocedure|flush_cf, "At " << mob->simTime() << ", " << mob->nodeName()
       <<" CN registering BU from src="<<dgram->srcAddress()
       <<" hoa="<<hoa<<" coa="<<coa);
  // bu is legal, if bu in which A bit is set, send the BA back to the
  // sending MN
  if ( bu->ack() )
  {
    BA* ba = new BA(BAS_ACCEPTED, bu->sequence(), bu->expires());
    sendBA(dgram->destAddress(), dgram->srcAddress(), hoa, ba, dgram->timestamp());
  }

  if (!mob->earlyBindingUpdate())
  {
    assert( dgram->timestamp() );
    check_and_cast<IPv6Mobility*>(bu->senderModule())->recordHODelay(mob->simTime()-dgram->timestamp(), dgram->destAddress());
  }
  return true;
}


//Sec. 9.4.1-4 no Sec. 5.2.3 generation of cookies/tokens as we are not testing security of RR procedure
void MIPv6MStateCorrespondentNode::processTI(MobilityHeaderBase* ti, IPv6Datagram* dgram)
{
  // MUST NOT carry destination option
  // check if the packet contains home address option (i.e. mn bound with us previously?
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

  MobilityHeaderBase* testMsg = 0;
  if (ti->kind() == MIPv6MHT_HOTI)
  {
    testMsg = new HOT;
  }
  else if (ti->kind() == MIPv6MHT_COTI)
  {
    testMsg = new COT;
  }
  else
    assert(false);

  // send back the HoT to the mobile node
  IPv6Datagram* reply = new IPv6Datagram(dgram->destAddress(), dgram->srcAddress(), testMsg);
  reply->setHopLimit(mob->ift->interfaceByPortNo(0)->ipv6()->curHopLimit);
  reply->setTransportProtocol(IP_PROT_IPv6_MOBILITY);
  
  assert(dgram->timestamp());
  reply->setTimestamp(dgram->timestamp());

  Dout(dc::rrprocedure|flush_cf, "RR procedure: At " <<mob->simTime() << "sec, " << mob->nodeName()<<" sending " << testMsg->className() << " src= " << dgram->destAddress() <<" to "<<dgram->srcAddress());

  mob->send(reply, "routingOut");
}

} // end namespace MobileIPv6
