// -*- C++ -*-
//
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
 * @file MIPv6MStateHomeAgent.cc
 * @author Eric Wu
 * @date 16.4.2002

 * @brief Implements functionality of Home Agent
 *
 */

#include "sys.h"
#include "debug.h"

#include <cassert>
#include <omnetpp.h>

#include "MIPv6MStateHomeAgent.h"
#include "IPv6Mobility.h"
#include "IPv6Datagram.h"
#include "MIPv6Entry.h"
#include "MIPv6DestOptMessages.h"
#include "MIPv6CDS.h"
#include "MobilityHeaders.h"
#include "RoutingTable6.h"
#include "InterfaceTable.h"
#include "IPv6InterfaceData.h"
#include "HdrExtDestProc.h"
#include "IPv6Encapsulation.h" //for tunneling of intercepted packets to HA
#include "opp_utils.h"
#include "IPv6CDS.h"

//buffer mipv6 packets
#include "RTPPacket.h"
//#include "RTCPPacket_m.h"
//separate voipframe functions out. esp aggregation ones
#include "RTPVoip.h"

namespace MobileIPv6
{

MIPv6MStateHomeAgent::MIPv6MStateHomeAgent(IPv6Mobility* mob):
  MIPv6MStateCorrespondentNode(mob), 
  tunMod(dynamic_cast<IPv6Encapsulation*>
	 (OPP_Global::findModuleByType(mob->rt, "IPv6Encapsulation")))
{}

MIPv6MStateHomeAgent::~MIPv6MStateHomeAgent(void)
{}

bool MIPv6MStateHomeAgent::
processMobilityMsg(IPv6Datagram* dgram)
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
    default:
      return MIPv6MStateCorrespondentNode::processMobilityMsg(dgram);
    break;
  }
  return true;
}

/**
 * @todo DAD defending and removal.
 *
 */

bool MIPv6MStateHomeAgent::processBU(BU* bu, IPv6Datagram* dgram)
{
  Dout(dc::mipv6|flush_cf, mob->nodeName()<<" "<<mob->simTime()<<" BU from "
       <<dgram->srcAddress()<<" lifetime="<<bu->expires());

  ipv6_addr hoa, coa;
  bool hoaOptFound;
  unsigned int ifIndex = dgram->inputPort();

  if (!preprocessBU(bu, dgram, hoa, coa, hoaOptFound))
    return false;

  Dout(dc::mipv6|flush_cf, mob->nodeName()<<" "<<mob->simTime()
       <<" coa="<<coa<<" hoa="<<hoa);

#ifndef USE_HMIP
  assert(bu->homereg());
#else
  assert(bu->homereg() || bu->mapreg());
#endif // USE_HMIP

  // check if the home address of the BU is onlink with respect to the home
  // agent's prefix list (Should also check for mapreg if hoa is an advertised
  // map prefix too)
  unsigned int ifIndexRef = 0;

  if (!mob->rt->cds->lookupAddress(hoa, ifIndexRef)
#ifdef USE_HMIP
      && !bu->mapreg()
#endif // #USE_HMIP
      )
  {
    BA* ba = new BA(BAS_NOT_HOME_SUBNET);
    sendBA(dgram->destAddress(), dgram->srcAddress(), hoa, ba, 0, ifIndex, false);
    Dout(dc::warning|dc::mipv6, " hoa="<<hoa<<" is not on link w.r.t. HA prefix list");
    return false;
  }

  // if the lifetime of BU is zero OR the MN returns to its home
  // subnet, perform Primary Care-of Address De-Registration
  if (bu->expires() == 0 || coa == hoa)
  {
    //assert(dgram->inputPort() > -1);
    if (dgram->inputPort() == -1)

    {
      Dout(dc::warning, mob->nodeName()<<" "<<mob->simTime()<<
           " set input port to zero when its -1 for BU");
      dgram->setInputPort(0);
    }

    if(!deregisterBCE(bu, hoa, (unsigned int)dgram->inputPort()))
    {
      BA* ba = new BA(BAS_NOT_HA_FOR_MN);
      sendBA(dgram->destAddress(), dgram->srcAddress(), hoa, ba, 0, ifIndex, false);
      Dout(dc::mipv6|dc::warning, " Failed pcoa de-registration for "
           <<dgram->srcAddress()<<" hoa="<<hoa);
      return false;
    }
    else
    {
      BA* ba = new BA(BAS_ACCEPTED, bu->sequence());
      sendBA(dgram->destAddress(), dgram->srcAddress(), hoa, ba, dgram->timestamp(),
	     ifIndex, false);
      Dout(dc::mipv6, " pcoa de-registration succeeded for "
           <<dgram->srcAddress()<<" hoa="<<hoa);
      return true;
    }
  }

  registerBCE(bu, hoa, dgram);

    BA* ba = new BA;
    ba->setStatus(BAS_ACCEPTED);
    ba->setSequence(bu->sequence());
    ba->setLifetime(bu->expires());

    sendBA(dgram->destAddress(), dgram->srcAddress(), hoa, ba, dgram->timestamp(),
	   ifIndex, false);

    Dout(dc::mipv6|flush_cf, mob->nodeName()<<" "<<mob->simTime()<<" BA sent to "
         <<dgram->srcAddress()<<" status="<<ba->status());

  return true;
}

void MIPv6MStateHomeAgent::bufferPackets(IPv6Datagram* dgram)
{
  cout<<"Buffering packet "<<*dgram<<endl;
  //find bce for mn if this is an rtp packet and store there
  if (dgram->transportProtocol() == IP_PROT_UDP)
  {
    RTPPacket* pkt = dynamic_cast<RTPPacket*> (dgram->encapsulatedMsg());
    if (!pkt || pkt->kind() != 1)
      return;
    
    VoipFrame* frame = static_cast<VoipFrame*> (pkt->contextPointer());
    //other non RTPVoip data
    if (!frame) 
      return;
    
    //assume single frames for now otherwise as only we do agg. no source agg yet
    cout<<"found frames time="<<frame->get<0>()<<" seqno="<<frame->get<1>()<<endl;
    //buffer downstream only i.e. to mn
    boost::weak_ptr<bc_entry> bce = mipv6cds->findBinding(dgram->destAddress());
    assert(bce.lock().get());
    //bce.lock()->pushFrame(*frame);
  }
}

/**
 * Create a binding for bu's home address option
 * @param bu Binding update from mobile node with home address option and care of address
 * @param dgram datagram which encapsulated bu
 * @param mob everpresent IPv6Mobility mobule
 *
 * @todo defend hoa and possibly link-local address if L bit set. Intercept
 * link local packets too if L set. Do DAD on hoa before accepting BU.
 *
 */

void MIPv6MStateHomeAgent::registerBCE(BU* bu, const ipv6_addr& hoa, IPv6Datagram* dgram)
{
  //Test that lifetime of bce is less than bu expires.
  //Test also prefix from which hoa is derived is not shorter than expires.
  boost::weak_ptr<bc_entry> bce = mipv6cds->findBinding(hoa);
  ipv6_addr oldcoa = IPv6_ADDR_UNSPECIFIED;
  if (bce.lock().get())
  {
    oldcoa = bce.lock()->care_of_addr;
  }
  //if bce not exists then do DAD on it and only after DAD successful then we
  //send back a BA. That will be tricky indeed
  //if (!bce.get())
  //do dad and then send self message for when dad successful
  //callback will sendBA and rest of this fn then multicast NA to all nodes addr on home link

  MIPv6MobilityState::registerBCE(bu, hoa, dgram);

  //retrieve updated bce (not necessary) or new bce
  bce = mipv6cds->findBinding(hoa);

  // The binding cache entry should have already been created by the
  // virtual function of MIPv6MobilityState
  assert(bce.lock() != 0);

  //Create tunnel from HA to MN for this binding (Sec. 9.4) so once packets
  //are intercepted we can send them to MN
  assert(tunMod != 0);


  //Was this a requirement of MIPv6? If so then HA cannot be a MAP either as MNs
  //will have moved to a map and bound with it first before sending first BU to
  //HA. This is only a problem if the MAP we bound with is also the HA as in
  //randomly generated scenario.
  //assert(dgram->inputPort() >= 0);



  /// This contains the real HA addr (Can this be anything else besides HA or
  /// MAP addr)
  ipv6_addr gaddr = dgram->destAddress();

  assert(gaddr != IPv6_ADDR_UNSPECIFIED);

  unsigned int vifIndex = tunMod->findTunnel(gaddr, bce.lock()->care_of_addr);

  if (vifIndex == 0)
  {
    vifIndex = tunMod->createTunnel(gaddr, bce.lock()->care_of_addr, dgram->inputPort());

    Dout(dc::mipv6|flush_cf, mob->nodeName()<<" Adding tunnel="<<gaddr<<"->"
         <<bce.lock()->care_of_addr<<" ifIndex of BU="<<dgram->inputPort()<<" hoa="<<hoa);

  }

  //Removing old tunnel which points to pcoa instead of ncoa
  if (oldcoa != IPv6_ADDR_UNSPECIFIED && oldcoa != bce.lock()->care_of_addr)
  {
    tunMod->destroyTunnel(gaddr, oldcoa);
    Dout(dc::debug|flush_cf, mob->nodeName()<<" removed old tunnel="<<gaddr<<"->"
         <<oldcoa);
  }

  //If L bit set means hoa is derived from link local
  //addr (formed from interface identifier). Also if L is set HA will protect
  //both hoa and link local address and do DAD for both. In this sim this L bit
  //will most likely be yes until we allow XML conf of home address too.

  if (bu->llac())
  {
    //TODO
    //derive lladdr from hoa
    //protect lladdr and hoa via dad 
    //also check hoa is either hoa or link local addr from tunnelled packets
    tunMod->tunnelDestination(bce.lock()->home_addr, vifIndex);
  }

}

/**
 * Remove binding for home address
 *
 * This function will remove the tunneling that is set up to intercept MN dest
 * and call base class to remove the Binding cache entry
 *
 * @param bu sent from MN
 * @param ifIndex of incoming BU
 * @param mob the usual mobule pointer
 *
 */

bool MIPv6MStateHomeAgent::deregisterBCE(BU* bu, const ipv6_addr& hoa, unsigned int ifIndex)
{
  boost::weak_ptr<bc_entry> bce = mipv6cds->findBinding(hoa);

  if (bce.lock().get() == 0)
    return false;

  assert(bce.lock().get() != 0);

  boost::weak_ptr<NeighbourEntry> ne = (*(mob->rt->cds))[hoa].neighbour;
  if (!ne.lock().get() || !tunMod->destroyTunnel(ne.lock().get()->ifIndex()))
  {
    Dout(dc::mipv6|flush_cf, mob->nodeName()<<":"<<ifIndex<<" failed to remove tunnel for hoa="<<hoa);
    assert(false);
  }

  return MIPv6MobilityState::deregisterBCE(bu, hoa, ifIndex);
}

/**
 * @param ifIndex should be the interface that HA messages exit from
 * @param mob ever present IPv6Mobility mobule to provide the context to work from
 * @return HA address on ifIndex interface with global or site scope
 * @deprecated As this is not used will be removed next revision
 */

ipv6_addr MIPv6MStateHomeAgent::globalAddr(unsigned int ifIndex) const
{
  InterfaceEntry *ie = mob->ift->interfaceByPortNo(ifIndex);
  for (unsigned int addrIndex = 0;  addrIndex < ie->ipv6()->inetAddrs.size(); addrIndex++)
  {
    if (ie->ipv6()->inetAddrs[addrIndex].scope() ==  ipv6_addr::Scope_Global ||
        ie->ipv6()->inetAddrs[addrIndex].scope() ==  ipv6_addr::Scope_Site
        )
    {
      return ie->ipv6()->inetAddrs[addrIndex];
    }
  }
  return IPv6_ADDR_UNSPECIFIED;
}

} // end namespace MobileIPv6
