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

#include "MIPv6MStateCorrespondentNode.h"
#include "IPv6Mobility.h"
#include "IPv6Datagram.h"
#include "MIPv6MobilityHeaders.h"
#include "MIPv6Entry.h"
#include "MIPv6DestOptMessages.h"
#include "MIPv6CDS.h"
#include "RoutingTable6.h" //setHopLimit

namespace MobileIPv6
{

MIPv6MStateCorrespondentNode* MIPv6MStateCorrespondentNode::_instance = 0;

MIPv6MStateCorrespondentNode* MIPv6MStateCorrespondentNode::instance(void)
{
  if(_instance == 0)
    _instance = new MIPv6MStateCorrespondentNode;

  return _instance;
}

void MIPv6MStateCorrespondentNode::
processMobilityMsg(IPv6Datagram* dgram,
                   MIPv6MobilityHeaderBase*& mhb,
                   IPv6Mobility* mod)
{
  MIPv6MobilityState::processMobilityMsg(dgram, mhb, mod);

  if (!mhb)
    return;

  switch ( mhb->header_type() )
  {
    case MIPv6MHT_BU:
    {
      BU* bu = check_and_cast<BU*>(mhb);
      processBU(dgram, bu, mod);
    }
    break;
    case MIPv6MHT_CoTI: case MIPv6MHT_HoTI:
    {
      TIMsg* ti = check_and_cast<TIMsg*>(mhb);
      processTI(ti, dgram, mod);
    }
    break;
    default:
      cerr << "Mobile IPv6 Mobility Header not recognised ... " << endl;
    break;
  }
}

// protected member functions

bool MIPv6MStateCorrespondentNode::processBU(IPv6Datagram* dgram,
                                             BU* bu,
                                             IPv6Mobility* mod)
{
  ipv6_addr hoa, coa;

  if (!preprocessBU(dgram, bu, mod, hoa, coa))
    return false;

  // BU in which H bit is set to the CN, we should send the BA back to
  // the MN

  if (bu->homereg())
  {
    BA* ba = new BA(BA::BAS_HR_NOT_SUPPORTED, UNDEFINED_SEQ,
                    UNDEFINED_EXPIRES, UNDEFINED_REFRESH);

    sendBA(dgram->destAddress(), dgram->srcAddress(), ba, mod);

    return false;
  }

  // if the lifetime of BU is zero OR the MN returns to its home
  // subnet, delete the its binding update, accordingly
  if (bu->expires() == 0 || dgram->srcAddress() == hoa)
  {
    // sec 5.1.9, signal an inappropriate attempt to use the home
    // address option without existing binding; NOTE that the home
    // address option has already checked for its existence by calling
    // MIPv6MobilityState::processBU()
    if(!deregisterBCE(bu, 0, mod))
    {
      BM* bm = new BM(bu->ha());
      sendBM(dgram->destAddress(), dgram->srcAddress(), bm, mod);
      Dout(dc::warning, mod->nodeName()<<" CN deregistration failed from hoa="
           <<hoa);
      return false;
    }
    return true;
  }

  if (mod->earlyBindingUpdate())
  {
    boost::weak_ptr<bc_entry> bce =
      mod->mipv6cds->findBinding(hoa);

    // binding cache entry has already been created
    if (bce.lock() &&  bce.lock()->care_of_addr == coa)
    {
      bce.lock()->expires = bu->expires();
      bce.lock()->seq_no = bu->sequence();

      if ( bu->ack() )
      {
        BA* ba = new BA(BA::BAS_ACCEPTED, bu->sequence(), bu->expires(),
                        bu->expires());

        sendBA(dgram->destAddress(), dgram->srcAddress(), ba, mod);
      }

      check_and_cast<IPv6Mobility*>(bu->senderModule())->recordHODelay(mod->simTime(), dgram->destAddress());
      return true;
    }
  }

  registerBCE(dgram, bu, mod);
  Dout(dc::mipv6|dc::rrprocedure|flush_cf, "At " << mod->simTime() << ", " << mod->nodeName()
       <<" CN registering BU from src="<<dgram->srcAddress()
       <<" hoa="<<hoa<<" coa="<<coa);
  // bu is legal, if bu in which A bit is set, send the BA back to the
  // sending MN
  if ( bu->ack() )
  {
    BA* ba = new BA(BA::BAS_ACCEPTED, bu->sequence(), bu->expires(),
                    bu->expires());

    sendBA(dgram->destAddress(), dgram->srcAddress(), ba, mod);
  }

  if (!mod->earlyBindingUpdate())
    check_and_cast<IPv6Mobility*>(bu->senderModule())->recordHODelay(mod->simTime(), dgram->destAddress());

  return true;
}

void MIPv6MStateCorrespondentNode::processTI(TIMsg* ti, IPv6Datagram* dgram, IPv6Mobility* mod)
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
      Dout(dc::rrprocedure|flush_cf, "At "<< mod->simTime() << ", RR procedure ERROR: " << mod->nodeName()<<" receives a "<< ti->className() << ", which contains an home address destination option");
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
  TMsg* testMsg = new TMsg(replyType, 0, ti->cookie, mod->mipv6cds->token(replyType));
  IPv6Datagram* reply = new IPv6Datagram(dgram->destAddress(), dgram->srcAddress(), testMsg);
  reply->setHopLimit(mod->ift->interfaceByPortNo(0)->curHopLimit);
  reply->setTransportProtocol(IP_PROT_IPv6_MOBILITY);

  Dout(dc::rrprocedure|flush_cf, "RR procedure: At " <<mod->simTime() << "sec, " << mod->nodeName()<<" sending " << testMsg->className() << " src= " << dgram->destAddress() <<" to "<<dgram->srcAddress());

  mod->send(reply, "routingOut");
}

void MIPv6MStateCorrespondentNode::sendBM(const ipv6_addr& srcAddr,
                                          const ipv6_addr& destAddr,
                                          BM* bm, IPv6Mobility* mod)
{
  IPv6Datagram* reply = new IPv6Datagram(srcAddr, destAddr, bm);
  reply->setTransportProtocol(IP_PROT_IPv6_MOBILITY);
  reply->setHopLimit(mod->ift->interfaceByPortNo(0)->curHopLimit);
  // TODO: include routing header

  mod->send(reply, "routingOut");
}


} // end namespace MobileIPv6
