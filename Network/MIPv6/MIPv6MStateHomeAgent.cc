// -*- C++ -*-
//
// Copyright (C) 2002, 2004 CTIE, Monash University
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
#include "MIPv6MobilityHeaders.h"
#include "MIPv6Entry.h"
#include "MIPv6DestOptMessages.h"
#include "MIPv6CDS.h"
#include "RoutingTable6.h"
#include "HdrExtDestProc.h"
#include "IPv6Encapsulation.h" //for tunneling of intercepted packets to HA
#include "opp_utils.h"
#include "IPv6CDS.h"



namespace MobileIPv6
{

MIPv6MStateHomeAgent* MIPv6MStateHomeAgent::_instance = 0;

MIPv6MobilityState* MIPv6MStateHomeAgent::instance(void)
{
  if(_instance == 0)
    _instance = new MIPv6MStateHomeAgent;

  return _instance;
}

MIPv6MStateHomeAgent::MIPv6MStateHomeAgent(void)
{}

MIPv6MStateHomeAgent::~MIPv6MStateHomeAgent(void)
{}

void MIPv6MStateHomeAgent::
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

    default:
      cerr << "Mobile IPv6 Mobility Header not recognised ... " << endl;
    break;
  }
}

/**
 * @todo Draft 18 no Home address in BU it is a home addr destination option only
 * DAD defending and removal.
 *
 */

bool MIPv6MStateHomeAgent::processBU(IPv6Datagram* dgram, BU* bu,
                                             IPv6Mobility* mod)
{
  Dout(dc::mipv6|flush_cf, mod->nodeName()<<" "<<mod->simTime()<<" BU from "
       <<dgram->srcAddress());

  ipv6_addr hoa, coa;

  if (!preprocessBU(dgram, bu, mod, hoa, coa))
    return false;


#ifndef USE_HMIP
  assert(bu->homereg());
#else
  assert(bu->homereg() || bu->mapreg());
#endif // USE_HMIP

  // check if the home address of the BU is onlink with respect to the home
  // agent's prefix list (Should also check for mapreg if hoa is an advertised
  // map prefix too)
  unsigned int ifIndexRef = 0;

  if (!mod->rt->cds->lookupAddress(hoa, ifIndexRef)
#ifdef USE_HMIP
      && !bu->mapreg()
#endif // #USE_HMIP
      )
  {
    BA* ba = new BA(BA::BAS_NOT_HOME_SUBNET, UNDEFINED_SEQ,
                    UNDEFINED_EXPIRES, UNDEFINED_REFRESH);

    sendBA(dgram->destAddress(), dgram->srcAddress(), ba, mod);
    Dout(dc::warning|dc::mipv6, " hoa="<<bu->ha()<<" is not on link w.r.t. HA prefix list");

    return false;
  }

  // if the lifetime of BU is zero OR the MN returns to its home
  // subnet, perform Primary Care-of Address De-Registration
  if (bu->expires() == 0 || coa == hoa)
  {
    assert(dgram->inputPort() > -1);

    if(!deregisterBCE(bu, (unsigned int)dgram->inputPort(), mod))
    {
      BA* ba = new BA(BA::BAS_NOT_HA_FOR_MN, UNDEFINED_SEQ,
                      UNDEFINED_EXPIRES, UNDEFINED_REFRESH);
      sendBA(dgram->destAddress(), dgram->srcAddress(), ba, mod);
      Dout(dc::mipv6|dc::warning, " Failed pcoa de-registration for "
           <<dgram->srcAddress()<<" hoa="<<bu->ha());
      return false;
    }
    else
    {
      BA* ba = new BA(BA::BAS_ACCEPTED, bu->sequence(), 0, 0);
      sendBA(dgram->destAddress(), dgram->srcAddress(), ba, mod);
      Dout(dc::mipv6, " pcoa de-registration succeeded for "
           <<dgram->srcAddress()<<" hoa="<<bu->ha());
      return true;
    }
  }

  registerBCE(dgram, bu, mod);

  //rev. 24 always sending back a BA regardless of A bit. Also some prefix
  //discovery clause i.e. status value of 1 if prefix will expire during
  //lifetime or after expires.

  // bu is legal, if bu in which A bit is set, send the BA back to the
  // sending MN
//  if ( bu->ack() )
//  {
    BA* ba = new BA(BA::BAS_ACCEPTED, bu->sequence(), bu->expires(),
                    bu->expires());

  //Ensure that dgram dest address and type 2 routing header is used only when
  //appropriate see sendBA comments

    sendBA(dgram->destAddress(), dgram->srcAddress(), ba, mod);

    Dout(dc::mipv6|flush_cf, mod->nodeName()<<" "<<mod->simTime()<<" BA sent to "
         <<dgram->srcAddress()<<" status="<<ba->status());

//  }


  return true;
}

/**
 * Create a binding for bu's home address option
 * @param bu Binding update from mobile node with home address option and care of address
 * @param dgram datagram which encapsulated bu
 * @param mob everpresent IPv6Mobility module
 *
 * @todo form other addr from home address for all on link prefixes of MN except
 * link local prefix for tunneling purpose when S is not set. Well 9.1 also says
 * form a different bce for each possible routing prefix supported by HA.
 *
 */

void MIPv6MStateHomeAgent::
registerBCE(IPv6Datagram* dgram, BU* bu, IPv6Mobility* mob)
{
  //if bce not exists then do DAD on it and only after DAD successful then we
  //send back a BA. That will be tricky indeed
  //boost::weak_ptr<bc_entry> bce = mob->mipv6cds->findBinding(bu->ha());
  //if (!bce.get())
  //do dad and then send self message for when dad successful
  //callback will sendBA and rest of this fn then multicast NA to all nodes addr on home link

  //Test that lifetime of bce is less than bu expires.
  //Test also prefix from which hoa is derived is not shorter than expires.
  boost::weak_ptr<bc_entry> bce = mob->mipv6cds->findBinding(bu->ha());
  ipv6_addr oldcoa = IPv6_ADDR_UNSPECIFIED;
  if (bce.lock().get())
  {
    oldcoa = bce.lock()->care_of_addr;
  }

  MIPv6MobilityState::registerBCE(dgram, bu, mob);

  bce = mob->mipv6cds->findBinding(bu->ha());

  // The binding cache entry should have already been created by the
  // virtual function of MIPv6MobilityState
  assert(bce.lock() != 0);

  //Create tunnel from HA to MN for this binding (Sec. 9.4) so once packets
  //are intercepted we can send them to MN
  if (!mob->mipv6cds->tunMod)
     mob->mipv6cds->tunMod = check_and_cast<IPv6Encapsulation*>
       (OPP_Global::findModuleByType(mob->rt, "IPv6Encapsulation"));
  IPv6Encapsulation* tunMod = mob->mipv6cds->tunMod;
  assert(tunMod != 0);

  assert(dgram->inputPort() >= 0);



  //Find HA addr (this is wrong as we could get wrong HA addr if HA serves as
  //diff HA on each iface and since HA addr is global can come on any iface anyway)
  //ipv6_addr gaddr = globalAddr(static_cast<unsigned int>(dgram->inputPort()), mob);

  /// This contains the real HA addr (Can this be anything else besides HA or
  /// MAP addr)
  ipv6_addr gaddr = dgram->destAddress();

  assert(gaddr != IPv6_ADDR_UNSPECIFIED);

  unsigned int vifIndex = tunMod->findTunnel(gaddr, bce.lock()->care_of_addr);

  if (vifIndex == 0)
  {
    vifIndex = tunMod->createTunnel(gaddr, bce.lock()->care_of_addr, dgram->inputPort());

    Dout(dc::mipv6|flush_cf, mob->nodeName()<<" Adding tunnel="<<gaddr<<"->"
         <<bce.lock()->care_of_addr<<" ifIndex of BU="<<dgram->inputPort()<<" hoa="<<bu->ha());

  }

  //Removing old tunnel which points to pcoa instead of ncoa
  if (oldcoa != IPv6_ADDR_UNSPECIFIED && oldcoa != bce.lock()->care_of_addr)
  {
    tunMod->destroyTunnel(gaddr, oldcoa);
    Dout(dc::debug|flush_cf, mob->nodeName()<<" removed old tunnel="<<gaddr<<"->"
         <<oldcoa);
  }

  //rev. 24 no sa only L bit. If L bit set means hoa is derived from link local
  //addr (formed from interface identifier). Also if L is set HA will protect
  //both hoa and link local address and do DAD for both. In this sim this L bit
  //will most likely be yes until we allow XML conf of home address too.

  //Do we need sa in bce?  Probably not
  //as all consequent actions from its value can be activated when the binding
  //is created (what happens when update specifies a diff single address bit?)
  if (!bu->saonly())
  {
    ///TODO form other addr home address for all on link prefixes of MN except link local
    ///prefix. Well 9.1 also says form a different bce for each possible routing
    ///prefix supported by HA does that mean all global addresses that are on
    ///link or any routable address including site-local since we can tunnel
    ///those to MN(9.4) if MN still at same site but diff subnet?
    tunMod->tunnelDestination(bce.lock()->home_addr, vifIndex);
  }
  else
  {
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
 * @param mob the usual module pointer
 *
 */

bool MIPv6MStateHomeAgent::deregisterBCE(BU* bu,  unsigned int ifIndex, IPv6Mobility* mob)
{
  boost::weak_ptr<bc_entry> bce = mob->mipv6cds->findBinding(bu->ha());

  // The binding cache entry should have already been created by the
  // virtual function of MIPv6MobilityState
  //Is it possible that ha did not receive any more bindings? and so when mn returns
  //home bindings expired already?
  assert(bce.lock().get() != 0);


  if (!mob->mipv6cds->tunMod)
    mob->mipv6cds->tunMod = check_and_cast<IPv6Encapsulation*>
      (OPP_Global::findModuleByType(mob->rt, "IPv6Encapsulation"));
  IPv6Encapsulation* tunMod = mob->mipv6cds->tunMod;

  boost::weak_ptr<NeighbourEntry> ne = (*(mob->rt->cds))[bu->ha()].neighbour;
  if (!ne.lock().get() || !tunMod->destroyTunnel(ne.lock().get()->ifIndex()))
  {
    Dout(dc::mipv6|flush_cf, mob->nodeName()<<":"<<ifIndex<<" failed to remove tunnel for hoa="<<bu->ha());
    assert(false);
  }

  return MIPv6MobilityState::deregisterBCE(bu, ifIndex, mob);
}

/**
 * @param ifIndex should be the interface that HA messages exit from
 * @param mod ever present IPv6Mobility module to provide the context to work from
 * @return HA address on ifIndex interface with global or site scope
 * @deprecated As this is not used will be removed next revision
 */

ipv6_addr MIPv6MStateHomeAgent::globalAddr(unsigned int ifIndex,  IPv6Mobility* mod) const
{
  Interface6Entry& ie = mod->rt->getInterfaceByIndex(ifIndex);
  for (unsigned int addrIndex = 0;  addrIndex < ie.inetAddrs.size(); addrIndex++)
  {
    if (ie.inetAddrs[addrIndex].scope() ==  ipv6_addr::Scope_Global ||
        ie.inetAddrs[addrIndex].scope() ==  ipv6_addr::Scope_Site
        )
    {
      return ie.inetAddrs[addrIndex];
    }
  }
  return IPv6_ADDR_UNSPECIFIED;
}

} // end namespace MobileIPv6
