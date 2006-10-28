// -*- C++ -*-
// Copyright (C) 2002, 2004 CTIE, Monash University
// Copyright (C) 2006 Johnny Lai
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
#include "IPProtocolId_m.h"
#include "InterfaceTable.h"
#include "IPv6InterfaceData.h"
#include "IPv6CDS.h"

#include "MIPv6DestOptMessages.h"
#include "MobilityHeaders.h"
#include "MIPv6Entry.h"

#include "MIPv6CDS.h"

namespace MobileIPv6
{

const int TENTATIVE_BINDING_LIFETIME = 3;

MIPv6MobilityState::MIPv6MobilityState(IPv6Mobility* mod):
  mob(mod), mipv6cds(new MIPv6CDS)
{
}

MIPv6MobilityState::~MIPv6MobilityState(void)
{
  delete mipv6cds;
}

void MIPv6MobilityState::initialize(int stage)
{
}

bool MIPv6MobilityState::processReceivedHoADestOpt(ipv6_addr hoa, IPv6Datagram* dgram)
{
  ipv6_addr  coa = dgram->srcAddress();
  bool bindingTest = true;
  if (dgram->transportProtocol() == IP_PROT_IPv6_MOBILITY)
  {
    MobilityHeaderBase* mhb = check_and_cast<MobilityHeaderBase*>(dgram->encapsulatedMsg());
    bindingTest = mhb->kind() != MIPv6MHT_BU;
  }

  if (bindingTest)
  {
    ///Drop if no binding for this home addr exists with matching care of addr as
    ///src addr of this packet. 
    bool dropped = false;
    boost::weak_ptr< bc_entry > bce = mipv6cds->findBinding(hoa);    
    if (!bce.lock().get())
      dropped = true;
    if (bce.lock()->care_of_addr != coa)
      dropped = true;
    if (dropped)
    {
      //processReceivedHoADestOpt has not run yet so do not reverse its effects
      sendBE(1, dgram);
      return false;
    }
  }
  return true;
}

//RFC 3775 Sec. 9.2
MobilityHeaderBase* MIPv6MobilityState::mobilityHeaderExists(IPv6Datagram* dgram)
{
  MobilityHeaderBase* mhb = 0;
  cMessage *pkt = dgram->encapsulatedMsg();
  mhb = check_and_cast<MobilityHeaderBase*>(pkt);

  if (!mhb)
  {
    opp_error("Unknown packet received in MIPv6MobilityState::processMobilityMsg");
    delete dgram;
  }
  if (mhb->encapsulatedMsg())
  {
    opp_error("IPPROTO_NONE violated from RFC 3775 9.2 i.e. should be no payload");
    //send icmp param problem 0 to src of dgram
  } 
  //TODO check length of headers according to type of header
  return mhb;

}

//TODO rate limit like icmp messages
void MIPv6MobilityState::sendBE(int status, IPv6Datagram* dgram)
{
  HdrExtDestProc* proc = static_cast<HdrExtDestProc*>(dgram->findHeader(EXTHDR_DEST));
  ipv6_addr origSrc = dgram->srcAddress();
  ipv6_addr hoa = IPv6_ADDR_UNSPECIFIED;

  if (proc)
  {    
    MIPv6TLVOptHomeAddress* hoaOpt = static_cast<MIPv6TLVOptHomeAddress*>(proc->getOption(IPv6TLVOptionBase::MIPv6_HOME_ADDRESS_OPT));
    if (hoaOpt)
    {
	hoa = hoaOpt->homeAddr();
    }
  }

  if (origSrc.isMulticast())
    return;

  IPv6Datagram* error = new IPv6Datagram(dgram->destAddress(), origSrc);
  error->setTransportProtocol(IP_PROT_IPv6_MOBILITY);
  error->setHopLimit(mob->ift->interfaceByPortNo(0)->ipv6()->curHopLimit);
  error->encapsulate(new BE(status, hoa));

  mob->send(error, "routingOut");
}

void MIPv6MobilityState::defaultResponse(MobilityHeaderBase* mhb, IPv6Datagram* dgram)
{
  DoutFatal(dc::core|dc::warning,
                " Mobile IPv6 Mobility Header not recognised ... "<< mhb->kind());
  sendBE(2, dgram);
  delete dgram;
}

/**
 *  Checks for valid BU see Sec. 9.5.1 in mip6 rfc
 *
 */

bool MIPv6MobilityState::preprocessBU(BU* bu, IPv6Datagram* dgram, ipv6_addr& hoa, ipv6_addr& coa, bool& hoaOptFound)
{
  // check if the BU is valid before processing it

  // draft 16, page 82 "The packet meets the specific authentication
  // requirements for Binding Updates, defined in section 4.5
  // ... NOT IMPLEMENTED.. :-)


  hoa = dgram->srcAddress();
  coa = dgram->srcAddress();

  //TODO send ba's to rt type 2 of hoa if hoa option included otherwise just to src addr
  hoaOptFound = false;

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
      Dout(dc::mipv6, mob->nodeName()<<" home address opt hoa="<<hoa);
      hoaOptFound = true;
    }
  }

  if (hoa.isMulticast())
  {
    Dout(dc::mipv6, mob->nodeName()<<" BU has non unicast home address");
    return false;
  }

  if (ipv6_addr_scope(hoa) == ipv6_addr::Scope_None ||
      ipv6_addr_scope(hoa) == ipv6_addr::Scope_Node ||
      ipv6_addr_scope(hoa) == ipv6_addr::Scope_Link)
  {
    Dout(dc::mipv6, mob->nodeName()<<" BU has non routable home address");
    return false;
  }

  //TODO ACOA retrieval
  /* 
  MIPv6MHParameterBase* acoaExist = bu->parameter(MIPv6MHPT_ACoA);
  if (acoaExist)
  {
    MIPv6MHAlternateCareofAddress* acoaPar = boost::polymorphic_downcast<MIPv6MHAlternateCareofAddress*>(acoaExist);

    assert(acoaPar);
    coa = acoaPar->address();
    Dout(dc::mipv6, mob->nodeName()<<" alternate coa="<<coa);
  }
  */
  // check if the sequence number is greater than the last successful
  // binding update

  boost::weak_ptr<bc_entry> bce = mipv6cds->findBinding(hoa);
  if (bce.lock().get() != 0 && OPP_Global::lessThanEqualsModulo(bu->sequence(), bce.lock()->seqNo()))
  {
    BA* ba = new BA(BAS_SEQ_OUT_OF_WINDOW, bce.lock()->seqNo(),
                    bce.lock()->expires);

    sendBA(dgram->destAddress(), dgram->srcAddress(), hoa, ba);
    Dout(dc::mipv6|dc::warning, " BU seq="<<bu->sequence()<<" < BC entry seq="<<bce.lock()->seqNo());
    return false;
  }

  if (bce.lock().get() != 0 && bce.lock()->is_home_reg != bu->homereg())
  {
    BA* ba = new BA(BAS_REG_TYPE_CHANGE_DIS, bce.lock()->seqNo(),
                    bce.lock()->expires);
    sendBA(dgram->destAddress(), dgram->srcAddress(), hoa, ba);
    Dout(dc::warning|dc::mipv6, " BU has different setting from bce for home_reg flag="<<(bu->homereg()));
    return false;
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
void MIPv6MobilityState::registerBCE(BU* bu, const ipv6_addr& hoa, IPv6Datagram* dgram)
{
  boost::weak_ptr<bc_entry> bce = mipv6cds->findBinding(hoa);

  if (bce.lock().get() != 0)
  {
    bce.lock()->care_of_addr = dgram->srcAddress();
    if (mob->earlyBindingUpdate())
      bce.lock()->expires = TENTATIVE_BINDING_LIFETIME;
    else
        bce.lock()->expires = bu->expires();
    bce.lock()->setSeqNo(bu->sequence());
  }
  else
  {
    // create a new entry bu does not exists in bc
    bc_entry* be = new bc_entry;

    // initialise the count for number of handovers then increment later
    be->handoverCount = 0;

    //Create a ctor for this
    be->home_addr = hoa;
    if (mob->earlyBindingUpdate())
      be->expires = TENTATIVE_BINDING_LIFETIME;
    else
        be->expires = bu->expires();
    be->is_home_reg = bu->homereg();
    be->setSeqNo(bu->sequence());
    // BSA set to zero as not being implemented yet ...
    be->bsa = 0;
    be->care_of_addr = dgram->srcAddress();

    // Sathya - Initialize prev_bu_time, and cell_resi_time
    be->prevBUTime = 0;
    be->avgCellResidenceTime = 0;

    if ( mob->isMobileNode() && mob->isEwuOutVectorHODelays() )
      be->cellResidenceTimeVec = new cOutVector("CN cell residence time");

    bce = mipv6cds->insertBinding(be);
    Dout(dc::custom, "bc "<<(*(mipv6cds)));
  }

  // Sathya - When BU is received, note down BU_arrive_time
  bce.lock()->buArrivalTime = mob->simTime();

  // Sathya - cell_resi_time = AVE(BU_arrive_time - prev_bu_time);
  double totalUpTime = bce.lock()->avgCellResidenceTime * bce.lock()->handoverCount;
  double prevCellResTime = bce.lock()->buArrivalTime - bce.lock()->prevBUTime;

  bce.lock()->avgCellResidenceTime =  ( totalUpTime + prevCellResTime ) / 
    (bce.lock()->handoverCount + 1);

  if ( mob->isMobileNode() && mob->isEwuOutVectorHODelays() )
  {
    bce.lock()->cellResidenceTimeVec->record(
      bce.lock()->buArrivalTime - bce.lock()->prevBUTime);
  }

/*  // must need cell residency signaling support from both MN and CN
  if ( mob->signalingEnhance() == CellResidency && bu->cellSignaling() )
  {
    double totalHandoverTime = bce.lock()->avgHandoverDelay * bce.lock()->handoverCount;      

    bce.lock()->avgHandoverDelay = ( totalHandoverTime +
                                     static_cast<HandoverDelay*>(bu->mobilityOption(MOPT_HandoverDelay))->delay() ) /
      (bce.lock()->handoverCount + 1) ;      
  }
*/
  bce.lock()->prevBUTime = bce.lock()->buArrivalTime;
  bce.lock()->handoverCount++;
}

///Remove from binding cache
bool MIPv6MobilityState::deregisterBCE(BU* bu, const ipv6_addr& hoa, unsigned int ifIndex)
{
  return mipv6cds->removeBinding(hoa);
}

/*
  @todo rev. 24 if bu did not have hoa option then no need for routing header
  otherwise use routing header with hoa ( prob. need to inc. a flag for this and
  how do we prevent forwarding module from adding a routing header which it does
  by default if bce for dest exists)
*/
void MIPv6MobilityState::sendBA(const ipv6_addr& srcAddr,
                                const ipv6_addr& destAddr,
				const ipv6_addr& hoa,
                                BA* ba, simtime_t timestamp)
{
  if ( destAddr.isMulticast() )
    return;

  IPv6Datagram* reply = new IPv6Datagram(srcAddr, destAddr, ba);
  reply->setKind(1);
  reply->setTransportProtocol(IP_PROT_IPv6_MOBILITY);
  reply->setHopLimit(mob->ift->interfaceByPortNo(0)->ipv6()->curHopLimit);
  reply->setTimestamp( timestamp );
  Dout(dc::mipv6, mob->nodeName()<<" sending BA to "<<destAddr);

  //rev. 24
  //if ba status is rejecting due to bad nonce 136/7/8 then must not include the
  //binding auth data mob opt. Otherwise must be there.

  //TODO send ba's to rt type 2 of hoa if hoa option included otherwise just to src addr


  mob->send(reply, "routingOut");
}

} // end namespace MobileIPv6
