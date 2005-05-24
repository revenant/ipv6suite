// -*- C++ -*-
// Copyright (C) 2002, 2004 CTIE, Monash University
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
 * @file MIPv6MobilityState.cc
 * @author Eric Wu, Johnny Lai
 * @date 16.4.2002

 * @brief Implements common functionality that CN, MN and HA has
 *
 */

#include "sys.h"
#include "debug.h"

#include <boost/weak_ptr.hpp>
#include <boost/cast.hpp>

#include "MIPv6MobilityState.h"
#include "IPv6Mobility.h"
#include "IPv6Datagram.h"
#include "HdrExtDestProc.h"
#include "HdrExtRteProc.h"
#include "IPProtocolId_m.h"
#include "InterfaceTable.h"
#include "IPv6InterfaceData.h"
#include "RoutingTable6.h"
#include "IPv6CDS.h"

#include "MIPv6DestOptMessages.h"
#include "MIPv6MobilityHeaders.h"
#include "MIPv6Entry.h"

#include "MIPv6MStateHomeAgent.h"
#include "MIPv6MStateCorrespondentNode.h"
#include "MIPv6MStateMobileNode.h"
#include "MIPv6CDS.h"

#ifdef USE_HMIP
#include "HMIPv6MStateMAP.h"
#include <iostream>
#endif

namespace MobileIPv6
{

const int TENTATIVE_BINDING_LIFETIME = 3;

MIPv6MobilityState::MIPv6MobilityState(void)
{}

MIPv6MobilityState::~MIPv6MobilityState(void)
{}

/*

  @todo rev. 24 SEc. 9.2 sending binding error for unrecognised messages. Check payload
  field is IPPROTO_NONE or send ICMP param prob code 0 to src addr.
  Check if the length in the option is actually greater or equal to how long it
  SHOULD be according to the draft standard

  @warning This changeState business is dangerous because we call
  MState::instance which can create a new instance when we don't want it
  to. E.g. When we want a MN to be a peer node i.e. CN role too then we have to
  be careful we call MStateMobileNode::instance otherwise we'll end up with two
  BC. TODO fix this misleading State pattern
 */
bool MIPv6MobilityState::nextState(IPv6Datagram* dgram, IPv6Mobility* mod)
{
  IPv6Datagram* dup_dgram = dgram->dup();

  cMessage *pkt = dup_dgram->decapsulate();
  MIPv6MobilityHeaderBase* mhb = dynamic_cast<MIPv6MobilityHeaderBase*>(pkt);

  // Fix the bu->physicalLenInOctet
  //if ( bu->physicalLenInOctet() < bu->length() )
  //  return false;

  if (!mhb)
  {
    delete pkt;
    delete dup_dgram;
    return false;
  }

  switch ( mhb->header_type() )
  {
      case MIPv6MHT_BU:
      {
        BU* bu = check_and_cast<BU*>(mhb);
        assert(bu != 0);

        if (mod->isHomeAgent() &&
            (bu->homereg()
#ifdef USE_HMIP
             || bu->mapreg()
#endif //USE_HMIP
             )
            )
        {
#ifdef USE_HMIP
          if ( mod->isMAP() && bu->mapreg())
            mod->changeState(HierarchicalMIPv6::HMIPv6MStateMAP::instance());
          else
#endif //USE_HMIP`
            mod->changeState(MIPv6MStateHomeAgent::instance());
        }
        else if (mod->isMobileNode())
          //Allowing MN to act as CN too
          mod->changeState(MIPv6MStateMobileNode::instance());
        else
          mod->changeState(MIPv6MStateCorrespondentNode::instance());
      }
      break;
      case MIPv6MHT_BA:
      {
        BA* ba = check_and_cast<BA*>(mhb);
        assert(ba != 0);

        mod->changeState(MIPv6MStateMobileNode::instance());
      }
      break;
      case MIPv6MHT_BM:
      {
        BM* bm = check_and_cast<BM*>(mhb);
        assert(bm != 0);

        mod->changeState(MIPv6MStateMobileNode::instance());
      }
      break;
    case MIPv6MHT_NONE:
    case MIPv6MHT_BR:
    case MIPv6MHT_HoTI: case MIPv6MHT_CoTI:
    {
      TIMsg* ti = check_and_cast<TIMsg*>(mhb);
      assert(ti != 0);
      if (mod->isMobileNode())
        mod->changeState(MIPv6MStateMobileNode::instance());
      else
        mod->changeState(MIPv6MStateCorrespondentNode::instance());
    }
    break;
    case MIPv6MHT_HoT: case MIPv6MHT_CoT:
    {
      TMsg* tmsg = check_and_cast<TMsg*>(mhb);
      assert(tmsg != 0);

      mod->changeState(MIPv6MStateMobileNode::instance());
    }
    break;
    default:
      cerr << " Unrecognised or Unimplemented message type "<<mhb->header_type()
           << " received at Mobility module"<<endl;
      break;
  }

  delete mhb;
  delete dup_dgram;

  return true;
}

//This fn is actually used by subclasses although I think it is a very bad form
//of reuse. Would be better to actually make it pure virtual and just stick
//these lines into the derived classes' fn
void MIPv6MobilityState::processMobilityMsg(IPv6Datagram* dgram,
                                            MIPv6MobilityHeaderBase*& mhb,
                                            IPv6Mobility* mod)
{
  cMessage *pkt = dgram->decapsulate();
  mhb = check_and_cast<MIPv6MobilityHeaderBase*>(pkt);

  if (!mhb)
    delete pkt;
}

// private functions

/**
 *
 *
 */

bool MIPv6MobilityState::preprocessBU(IPv6Datagram* dgram, BU* bu, IPv6Mobility* mod, ipv6_addr& hoa, ipv6_addr& coa)
{
  // check if the BU is valid before processing it

  // draft 16, page 82 "The packet meets the specific authentication
  // requirements for Binding Updates, defined in section 4.5
  // ... NOT IMPLEMENTED.. :-)


  hoa = dgram->srcAddress();
  coa = dgram->srcAddress();


  bool hoaOptFound = false;

  // check if the packet contains home address option
  HdrExtProc* proc = dgram->findHeader(EXTHDR_DEST);
  if (proc)
  {

    IPv6TLVOptionBase* opt = boost::polymorphic_downcast<HdrExtDestProc*>(proc)->
      getOption(IPv6TLVOptionBase::MIPv6_HOME_ADDRESS_OPT);
    if (opt)
    {

      MIPv6TLVOptHomeAddress* haOpt = boost::polymorphic_downcast<MIPv6TLVOptHomeAddress*>(opt);
      assert(haOpt);
      hoa = haOpt->homeAddr();
      Dout(dc::mipv6, mod->nodeName()<<" home address opt hoa="<<hoa);
      hoaOptFound = true;
    }
  }

  if (hoa.isMulticast() ||
      // pg 86 of rev. 24 seems to test this condition later but it sounds
      // like it should be tested much earlier otherwise BU is already processed
      dgram->srcAddress().isMulticast())
  {
    Dout(dc::mipv6, mod->nodeName()<<" BU has non unicast home address");
    return false;
  }
  if (ipv6_addr_scope(hoa) == ipv6_addr::Scope_None ||
      ipv6_addr_scope(hoa) == ipv6_addr::Scope_Node ||
      ipv6_addr_scope(hoa) == ipv6_addr::Scope_Link)
  {
    Dout(dc::mipv6, mod->nodeName()<<" BU has non routable address");
    return false;
  }


  MIPv6MHParameterBase* acoaExist = bu->parameter(MIPv6MHPT_ACoA);
  if (acoaExist)
  {
    MIPv6MHAlternateCareofAddress* acoaPar = boost::polymorphic_downcast<MIPv6MHAlternateCareofAddress*>(acoaExist);

    assert(acoaPar);
    coa = acoaPar->address();
    Dout(dc::mipv6, mod->nodeName()<<" alternate coa="<<coa);
  }

  // check if the sequence number is greater than the last successful
  // binding update

  boost::weak_ptr<bc_entry> bce = mod->mipv6cds->findBinding(hoa);
  if (bce.lock().get() != 0 && bu->sequence() < bce.lock()->seq_no )
  {
    BA* ba = new BA(BA::BAS_SEQ_OUT_OF_WINDOW, bce.lock()->seq_no,
                    bce.lock()->expires, UNDEFINED_REFRESH);

    sendBA(dgram->destAddress(), dgram->srcAddress(), ba, mod);
    Dout(dc::mipv6|dc::warning, " BU seq="<<bu->sequence()<<" < BC entry seq="<<bce.lock()->seq_no);
    return false;
  }

  if (!bu->homereg())
  {
    //Pg. 83 rev. 24
    // nonce indices mob opt present
    //regnerate home keygen token and coa token. generate kBM and verify authenticator in bu
    //binding authorizatino data mob opt must be present and last opt and no padding
  }
  else
  {
    //nonce indices mob opt not present
  }

  if (bce.lock().get() != 0 && bce.lock()->is_home_reg != bu->homereg())
  {
    BA* ba = new BA(BA::BAS_REG_TYPE_CHANGE_DIS, bce.lock()->seq_no,
                    bce.lock()->expires, UNDEFINED_REFRESH);
    sendBA(dgram->destAddress(), dgram->srcAddress(), ba, mod);
    Dout(dc::mipv6|dc::warning, " BU has different setting from bce for home_reg flag="<<(bu->homereg()));
  }

  if (bu->homereg() && !hoaOptFound)
  {
    Dout(dc::mipv6|dc::warning, " BU for home reg sent without a home addr option");
    return false;
  }

  assert ( dgram->timestamp ());

  return true;
}

///Register in binding cache
void MIPv6MobilityState::registerBCE(IPv6Datagram* dgram, BU* bu,
                                     IPv6Mobility* mod)
{
/*
  //Not sure if we really need this zeroing out of the ne

  IPv6NeighbourDiscovery::IPv6CDS::DCI it;

  bool success = mod->rt->cds->findDestEntry(bu->ha(), it);

  if (!success)
  {
    //CN's should already have a destEntry for the MN?  Assuming it was not away
    //from home before it initiated communications I guess
    assert(mod->isHomeAgent());

    if (mod->isHomeAgent())
    {
      //HA needs to destroy its knowledge of MN and set the neighbour to none
      (*mod->rt->cds)[bu->ha()].neighbour = boost::weak_ptr<IPv6NeighbourDiscovery::NeighbourEntry>();

      mod->rt->cds->findDestEntry(bu->ha(), it);
    }
  }
*/
  boost::weak_ptr<bc_entry> bce = mod->mipv6cds->findBinding(bu->ha());

  if (bce.lock().get() != 0)
  {
    bce.lock()->care_of_addr = dgram->srcAddress();
    if (mod->earlyBindingUpdate())
      bce.lock()->expires = TENTATIVE_BINDING_LIFETIME;
    else
        bce.lock()->expires = bu->expires();
    bce.lock()->seq_no = bu->sequence();
  }
  else
  {
    // create a new entry bu does not exists in bc
    bc_entry* be = new bc_entry;

    //Create a ctor for this
    be->home_addr = bu->ha();
    if (mod->earlyBindingUpdate())
      be->expires = TENTATIVE_BINDING_LIFETIME;
    else
        be->expires = bu->expires();
    be->is_home_reg = bu->homereg();
    be->seq_no = bu->sequence();
    // BSA set to zero as not being implemented yet ...
    be->bsa = 0;
    be->care_of_addr = dgram->srcAddress();

    // Sathya - Initialize prev_bu_time, and cell_resi_time
    be->prevBUTime = 0;
    be->avgCellResidenceTime = 0;

    if ( mod->isMobileNode() && mod->rt->isEwuOutVectorHODelays() )
      be->cellResidenceTimeVec = new cOutVector("CN cell residence time");

    bce = mod->mipv6cds->insertBinding(be);
    Dout(dc::custom, "bc "<<(*(mod->mipv6cds)));
  }

  // Sathya - When BU is received, note down BU_arrive_time
  bce.lock()->buArrivalTime = mod->simTime();

  // Sathya - cell_resi_time = AVE(BU_arrive_time - prev_bu_time);
  bce.lock()->avgCellResidenceTime = bce.lock()->buArrivalTime - bce.lock()->prevBUTime;

  if ( mod->isMobileNode() && mod->rt->isEwuOutVectorHODelays() )
  {
    bce.lock()->cellResidenceTimeVec->record(
      bce.lock()->buArrivalTime - bce.lock()->prevBUTime);
  }

  bce.lock()->prevBUTime = bce.lock()->buArrivalTime;
}

///Remove from binding cache
bool MIPv6MobilityState::deregisterBCE(BU* bu,  unsigned int ifIndex, IPv6Mobility* mod)
{
  return mod->mipv6cds->removeBinding(bu->ha());
}

/*
  @todo rev. 24 if bu did not have hoa option then no need for routing header
  otherwise use routing header with hoa ( prob. need to inc. a flag for this and
  how do we prevent forwarding module from adding a routing header which it does
  by default if bce for dest exists)
*/
void MIPv6MobilityState::sendBA(const ipv6_addr& srcAddr,
                                const ipv6_addr& destAddr,
                                BA* ba, IPv6Mobility* mod, simtime_t timestamp)
{
  IPv6Datagram* reply = new IPv6Datagram(srcAddr, destAddr, ba);
  reply->setTransportProtocol(IP_PROT_IPv6_MOBILITY);
  reply->setHopLimit(mod->ift->interfaceByPortNo(0)->ipv6()->curHopLimit);
  reply->setTimestamp( timestamp );
  Dout(dc::mipv6, mod->nodeName()<<" sending BA to "<<destAddr);

  //rev. 24
  //if ba status is rejecting due to bad nonce 136/7/8 then must not include the
  //binding auth data mob opt. Otherwise must be incoluded.
  //if destAddr is not unicast then BA not sent (this is checked in preProcessBU
  //and we do not process that BU any further)


  //No routing headers are added to message Draft 17 6.1.8
  mod->send(reply, "routingOut");
}

} // end namespace MobileIPv6
