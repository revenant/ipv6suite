// -*- C++ -*-
// Copyright (C) 2006 by Johnny Lai
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
 * @file MIPv6MStateMobileNode.cc
 * @author Eric Wu, Johnny Lai
 * @date 16.4.2002
 *
 * @brief Implements functionality of Mobile Node
 */

#include "sys.h"
#include "debug.h"


#include <iomanip> //setprecision
#include <string>
#include <iostream>
#include <boost/cast.hpp>
#include <boost/bind.hpp>

#include "MIPv6MStateMobileNode.h"
#include "cSignalMessage.h"
#include "IPv6Datagram.h"
#include "IPv6Mobility.h"
#include "MobilityHeaders.h"
#include "MIPv6CDSMobileNode.h"
#include "MIPv6Entry.h"
#include "MIPv6MNEntry.h"
#include "MIPv6DestOptMessages.h" //MIPv6OptHomeAddress
#include "opp_utils.h"
#include "IPv6Encapsulation.h"
#include "RoutingTable6.h" // for sendBU
#include "InterfaceTable.h"
#include "IPv6InterfaceData.h"
#include "MobilityOptions_m.h"
#include "MIPv6Timers.h" //MIPv6PeriodicCB
#include "MobilityHeaderBase_m.h"
#include "TimerConstants.h" // SELF_SCHEDULE_DELAY

#ifdef USE_HMIP
#include "HMIPv6CDSMobileNode.h"
using HierarchicalMIPv6::HMIPv6CDSMobileNode;
#if EDGEHANDOVER
#include "NeighbourDiscovery.h"
#include "EHNDStateHost.h"
#include "EHTimers.h"
#include "EHCDSMobileNode.h"
#endif //EDGEHANDOVER
#endif //USE_HMIP

#include "IPvXAddress.h"
#include "IPAddressResolver.h"
#include "IPv6CDS.h"


#if (defined OPP_VERSION && OPP_VERSION >= 3) || OMNETPP_VERSION >= 0x300
#include "opp_utils.h" //getParser()
#include "XMLOmnetParser.h"
#include "XMLCommon.h"
#endif

namespace MobileIPv6
{

const simtime_t CELL_RESI_THRESHOLD = 4;


  ///dgram contains bu
void recordBUVector(IPv6Datagram* dgram, IPv6Mobility* mob, MIPv6CDS* mipv6cds)
{
  //if dgram->name() == EarlyBU then record ebu time and same for BA
  const simtime_t now = mob->simTime();
  if (mipv6cds->mipv6cdsMN->primaryHA().get() && 
      dgram->destAddress() == mipv6cds->mipv6cdsMN->primaryHA()->addr())
  {
    mob->buVector->record(now);
  }
#ifdef USE_HMIP
  else if (mob->hmipSupport())
  {
#if EDGEHANDOVER
    if (mob->edgeHandover() && dgram->destAddress() == mipv6cds->ehcds->boundMapAddr())
    {
      mob->bbuVector->record(now);
    } else
#endif //EDGEHANDOVER
      if (mipv6cds->hmipv6cdsMN->isMAPValid() && 
	  mipv6cds->hmipv6cdsMN->currentMap().addr() == dgram->destAddress())
      {
	mob->lbuVector->record(now);
      }
  }
#endif //USE_HMIP
}

void recordBAVector(IPv6Datagram* dgram, IPv6Mobility* mob, MIPv6CDS* mipv6cds)
{
  const simtime_t now = mob->simTime();
  if (dgram->srcAddress() == mipv6cds->mipv6cdsMN->primaryHA()->addr())
    mob->backVector->record(now);
#ifdef USE_HMIP
  else if (mob->hmipSupport())
  {
#if EDGEHANDOVER
    if (mob->edgeHandover() && dgram->srcAddress() == mipv6cds->ehcds->boundMapAddr())
    {
      mob->bbackVector->record(now);
    } 
    else
#endif //EDGEHANDOVER
      if (mipv6cds->hmipv6cdsMN->isMAPValid() && mipv6cds->hmipv6cdsMN->currentMap().addr() == dgram->srcAddress())
      {
	mob->lbackVector->record(now);
      }
  }
#endif //USE_HMIP

}

class BURetranTmr;
typedef std::list<BURetranTmr*> BURetranTmrs;
typedef BURetranTmrs::iterator BURTI;

/**
   @class BURetranTmr
   @brief Implement Binding Update retransmissions
*/

class BURetranTmr: public cSignalMessage
{
  friend class MIPv6MStateMobileNode;

public:

  // Constructor/destructor.
  BURetranTmr(unsigned int timeout, IPv6Datagram* dgram,
              MIPv6MStateMobileNode* mn, IPv6Mobility* mob, MIPv6CDS* mipv6cds)
    :cSignalMessage("BURetransmissionTmr", Tmr_BURetransmission), timeout(timeout),
     dgram(dgram), stateMN(mn), mipv6cds(mipv6cds)
    {
      connect(boost::bind(&BURetranTmr::retransmitBU, this));
    }
  virtual ~BURetranTmr(void)
    {
//stop it from dumping core at end run of sim
//TODO LEAK
//      delete dgram;
    };

  ///Exponential backoff till timeout >= MAX_BINDACK_TIMEOUT
  void retransmitBU()
    {
      string nodeName =
        //check_and_cast<IPv6Mobility*>(module())->nodeName();
        static_cast<IPv6Mobility*>(module())->nodeName();

      if (dgram->destAddress() == IPv6_ADDR_UNSPECIFIED)
      {
        Dout(dc::warning|flush_cf, nodeName<<" "<<simulation.simTime()
             <<" todo find out why BU to unspecified address (removing timer)");
        assert(dynamic_cast<IPv6Mobility*>(mod));
        stateMN->removeBURetranTmr(this);
        return;
      }

      if (timeout < MAX_BINDACK_TIMEOUT)
      {
        timeout *=2;
      }
      else
      {
        bu_entry* bule = mipv6cds->mipv6cdsMN->findBU(dgram->destAddress());
        assert(bule);
        bule->problem = true;
        bule->state = 0;
        if (mipv6cds->mipv6cdsMN->primaryHA()->addr() == dgram->destAddress())
        {
          cerr<< " **************************************** " << endl;
          cerr<< nodeName <<" at " << module()->simTime() 
              << " sec, MAX_BINDACK_TIMEOUT! Unable to receive BAck from HA "<< endl << endl;
          cerr<<" Suggestion: check Router IPv6 address and advertising prefix in XML" << endl << endl;
          cerr<< " **************************************** " << endl;
	  Dout(dc::warning, nodeName <<" "<< module()->simTime()<<" MAX_BINDACK_TIMEOUT! Unable to receive BAck from primary HA bule="<<*bule);
        }
	else
	{
	  Dout(dc::notice, nodeName <<" "<< module()->simTime()<<" MAX_BINDACK_TIMEOUT! Unable to receive BAck from bule="<<*bule);
	}
        
        stateMN->removeBURetranTmr(this);
        return;
      }

      //Update binding entry with this retransmission
      bu_entry* bule = mipv6cds->mipv6cdsMN->findBU(dgram->destAddress());
      if (!bule)
      {
        Dout(dc::warning|dc::mipv6|flush_cf, nodeName<<" "<<module()->simTime()
             <<" bule missing very bad "<<dgram->destAddress());
        stateMN->removeBURetranTmr(this);
        return;
      }
      assert(bule != 0);

      bule->state++;
      bule->setSequence(bule->sequence()+1);
      bule->last_time_sent = module()->simTime();

      //reschedule a transmission if BA does not arrive in timeout seconds
      rescheduleDelay(timeout);

      //Resend copy of dgram containing updated BU
      BU* bu = check_and_cast<BU*> (dgram->encapsulatedMsg());
      bu->setSequence(bule->sequence());
      module()->send(dgram->dup(), "routingOut");

      IPv6Mobility* mob = static_cast<IPv6Mobility*>(mod);
      simtime_t now = module()->simTime();
      recordBUVector(dgram, mob, mipv6cds);

      assert(dgram->destAddress() != IPv6_ADDR_UNSPECIFIED);
      Dout(dc::mipv6|flush_cf, nodeName<<" "<<now<<" Resending BU for the "
           <<dgram->destAddress()<< " bu="<<*bu);

    }

private:
  ///now-msg->sendingTime() = timeout
  unsigned int timeout;
  BURetranTmr();
public:
  IPv6Datagram* dgram;
  MIPv6MStateMobileNode* stateMN;
  MIPv6CDS* mipv6cds;

};

std::ostream& operator<<(std::ostream& os,
                         const MobileIPv6::BURetranTmr& burtmr)
{
  return os<< "bu timer to "<<*(burtmr.dgram);
}

std::ostream& operator<<(std::ostream& os,
                         const MobileIPv6::MIPv6MStateMobileNode::BURetranTmrs& list)
{
  copy(list.begin(), list.end(), ostream_iterator<
       MobileIPv6::MIPv6MStateMobileNode::BURetranTmrs::value_type >(os, "\n"));
  return os;
}

/**
 * @class sendBUs
 *
 * @brief functor to update every CN/HA in BUL whenever we do a handoever with same hoa
 *
 * @todo lifetime of bindings currently is fixed to the minimum current lifetime
 * of all prefixes rather than just min(home prefix, foreign prefix)
 */

struct sendBUs: public std::unary_function<bu_entry*, void>
{
  sendBUs(const ipv6_addr& coa, const ipv6_addr& oldcoa, const ipv6_addr& hoa,
          unsigned int lifetime, IPv6Mobility* mob, MIPv6CDSMobileNode* cds)
    :mipv6cdsMN(cds), coa(coa), oldcoa(oldcoa), hoa(hoa), lifetime(lifetime),
     mob(mob), mstateMN(boost::polymorphic_downcast<MIPv6MStateMobileNode*>
                        (mob->role))
    {
      //Retain copy of previous coa otherwise will be overwritten by the new updates
//      assert(oldcoa != IPv6_ADDR_UNSPECIFIED && oldcoa != mipv6cdsMN->homeAddr());
      assert(mstateMN != 0);
    }

  ///Will not update MAPs or entries with different hoa
  void operator()(boost::shared_ptr<bu_entry>& bule)
    {
      //Some CN may have requested us for one already?
      if (bule->careOfAddr() == coa && bule->state > 0 )
      {
        Dout(dc::mipv6, mob->nodeName()<<" outstanding BU already sent to "<<
             bule->addr()<<" for coa="<<coa);
        return;
      }

#ifdef USE_HMIP
      if (bule->isMobilityAnchorPoint())
      {
        Dout(dc::hmip, mob->nodeName()<<" skipping MAP "<<bule->addr()
             <<" in sendBUs");
        return;
      }
#endif //USE_HMIP

      if (bule->homeAddr() != hoa)
      {
         Dout(dc::hmip, mob->nodeName()<<" skipping binding "<<bule->addr()
              <<"with different hoa="<<bule->homeAddr() <<" in sendBUs");
	 return;
      }

      bool homeReg = bule->homeReg();
      if (mob->returnRoutability() && !homeReg)
      {
        mstateMN->sendInits(bule->addr(), coa);
      }
      else
      {
        mstateMN->sendBU(bule->addr(), coa, bule->homeAddr(),
                         //we should really be using this instead but as timers
                         //on binding updates not implemented yet we will use
                         //the min lifetime of all addr

                         //bule->expires(),
                         lifetime,
                         homeReg,
                         //don't care about ifIndex as dad only done on
                         //primaryHA and only the very first BU to it
                         mipv6cdsMN->primaryHA()->re.lock()->ifIndex());
      }
    }

private:
  MIPv6CDSMobileNode* mipv6cdsMN;
  const ipv6_addr& coa;
  const ipv6_addr& oldcoa;
  const ipv6_addr& hoa;

  unsigned int lifetime;
  IPv6Mobility* mob;
  MIPv6MStateMobileNode* mstateMN;
};

MIPv6MStateMobileNode::MIPv6MStateMobileNode(IPv6Mobility* mod):
  MIPv6MStateCorrespondentNode(mod), buRetranTmrs(BURetranTmrs()),
  mipv6cdsMN(0), hmipv6cds(0), ehcds(0), tunMod(0)
{
  InterfaceTable *ift = mod->ift;
  mipv6cdsMN = new MIPv6CDSMobileNode(ift->numInterfaceGates());
  mipv6cds->mipv6cdsMN = mipv6cdsMN;
  
  ((MIPv6PeriodicCB*)(periodTmr))->connect(boost::bind(&MIPv6CDSMobileNode::expireLifetimes,
				mipv6cdsMN, periodTmr));

	mob->backVector = new cOutVector("BAck recv");
	mob->buVector = new cOutVector("BU sent");
	mob->lbuVector = new cOutVector("LBU sent");
	mob->lbackVector = new cOutVector("LBAck recv");
	//BU to Bound map
	mob->bbuVector = new cOutVector("BBU sent");
    mob->bbackVector = new cOutVector("BBAck recv");

    tunMod = check_and_cast<IPv6Encapsulation*>(
      OPP_Global::findModuleByType(mob, "IPv6Encapsulation"));
    assert(tunMod);

}

MIPv6MStateMobileNode::~MIPv6MStateMobileNode(void)
{
  delete mipv6cdsMN;
  delete mipv6cdsMN;

  delete mob->backVector;
  delete mob->buVector;
  delete mob->lbuVector; 
  delete mob->lbackVector;
  delete mob->bbuVector;
  delete mob->bbackVector;

  mob->backVector = 0;
  mob->buVector = 0;
  mob->lbuVector = 0; 
  mob->lbackVector = 0;
  mob->bbuVector = 0;
  mob->bbackVector = 0;
}

void MIPv6MStateMobileNode::initialize(int stage)
{
  if (stage == 1)
  {
    parseXMLAttributes();
  }
  if (stage == 2 && mob->rt->mobilitySupport() && mob->isMobileNode() &&
      mob->par("homeAgent").stringValue()[0])
  { 
    ipv6_addr haAddr = IPv6_ADDR_UNSPECIFIED;
    IPvXAddress destAddr = IPAddressResolver().resolve(mob->par("homeAgent"));
    if (destAddr.isNull())
      haAddr = c_ipv6_addr(mob->par("homeAgent"));
    else
      haAddr = c_ipv6_addr(destAddr.get6().str().c_str());

    if (haAddr == IPv6_ADDR_UNSPECIFIED)
      return;
    Dout(dc::mipv6, mob->nodeName()<<" using assigned homeAgent "<<haAddr
         <<" from par(homeAgent) "<<mob->par("homeAgent"));
    IPv6NeighbourDiscovery::RouterEntry* re = new IPv6NeighbourDiscovery::RouterEntry(haAddr);
    re->setState(NeighbourEntry::INCOMPLETE);
    mob->rt->cds->insertRouterEntry(re, false);
    boost::weak_ptr<IPv6NeighbourDiscovery::RouterEntry> bre = mob->rt->cds->router(haAddr);
    MIPv6RouterEntry* ha = new MIPv6RouterEntry(bre, true, ipv6_prefix(haAddr, 64), VALID_LIFETIME_INFINITY);

    mipv6cdsMN->insertHomeAgent(ha);

    boost::shared_ptr<MIPv6RouterEntry> bha = mipv6cdsMN->findHomeAgent(haAddr);
    mipv6cdsMN->primaryHA() = bha;
    //Todo assuming HA is on ifIndex of 0
    bool primaryHoa = true;

    ipv6_prefix hoa = mipv6cdsMN->formHomeAddress(
      mipv6cdsMN->primaryHA(), mob->ift->interfaceByPortNo(0), primaryHoa);

    mob->rt->assignAddress(IPv6Address(hoa), 0);

    mipv6cdsMN->setAwayFromHome(true);
    mipv6cdsMN->currentRouter() = boost::shared_ptr<MIPv6RouterEntry>();

    mob->rt->cds->removeDestEntryByNeighbour(haAddr);

    WATCH_PTRLIST(buRetranTmrs);
  }
}

bool MIPv6MStateMobileNode::processMobilityMsg(IPv6Datagram* dgram)
{

  MobilityHeaderBase* mhb = mobilityHeaderExists(dgram);

  if (!mhb)
    return false; //dgram deleted already

  switch ( mhb->kind() )
  {
    case MIPv6MHT_BA:
    {
      BA* ba = check_and_cast<BA*>(mhb);
      processBA(ba, dgram);
    }
    break;

    case MIPv6MHT_BE:
    {
      BE* be = check_and_cast<BE*>(mhb);
      processBE(be, dgram);
    }
    break;

    case MIPv6MHT_BRR:
    {
      BRR* br =  check_and_cast<BRR*> (mhb);
      processBRR(br, dgram);
    }
    break;
    case MIPv6MHT_HOT: case MIPv6MHT_COT:
    {
      //TMsg* t =  check_and_cast<TMsg*> (mhb);
      processTest(mhb, dgram);
    }
    break;
    default:
      return MIPv6MStateCorrespondentNode::processMobilityMsg(dgram);
    break;
  }
  return true;
}

/**
 * 11.7.3
 */

bool MIPv6MStateMobileNode::processBA(BA* ba, IPv6Datagram* dgram)
{
  // check if the length in the option is actually greater or equal to
  // how long it SHOULD be according to the draft standard
  bool cont = false;
  bu_entry* bue = mipv6cdsMN->findBU(dgram->srcAddress());
  if (bue == 0)
  {
    Dout(dc::mipv6|dc::notice|flush_cf, mob->nodeName()<<" BA received from "
         <<dgram->srcAddress()<<" with no corresponding BUL entry");
    return cont;
  }

  if (ba->sequence() != bue->sequence())
  {
    Dout(dc::mipv6|dc::notice|flush_cf, mob->nodeName()<<" BA received from "
         <<dgram->srcAddress()<<" IGNORED. With sequence "<< ba->sequence()
         <<" instead of "<< bue->sequence());
    //11.7.3 
    return cont;
  }

  simtime_t now = mob->simTime();

  //cancel and delete BU retransmission timer (as received)

  //Warning: Assuming that we are sending only binding update for one care of
  //addr not more(when multiple interfaceID's exist), if we do then will need to
  //check exactly for which care of addr we are removing timer for
  bool found = false;
  for (BURTI it = buRetranTmrs.begin(); it != buRetranTmrs.end(); it++)
  {
    Dout(dc::mipv6|flush_cf, **it);
    if ((*it)->dgram->destAddress() == dgram->srcAddress())
    {
      if ((*it)->isScheduled())
        (*it)->cancel();
      else
      {
        Dout(dc::warning|flush_cf,  mob->nodeName()<<" "<<now
             <<" unexpected BU Retrans timer not scheduled "
             <<" is this ba to a BU that we gave up retransmitting? "
             <<dgram);
      }

      if ((*it)->dgram->srcAddress() == mipv6cdsMN->homeAddr() && dgram->srcAddress() == mipv6cdsMN->primaryHA()->addr())
      {
        Dout(dc::mipv6|dc::notice|flush_cf, mob->nodeName()<<" "<<now
             <<" returning home completed setting awayFromHome to false");
        mipv6cdsMN->setAwayFromHome(false);

      }
      Dout(dc::mipv6|flush_cf, mob->nodeName()<<" "<<now
           <<" deleting BURetranTmr as we received Back from "<<dgram->srcAddress());
      removeBURetranTmr(*it);
      found = true;
      break;
    }
  }

  if (!found)
    Dout(dc::warning|dc::mipv6|flush_cf, mob->nodeName()<<" "<<now
         <<" unable to find BURetranTmr for deletion as received Back from "
         <<dgram->srcAddress());

  if (BAS_ACCEPTED != ba->status())
  {
    //don't delete bule in this case as could be an old bu coming in
    if (ba->status() == BAS_SEQ_OUT_OF_WINDOW)
      return cont;

    //Remove entry from BUL if BU failed.
    mipv6cdsMN->removeBU(bue->addr());
    Dout(dc::mipv6|dc::notice|flush_cf, mob->nodeName()<<" BU rejected by "
         <<dgram->srcAddress()<<" status="<<ba->status());

    //Don't know how to do optional recover from these errors for now sending
    //new BU.
    return cont;
  }

  Dout(dc::mipv6| flush_cf, mob->nodeName()<<" "<<now<<" BA received from "
       <<dgram->srcAddress()<<" seq="<<ba->sequence());

  bue->state = 0;

  recordBAVector(dgram, mob, mipv6cds);

  if ( bue->homeReg() )
  {
    mob->prevLinkUpTime = now;

    if ( mob->isEwuOutVectorHODelays() )
    {
      assert( dgram->timestamp() );
      bue->regDelay->record(now - dgram->timestamp() );
    }
  }

  if (ba->lifetime() < bue->lifetime())
  {
    bue->setExpires(max<int>(bue->expires()-(bue->lifetime()-ba->lifetime()), 0));
  }
  cont = true;
  return cont;
}

void MIPv6MStateMobileNode::recordHODelay(const simtime_t buRecvTime, ipv6_addr addr)
{
  if ( !mob->isEwuOutVectorHODelays() )
    return;

  if ( mob->signalingEnhance() != None )
  {
    // just make sure we always use hoa of the peer if the peer is also a MN
    boost::weak_ptr<bc_entry> bce =
      mipv6cds->findBindingByCoA(addr);
    if(bce.lock())
      addr = bce.lock()->home_addr;
  }

  bu_entry* bue = mipv6cdsMN->findBU(addr);
  assert(bue);

  bue->regDelay->record(buRecvTime);
}

/**
   @brief Sends a BU to every node that is in BUL as long as they do not have
   knowledge of the coa except the MAP. pcoa forwarding occurs here also if ARs
   are HAs too.

   HMIPv6 previous MAP forwarding is done in HMIPv6NDState::processRtrAd -
   mapHandover.  pcoa forwarding for ars is done in HMIPv6NDState::arHandover.
 */
void MIPv6MStateMobileNode::sendBUToAll(const ipv6_addr& coa, const ipv6_addr hoa,
                                        size_t lifetime)
{

  //for every bu_entry send it the new details of the new BU and update the
  //binding entry in the process

#ifdef USE_HMIP
  if (mob->hmipSupport())
  {
    assert(hmipv6cds);
  }
#endif //USE_HMIP

  if (
    //First registration is always to primary HA
    mipv6cdsMN->bulEmpty()
#ifdef USE_HMIP
    //point of hmip is to bind with MAP first to eliminate extra binding.
    //HMIP can send BU to MAP before BU to HA
    || (mob->hmipSupport() &&
        !mipv6cdsMN->findBU(mipv6cdsMN->primaryHA()->addr()) &&
        hmipv6cds->isMAPValid())
#endif //USE_HMIP
    //ebu causes bul to contain cn entries while at home for home test purposes
    || (mob->earlyBindingUpdate() && !mipv6cdsMN->findBU(mipv6cdsMN->primaryHA()->addr()))
    )
  {
    assert(mipv6cdsMN->homeAddr() != IPv6_ADDR_UNSPECIFIED);
    ipv6_addr oldcoa = mipv6cdsMN->careOfAddr();

    //Send BU to pHA
    //Create pHA bul entry and update it
    sendBU(mipv6cdsMN->primaryHA()->addr(), coa, hoa,
           lifetime, true,
           mipv6cdsMN->primaryHA()->re.lock()->ifIndex());

    //send to current cns too (3.5 proactive registrations)
    if (mob->earlyBindingUpdate())
      for_each(mipv6cdsMN->bul.begin(), mipv6cdsMN->bul.end(),
               sendBUs(coa, oldcoa, hoa, lifetime, mob, mipv6cdsMN));
  }
  else
  {

    ipv6_addr oldcoa = mipv6cdsMN->careOfAddr();

    //

    //There's no need to resend BU containing exactly the same information
    //This condition is possible when router does not send advertisements
    //within the advertised interval and so we think we've moved to new
    //subnet. After receiving the much delayed router advertisement from
    //same router we should just reassign that routers status and not
    //handover

    Dout(dc::mipv6|dc::debug|flush_cf, " in send bu to all hoa "<<hoa<<" coa "
         <<coa<<" oldcoa "<<oldcoa<<" lifetime="<<lifetime);
    if (oldcoa != coa)
    {

      //Iterate through every BU_entry and send updates to nodes with same hoa
      //(currently it just skips map indiscriminately)
      for_each(mipv6cdsMN->bul.begin(), mipv6cdsMN->bul.end(),
               sendBUs(coa, oldcoa, hoa, lifetime, mob, mipv6cdsMN));
    }

  }
}

void MIPv6MStateMobileNode::processTest(MobilityHeaderBase* testMsg, IPv6Datagram* dgram)
{

  assert(testMsg->kind() == MIPv6MHT_HOT || testMsg->kind() == MIPv6MHT_COT);
  Dout(dc::rrprocedure|flush_cf, "RR Procedure At " << mob->simTime() << " sec, " 
       << mob->nodeName()<<" receives " << testMsg->className() << " src=" 
       << dgram->srcAddress() << " dest= " << dgram->destAddress());

  bu_entry* bule = 0;
  ipv6_addr srcAddr = dgram->srcAddress();

  bool isDirectRoute = false;
  boost::weak_ptr<bc_entry> bce;
  if ( mob->signalingEnhance() != None  )
  {
    bce = mipv6cds->findBindingByCoA(srcAddr);
    if(bce.lock())
    {
      isDirectRoute = true;
      srcAddr = bce.lock()->home_addr;
    }
  }

  // check if the source address of the packet belongs to a CN for
  // which the MN has a BUL entry.
  if ((bule = mipv6cdsMN->findBU(srcAddr)) == 0)
  {
    Dout(dc::warning|dc::rrprocedure|flush_cf, "RR procedure ERROR: "
         << mob->nodeName()<<" src="<<srcAddr
         <<" in "<< testMsg->className() << " does not appear in the binding update list");
    return;
  }

  if ( testMsg->kind() == MIPv6MHT_COT && mipv6cdsMN->careOfAddr() != dgram->destAddress())
  {
    Dout(dc::warning|dc::rrprocedure|flush_cf, "RR procedure ERROR: "
         << mob->nodeName()<<" dest="<<dgram->destAddress()
         <<"in "<< testMsg->className() << " does not match coa ="
         <<mipv6cdsMN->careOfAddr());
    return;
  }

  if ( testMsg->kind() == MIPv6MHT_HOT && mipv6cdsMN->homeAddr() != dgram->destAddress())
  {
    Dout(dc::warning|dc::rrprocedure|flush_cf, "RR procedure ERROR: "
         << mob->nodeName()<<" dest="<<dgram->destAddress()
         <<"in "<< testMsg->className() << " does not match home address ="
         <<mipv6cdsMN->homeAddr());
    return;
  }

  HOT* hot = 0;
  COT* cot = 0;
  
  if (testMsg->kind() == MIPv6MHT_HOT)
    hot = check_and_cast<HOT*>(testMsg);
  else
    cot = check_and_cast<COT*>(testMsg);

  assert(hot || cot);

  // check if the core-of init cookie filed in the message matches the
  // value stored in the BUL
  if ((hot && hot->homeCookie() != bule->homeCookie()) ||
       (cot && cot->careOfCookie() != bule->careOfCookie()))
  {
    Dout(dc::warning|dc::rrprocedure|flush_cf, "RR procedure ERROR: " << mob->nodeName()
	 <<" the init cookie filed in the " << testMsg->className() 
	 << " does not match entry in BUL");
    return;
  }

  bule->resetTestInitTimeout((MIPv6HeaderType)testMsg->kind());
  /*
  Dout(dc::rrprocedure|flush_cf, "RR Procedure At " << mob->simTime() << " sec, " 
       << mob->nodeName()<<" has verified that " << testMsg->className() 
       << " src=" << srcAddr << " dest= " << dgram->destAddress() << " is valid");
  */
  if ( testMsg->kind() == MIPv6MHT_HOT && mob->earlyBindingUpdate())
  {
    //10 is just arbitrary number so that it refreshes before the home token expires
    bule->hotiRetransTmr->reschedule(mob->simTime() + MAX_TOKEN_LIFETIME - 10);
    
    Dout(dc::rrprocedure|flush_cf, " RR procedure:(EARLY BU)  At " <<  mob->simTime()
	 << " sec, " << mob->nodeName() << " next home test will be at " 
	 << bule->hotiRetransTmr->arrivalTime()<<" sec");    
  }  

  if (hot)
    bule->homeNI = hot->hni();
  else
    bule->careOfNI = cot->coni();

  bool returnHome = bule->homeAddr() == mipv6cdsMN->careOfAddr();

  if (bule->testSuccess() || (mob->earlyBindingUpdate() && 
			      ((cot && bule->homeNI != 0) || 
			       (hot && bule->careOfNI != 0))
			      ) ||
      //for pro active home test bule willl look like a returning home bulue
      //except that bu to cn was never sent (i.e. last_time_sent == 0) so we only
      //want to deregister when it is not a pro active home test entry
      (returnHome && bule->homeNI != 0 && bule->last_time_sent != 0))
  {
    assert(dgram->timestamp());
    size_t lifetime = returnHome? 0:mob->rt->minValidLifetime();
    // send BU
    sendBU(dgram->srcAddress(), mipv6cdsMN->careOfAddr(),
           mipv6cdsMN->homeAddr(), lifetime,
           false, dgram->inputPort(), false, dgram->timestamp());
    Dout(dc::rrprocedure|flush_cf, "RR Procedure At " << mob->simTime()<< " sec, "
         << mob->rt->nodeName()
         <<" Correspondent Registration: sending BU to CN (Route Optimisation) dest= "
         << dgram->srcAddress());
  }
  /* not really part of ebu spec because they say only do ebu when sending a
     coti but nevertheless another valid case for sending ebu. But how to tell
     whether ebu sent already during this handover? (becomes many little tweaks
     to improve throughput)

  else if (hot && bule->careOfNI == 0 &&
	   mipv6cdsMN->homeAddr() != mipv6cdsMN->careOfAddr())
  {
    //early bu (useful for cases where we are already away from home and then we
    //start sending stuff to cn) however coti is already under way here (as soon
    //as encapsulated packet from cn) so advantage slim
    sendBU(dgram->srcAddress(), mipv6cdsMN->careOfAddr(),
           mipv6cdsMN->homeAddr(), mob->rt->minValidLifetime(),
           false, dgram->inputPort(), false, dgram->timestamp());    
  }
  */
}

void MIPv6MStateMobileNode::sendInits(const ipv6_addr& dest,
                                      const ipv6_addr& coa)
{
  OPP_Global::ContextSwitcher switchContext(mob);

  std::vector<ipv6_addr> addrs(2);
  addrs[0] = dest;
  addrs[1] = coa;

  //if coa is unspecified then means proactive hoti
  //so only set up hoti things. and allow it to repeat according to ebu.
  //send will call this fn with the cn addr in dest and coa unspecified
  bu_entry* bule = mipv6cdsMN->findBU(dest);

  bool isNewBU = false;
  if(!bule)
  {
    isNewBU = true;
    bool homereg = false;
    bule = new bu_entry(dest, mipv6cdsMN->homeAddr(), coa, mob->rt->minValidLifetime(), 0, 0, homereg);

    if ( mob->isEwuOutVectorHODelays() )
    	bule->regDelay = new cOutVector("CN reg (including RR)");

    mipv6cdsMN->addBU(bule);

    bule->hotiRetransTmr = new TIRetransTmr("Sched_SendHoTI", Sched_SendHoTI);

    bule->cotiRetransTmr = new TIRetransTmr("Sched_SendCoTI", Sched_SendCoTI);
  }
  else
  {
    if (bule->isPerformingRR() || mob->simTime() < bule->last_time_sent + (simtime_t) 1/3) //MAX_UPDATE_RATE  // for some reason, MAX_UPDATE_RATE keeps returning zero.. I can't be stuffed fixing it.. just leave it for now
      return;
  }

  simtime_t testInitScheduleTime = mob->simTime() + SELF_SCHEDULE_DELAY;

  bool returnHome = bule->homeAddr() == coa;

  *(bule->cotiRetransTmr) = boost::bind(&MobileIPv6::MIPv6MStateMobileNode::sendCoTI, this,
					addrs, mob->simTime());

  if ( !mob->earlyBindingUpdate() && bule->hotiRetransTmr->isScheduled() )
  {
    Dout(dc::rrprocedure|flush_cf, "ERROR: At " <<  mob->simTime()<< " sec, " 
	 << mob->nodeName() << " moves to a new foreign network but still retransmits the HoTI for the previous CoA. The timer message is canceled");
    bule->hotiRetransTmr->cancel();
  }

  if ( bule->cotiRetransTmr->isScheduled() )
  {
    Dout(dc::rrprocedure|flush_cf, "ERROR: At " <<  mob->simTime()<< " sec, " 
	 << mob->nodeName() << " moves to a new foreign network but still retransmits the CoTI for the previous CoA. The timer message is canceled");
    bule->cotiRetransTmr->cancel();
  }

  if (!mob->earlyBindingUpdate() || (mob->earlyBindingUpdate() && 
				     (isNewBU || !bule->homeNI) && 
				     !bule->hotiRetransTmr->isScheduled()))
  {
    *(bule->hotiRetransTmr) = boost::bind(&MobileIPv6::MIPv6MStateMobileNode::sendHoTI, this,
					  addrs, mob->simTime());
    bule->hotiRetransTmr->reschedule(testInitScheduleTime);
  }

  if (!returnHome)
    bule->cotiRetransTmr->reschedule(testInitScheduleTime);

  if (returnHome && bule->homeNI != 0)
    sendBU(dest, coa,
           mipv6cdsMN->homeAddr(), 0, //allow deletion of cns even under
				      //proactive home test, if comms still
				      //active we can redo home test (TODO what
				      //method for determining when to stop
				      //proactive home test?)
           false, 0, false, mob->simTime());

}

void MIPv6MStateMobileNode::sendHoTI(const std::vector<ipv6_addr> addrs, simtime_t timestamp)
{
  ipv6_addr dest = addrs[0];
  const ipv6_addr& coa = addrs[1];
  //don't really need coa (as it's sometimes wrong due to reusing previous timer
  //message?)  mip6 spec just says it has to be reverse tunnelled through ha and
  //no requirement that it is same as coa in care of test

  bu_entry* bule = mipv6cdsMN->findBU(dest);

  assert(bule);

  bule->increaseHotiTimeout();

  cModule* outputMod = OPP_Global::findModuleByType(mob, "IPv6Output");
  assert(outputMod);

  HOTI* hoti = new HOTI;
  bule->setHomeCookie(hoti->homeCookie());
  IPv6Datagram* dgram_hoti = 
    constructDatagram(mipv6cdsMN->homeAddr(), dest, hoti, 0, timestamp);

  Dout(dc::rrprocedure,"HOTI: At " << mob->simTime()<< " sec, "<< mob->nodeName() 
       << " sending HoTI src= " << dgram_hoti->srcAddress() << " to " 
       << dgram_hoti->destAddress()<<"cb coa="<<coa<< " cdsMN->coa="<<mipv6cdsMN->careOfAddr());
  
  if (mipv6cdsMN->careOfAddr() != mipv6cdsMN->homeAddr())
  {
    
    size_t vIfIndex = tunMod->findTunnel(mipv6cdsMN->careOfAddr(),
                                         mipv6cdsMN->primaryHA()->prefix().prefix);
//    if(!vIfIndex)
//      vIfIndex = tunMod->createTunnel(coa, mipv6cdsMN->primaryHA()->prefix().prefix, 0);
    assert(vIfIndex);

    dgram_hoti->setOutputPort(vIfIndex);
    mob->sendDirect(dgram_hoti, 0, tunMod, "mobilityIn");
  }
  else
  {
    //returning home case
    mob->sendDirect(dgram_hoti, 0, outputMod, "mobilityIn");
  }

  bule->hotiRetransTmr->rescheduleDelay(bule->homeInitTimeout());

  Dout(dc::rrprocedure|flush_cf," next HoTI retransmission time will be at " 
       <<   bule->hotiRetransTmr->arrivalTime());
}

void MIPv6MStateMobileNode::sendCoTI(const std::vector<ipv6_addr> addrs, simtime_t timestamp)
{
  ipv6_addr dest = addrs[0];
  const ipv6_addr& coa = addrs[1];

  bu_entry* bule = mipv6cdsMN->findBU(dest);

  assert(bule);

  bule->increaseCotiTimeout();

  cModule* outputMod = OPP_Global::findModuleByType(mob, "IPv6Output");
  assert(outputMod);

  COTI* coti = new COTI;
  bule->setCareofCookie(coti->careOfCookie());

  // Once the tunnel is established, the packet destinated for the
  // particular tunnel will be sent in tunnel. We want the CoTI to be
  // sent directly to the correspondent node. Therefore it is sent
  // directly to the output core via "mobilityIn")

  IPv6Datagram* dgram_coti = constructDatagram(coa, dest, coti, 0, timestamp);

  mob->sendDirect(dgram_coti, 0, outputMod, "mobilityIn");

  bule->cotiRetransTmr->reschedule(mob->simTime() + bule->careOfInitTimeout());

  Dout(dc::rrprocedure|flush_cf,"COTI: At " << mob->simTime()<< " sec, "<< mob->nodeName() 
       << " sending CoTI src= " << dgram_coti->srcAddress() << " to " 
       << dgram_coti->destAddress()<< "| next CoTI retransmission time will be at " 
       <<   bule->cotiRetransTmr->arrivalTime());
  cerr<<"COTI: At " << mob->simTime()<< " sec, "<< mob->nodeName() 
       << " sending CoTI src= " << dgram_coti->srcAddress() << " to " 
       << dgram_coti->destAddress()<< "| next CoTI retransmission time will be at " 
      <<   bule->cotiRetransTmr->arrivalTime()<<endl;

  //looks like acc. to fig. 1 of ebu draft that proactive home reg needs to have
  //taken place for ebu to be effective. i.e. no point sending ebu when HOT
  //not received at handover time.
  if (mob->earlyBindingUpdate() && bule->homeNI != 0)
  {
    // send early BU
    sendBU(dest, coa,
           mipv6cdsMN->homeAddr(), mob->rt->minValidLifetime(),
           false, 0, false, mob->simTime());
    Dout(dc::rrprocedure|flush_cf, "RR Procedure (Early BU) At " << mob->simTime()<< " sec, " << mob->rt->nodeName()
         <<" Correspondent Registration: sending BU to CN dest= "
         << IPv6Address(dest));
  }
}

/**
 * Binding Error in RFC 3775
 *
 */

void MIPv6MStateMobileNode::processBE(BE* bm, IPv6Datagram* dgram)
{
  bu_entry* bule = 0;

  if ((bule = mipv6cdsMN->findBU(dgram->srcAddress())) != 0)
  {
/*
 o  If the mobile node has recent upper layer progress information,
      which indicates that communications with the correspondent node
      are progressing, it MAY ignore the message.  This can be done in
      order to limit the damage that spoofed Binding Error messages can
      cause to ongoing communications.

   o  If the mobile node has no upper layer progress information, it
      MUST remove the entry and route further communications through the
      home agent.  It MAY also optionally start a return routability
      procedure (see Section 5.2).

*/
    //Maybe we should assume first option if we can tell that it is rapid stuff
    //from other side i.e. rtp and we are receiving it we can choose to wait for
    //ack from it.or redo bu/rr

    Dout(dc::notice, mob->nodeName()<<" "<<mob->simTime()
	 <<" removing bu list entry in response to BE from "
	 <<dgram->srcAddress());
    //doing second option (since first is difficult)
    mipv6cdsMN->removeBU(dgram->srcAddress());
  }
  else
    Dout(dc::warning, mob->nodeName()<<" "<<mob->simTime()
         <<" Unable to process BE from "<<dgram->srcAddress());
  // if the mobile node does not have a binding update list entry for
  // the source of the binding missing message, it MUST ignore the
  // message

}

/**
 * @brief Respond to known CN i.e. a node in BUL
 *
 */

void  MIPv6MStateMobileNode::processBRR(BRR* br, IPv6Datagram* dgram)
{
  //IPv6Datagram* dgram = check_and_cast<IPv6Datagram*> (br->encapsulatedMsg());
  size_t ifIndex = dgram->inputPort();
  bu_entry* bule = 0;

  //Check that CN is in BUL then send BU and update BUL details
  if ((bule = mipv6cdsMN->findBU(dgram->srcAddress())) != 0)
  {

    //if unique identified exists in br then send it back in the BU too

    //Latest lifetime
    unsigned int lifetime = 30000;

    //Home Agents would never send BR to us as the onus is on the MN to update
    //its own home address lifetime.
    bool homeReg = false;
    assert(!mipv6cdsMN->findHomeAgent(dgram->srcAddress()));

    size_t ifIndex = dgram->inputPort();
    sendBU(dgram->srcAddress(), mipv6cdsMN->primaryHA()->prefix().prefix,
           dgram->destAddress(), lifetime, homeReg, ifIndex);
  }
  else
  {
    cerr<<mob->nodeName()<<":"<<ifIndex<<" Possible DOS attack from "
        <<dgram->srcAddress()<<" directed at home address "<<dgram->destAddress()
        <<"\n";
  }


  delete br;
}

bool MIPv6MStateMobileNode::removeBURetranTmr(BURetranTmr* buTmr, bool all)
{
  if (!all)
  {
    assert(buTmr);
    buRetranTmrs.remove(buTmr);
    delete buTmr;
    return true;
  }
  assert (!buTmr && all);
  if (buRetranTmrs.empty())
    return false;
  for (BURTI buit = buRetranTmrs.begin(); 
       buit != buRetranTmrs.end(); /*no code here*/)
  {
    if ((*buit)->isScheduled())
    {
      bu_entry* bule = 
	mipv6cdsMN->findBU((*buit)->dgram->destAddress());
      bule->state = 0;
      (*buit)->cancel();
    }
    delete *buit;
    buit = buRetranTmrs.erase(buit);
  }
  return true;
}

#ifdef USE_HMIP
bool MIPv6MStateMobileNode::sendMapBU(const ipv6_addr& dest, const ipv6_addr& coa,
                                      const ipv6_addr& hoa, size_t lifetime,
                                      size_t ifIndex)
{
  return sendBU(dest, coa, hoa, lifetime, false, ifIndex, true);
}
#endif //USE_HMIP

/**
 * @brief send a binding update and update the bul entry in the process
 *
 * Sec 10.7 Only homeReg have binding acks turned on.  For CN it's always off.
 * @param dest address of destination to send BU
 * @param coa care of address in BU
 * @param hoa home address in BU

 * @param lifetime is the valid time of binding before removed from both MN and
 * CN/HA BU cache.

 *
 * @param homeReg specify whether this is a home registration to primary HA or
 * previous subnet's HA
 *
 * @param ifIndex is the interface which was connected to "home subnet" for
 * remote startups this may be difficult to determine.  It is used for
 * retransmission of the BU in case we want a BA from HA.
 *
 * @param mob the ever present IPv6Mobility Ned module
 *
 * @param mapReg sets the corresponding MAP registration flag
 */

bool MIPv6MStateMobileNode::sendBU(const ipv6_addr& dest, const ipv6_addr& coa,
                                   const ipv6_addr& hoa, size_t lifetime,
                                   bool homeReg, size_t ifIndex
#ifdef USE_HMIP
                                   , bool mapReg
#endif //USE_HMIP
                                   , simtime_t timestamp
)
{
  //TODO 11.7.2 destAddress is either src addr of inner packet or hoa dest option of
  //inner packet that triggered this route optimisation to CN (prob. for
  //simultaneous mns)

  //Can be called from NDStateHost(IPv6NeighbourDiscovery) or
  //itself(IPv6Mobility)
  // 2 lines equiv to Enter_Method except that uses this pointer implicitly
  OPP_Global::ContextSwitcher switchContext(mob);
  //switchContext.methodCall("sendBU(%s)", ipv6_addr_toString(dest).c_str());

  bool ack = homeReg;  

  bu_entry* bule = 0;

  if (mob->signalingEnhance() != None )
  {
    boost::weak_ptr<bc_entry> bce =
      mipv6cds->findBindingByCoA(dest);
    if (bce.lock())
      bule = mipv6cdsMN->findBU(bce.lock()->home_addr);
  }

  if (!bule)
    bule = mipv6cdsMN->findBU(dest);

  if (mob->returnRoutability() && !homeReg && !mapReg)
    assert(bule);

#ifdef USE_HMIP

  if (mapReg)
    ack = mapReg;

  if (!ack && mipv6cdsMN->sendBUAckFlag())
    ack = true;

#endif //USE_HMIP

  unsigned int seq = 0;

  if (bule)
  {
    seq = bule->sequence();
    seq++;

    if (bule->problem)
    {
      Dout(dc::warning|flush_cf, mob->nodeName()<<(homeReg?" HA ":" CN ")<<dest
           <<" bule has problem flag set continuing anyway!");
      //return false;
    }

    if ( bule->careOfAddr() == coa )
    {
      simtime_t updateInterval = mob->simTime() - bule->last_time_sent;
      //Not 100% correct because we should allow 3 very fast and consecutive BU
      //in a second instead of average separation of 1/3 seconds (see Sec. 11.8)
      if (updateInterval < MAX_UPDATE_RATE)
      {
        Dout(dc::mipv6|dc::notice|flush_cf, mob->nodeName()
             <<" BU to "<<bule->addr()<<" Rate limited as only "<<updateInterval
             <<" seconds have passed and min BU interval is "<<MAX_UPDATE_RATE);
        return false;
      }
    }

  }

  BU* bu = new BU(ack, homeReg,
#ifdef USE_HMIP
                  mapReg,
#endif //USE_HMIP
  seq, lifetime); //hoa

  bool returnHome = coa == hoa;
  bool ebu = false;
  if (!homeReg && !mapReg )
  {

    if (!mob->earlyBindingUpdate())
    {
    //rr should already have been done
      assert(bule->homeNI);
      if (!returnHome)
      {
	assert(bule->careOfNI);
	assert(bule->testSuccess());
      }	 
      else
	assert(bule->homeNI != 0);
      assert(!bule->isPerformingRR());
    }
    else
    {
      assert(bule->homeNI); 
    }

    //11.7.2 , 6.1.7 & 5.2.6 for CN
    MIPv6OptNI* ni = new MIPv6OptNI;
    ni->setHni(bule->homeNI);
    if (!mob->earlyBindingUpdate() || 
	(mob->earlyBindingUpdate() && !bule->tentativeBinding()))
      ni->setConi(bule->careOfNI);
    else 
    {
      ni->setConi(0);
      ebu = true;
      lifetime = TENTATIVE_BINDING_LIFETIME;
      bu->setExpires(TENTATIVE_BINDING_LIFETIME);
    }

    //no need to reset as we promise to continually update it for ebu
    if (!mob->earlyBindingUpdate())
      bule->homeNI = 0;

    bule->careOfNI = 0;
    bu->addOption(ni);
    //add Kbm
    bu->addOption(new MIPv6OptBAD);   
  }

  if (ebu && !returnHome)
    bu->setName("EarlyBU");
  
  IPv6Datagram* dgram = constructDatagram(coa, dest, bu, 0);
  if (homeReg || mapReg)
  {
    // BU sent to HA should not have any timestamp set
    assert( !timestamp );
    dgram->setTimestamp( mob->simTime() );
  }
  else
  {
    // BU sent to CN should already have timestamp from previous state
    assert( timestamp );
    dgram->setTimestamp( timestamp );
  }

  //Draft 17 6.1.7 BU must have haddr dest opt
  HdrExtDestProc* destProc = dgram->acquireDestInterface();

  destProc->addOption(new MIPv6TLVOptHomeAddress(hoa));

  mob->send(dgram, "routingOut");

  //  if (mapReg || homeReg || ((mob->earlyBindingUpdate() && ((!mapReg && !homeReg) && ebu))) ||
  //  !mob->earlyBindingUpdate())
  recordBUVector(dgram, mob, mipv6cds);

  //Create BU retransmission timer
  if (ack)
  {
    double timeout = INITIAL_BINDACK_TIMEOUT_FIRSTREG + SELF_SCHEDULE_DELAY;

    if (bule != 0)
      timeout = (double)INITIAL_BINDACK_TIMEOUT + SELF_SCHEDULE_DELAY;

    BURetranTmr* buTmr = 0;
    if (!bule || (bule && bule->state == 0))
    {
      assert(timeout > 0);
      assert(static_cast<int> (timeout) > 0);
      //Should we preserve the value instead of truncating timeout?
      buTmr = new BURetranTmr(static_cast<int>(timeout), dgram->dup(), this, mob, mipv6cds);
      buRetranTmrs.push_back(buTmr);
    }
    else
    {
      if (bule->problem)
        Dout(dc::mipv6, mob->nodeName()<<" bule "<<dgram->destAddress()<<" has problem flag set");
      bool found = false;
      Dout(dc::mipv6|dc::warning|flush_cf, mob->nodeName()<<" "<<bule->state
           <<" outstanding BU transmission already to "<<dgram->destAddress());
      for (BURTI it = buRetranTmrs.begin(); it != buRetranTmrs.end(); it++)
      {
        if ((*it)->dgram->destAddress() == dgram->destAddress())
        {
          buTmr = *it;
          buTmr->cancel();
          delete buTmr->dgram;
          buTmr->dgram = dgram->dup();
          buTmr->timeout = static_cast<int> (timeout);
          found = true;
          Dout(dc::mipv6, mob->nodeName()<<" reusing BU retransmission timer for dest="<<dest);
          break;
        }
      }
      //assert(found);
      if (!found)
      {
        Dout(dc::warning, mob->nodeName()<<" "<<mob->simTime()<<" unable to find BU retrans timer for BU "<<*bu
						<<" dgram"<<*dgram);
      }
    }

    if (buTmr)
    {
      mob->scheduleAt(mob->simTime()+timeout, buTmr);
      Dout(dc::mipv6, mob->nodeName()<<" Waiting for BU ack timeout="
           <<setprecision(4)<<timeout);
    }
  } //if ack

#ifdef USE_HMIP
  if (mapReg)
    Dout(dc::hmip, mob->nodeName()<<" "<<setprecision(4)<<mob->simTime() + SELF_SCHEDULE_DELAY
       <<" BU to "<<dgram->destAddress()<<" hoa="<<hoa<<" coa="<<coa
       <<" seq="<<seq<<(homeReg?" home":
                        mapReg?" map":
                        " CN")<<" reg"<<(ack?" ack":"")
       <<" lifetime="<<dec<<lifetime);
  else
#endif //USE_HMIP
  Dout(dc::mipv6, mob->nodeName()<<" "<<setprecision(4)<<mob->simTime() + SELF_SCHEDULE_DELAY
       <<" BU to "<<dgram->destAddress()<<" hoa="<<hoa<<" coa="<<coa
       <<" seq="<<seq<<(homeReg?" home":
                        " CN")<<" reg"<<(ack?" ack":"")
       <<" lifetime="<<dec<<lifetime);

  //Update or create bule and add to BUL and start/reschedule lifetime of entry
  if (!bule)
  {
    bule = new bu_entry(dest, hoa, coa, lifetime, seq,
                        mob->simTime() + SELF_SCHEDULE_DELAY, homeReg
#ifdef USE_HMIP
                        , mapReg
#endif //USE_HMIP
                        );
    //Increment every time we send.  Only when we get back BA do we decrement
    if (ack)
      bule->state++;
    mipv6cdsMN->addBU(bule);

    if ( homeReg && mob->isEwuOutVectorHODelays() )
    {
      //TODO bule gets recreated when return home and go away again so
      //previous vector wiped out
      bule->regDelay = new cOutVector("home reg");
    }
    else if (mob->isEwuOutVectorHODelays())
    {
      ostringstream os;
      os<<dest<<(mapReg?" MAP ": " CN ")<<" reg (without RR)";
      bule->regDelay = new cOutVector(os.str().c_str());
    }
  }
  else
  {
    if (ack)
      bule->state++;
    bule->setLifetime(lifetime);
    bule->setSequence(seq);
    bule->setCareOfAddr(coa);
    bule->last_time_sent = mob->simTime() + SELF_SCHEDULE_DELAY;
  }

  return updateTunnelsFrom(dgram->destAddress(), coa, ifIndex, homeReg, mapReg);

}

bool MIPv6MStateMobileNode::updateTunnelsFrom
(ipv6_addr budest, ipv6_addr coa, unsigned int ifIndex,
 bool homeReg, bool mapReg)
{
  //if coa diff from bule->coa (change it)!!
  if (homeReg || mapReg)
  {
    //delete reverse tunnels with exit point of ha/map
    size_t delIndex = tunMod->findTunnel(IPv6_ADDR_UNSPECIFIED,
					 budest);
    size_t vIfIndex = tunMod->findTunnel(coa, budest);

    if (vIfIndex != delIndex)
    {
      mob->bubble("Destroy tunnel delIndex");
      tunMod->destroyTunnel(delIndex);
    }

    if (!mipv6cdsMN->awayFromHome())
      return true;

  ///Returns the vIfIndex of tunnel if found
//  size_t findTunnelWithExitPoint(const ipv6_addr& exit);

    //Create reverse tunnel to HA/Map
    if (!vIfIndex)
    {
      vIfIndex = tunMod->createTunnel(coa, budest, ifIndex);
      Dout(dc::hmip|dc::encapsulation|dc::debug|flush_cf, mob->nodeName()
	   <<" reverse tunnel created entry coa="<<coa<<" exit "
	   <<(homeReg?"ha=":"map=")
	   <<budest<<" vIfIndex="<<hex<<vIfIndex<<dec);
    }
    else
    {
      Dout(dc::warning|flush_cf, mob->nodeName()
	   <<" reverse tunnel exists already coa="<<coa<<" exit "
	   <<(homeReg?"ha=":"map=")
	   <<budest<<" vIfIndex="<<hex<<vIfIndex<<dec);
    }
  }

  return true;
}

//Process Datagram according to MIPv6 Sec. 11.3.1 while away from home
bool MIPv6MStateMobileNode::mnSendPacketCheck(IPv6Datagram& dgram, bool& tunnel)
{
  tunnel = false;
  //see if binding exists at cn (BU to the peer i.e. bule exists) and if so use
  //a hoa dest opt and send packet via route optimised path. Otherwise send via
  //reverse tunnel to HA

  RoutingTable6* rt = mob->rt;
  IPv6Datagram* datagram = &dgram;

  assert(rt->isMobileNode());

  if (mipv6cdsMN->primaryHA().get() != 0 &&
      mob->earlyBindingUpdate() && !mipv6cdsMN->awayFromHome() && 
      datagram->srcAddress() == mipv6cdsMN->homeAddr())
  {
    //proactive home test
    MobileIPv6::bu_entry* bule = 0;
    bule = mipv6cdsMN->findBU(datagram->destAddress());
    if (!bule)
    {
      Dout(dc::rrprocedure|flush_cf, mob->simTime()<< " "<< mob->rt->nodeName()
	   <<"proactive home test");
      sendInits(datagram->destAddress(), mipv6cdsMN->homeAddr());
      return true;
    }
  }

  if (!mipv6cdsMN->awayFromHome() ||
      mipv6cdsMN->primaryHA().get() == 0)
    return true;
  if (datagram->srcAddress() != mipv6cdsMN->homeAddr())
    return true;

/*
    // In MN-MN communications, there are times when the MN has the
    // care-of address of the CN in its binding cache entry. Since the
    // dest address of the packet got swapped to the care-of address
    // from the home address of the CN, it is important that we use
    // home address as a key to obtain the binding update

    MobileIPv6::bu_entry* bule = 0;
    using MobileIPv6::bc_entry;
    boost::weak_ptr<bc_entry> bce;
    bce = rt->mipv6cds->findBinding(datagram->destAddress());
    if (bce.lock().get() != 0)
      bule = mipv6cdsMN->findBU(bce.lock()->home_addr);
    else
      bule = mipv6cdsMN->findBU(datagram->destAddress());

*/

  //above no longer applies because we do not swap dest address until after this
  //fn see IPv6Send::endService
  MobileIPv6::bu_entry* bule = 0;
  bule = mipv6cdsMN->findBU(datagram->destAddress());

  if (bule)
    Dout(dc::debug|flush_cf, "bule "<<*bule);

  bool pcoa = false;

// {{{ Do Route Optimisation (RO) if bule contains this mn's coa

  // The following section of the code only applies to data packet
  // sent from upper layer. Mobility messages do not contain the
  // home address option. 
  MobilityHeaderBase* ms = 0;
  if (datagram->transportProtocol() == IP_PROT_IPv6_MOBILITY)
    ms = check_and_cast<MobilityHeaderBase*>(datagram->encapsulatedMsg());
  if (ms == 0 && bule && (!bule->isPerformingRR() || 
			  (mob->earlyBindingUpdate() && bule->isPerformingRR())) &&
      bule->homeAddr() == datagram->srcAddress() &&
      //too strict a test for one of the current coa should instead check that
      //it is assigned somewhere?
      mipv6cdsMN->careOfAddr(pcoa) == bule->careOfAddr() &&      
      //state 0 means ba received or assumed to be received > 0 means
      //outstanding BUs
      (!bule->problem && bule->state == 0) && bule->expires() > 0)
  {
    //Do route optimisation when BU to cn exists already
    HdrExtDestProc* destProc = datagram->acquireDestInterface();
    //Should only ever have one home addr destOpt.
    assert(destProc->getOption(IPv6TLVOptionBase::MIPv6_HOME_ADDRESS_OPT)
	   == 0);

    bool test = destProc->addOption(
      new MobileIPv6::MIPv6TLVOptHomeAddress(datagram->srcAddress()));
    assert(test);
    datagram->setSrcAddress(bule->careOfAddr());
    Dout(dc::mipv6, rt->nodeName()<<" Added homeAddress Option "
	 <<bule->homeAddr()<<" src addr="<<bule->careOfAddr()
	 <<" for destination "<<datagram->destAddress());
    //test for addr is assigned on link to send is done in IPv6Send::endService
    return true;
  }
// }}}
// {{{ reverse tunnel (mipv6)

  else
  {
    //Reverse Tunnel
    size_t vIfIndex = tunMod->findTunnel(mipv6cdsMN->careOfAddr(pcoa),
					 mipv6cdsMN->primaryHA()->prefix().prefix);
    if (!vIfIndex)
    {
      if (rt->hmipSupport())
      Dout(dc::mipv6, " reverse tunnel to HA not found as coa="<<mipv6cdsMN->careOfAddr(pcoa)
	   <<" is the old one and we have handed to new MAP, awaiting BA "
	   <<"from HA to use rcoa, packet dropped");
      else
	Dout(dc::mipv6, " reverse tunnel to HA not found as coa="<<mipv6cdsMN->careOfAddr(pcoa)
	     <<" not ready yet, awaiting BA from HA to use coa, packet dropped");
      Dout(dc::mipv6, " tunnels "<<*tunMod);
      return false;
    }

    Dout(dc::mipv6|dc::encapsulation|dc::debug|flush_cf, rt->nodeName()
	 <<" reverse tunnelling vIfIndex="<<hex<<vIfIndex<<dec
	 <<" for dest="<<datagram->destAddress());
    datagram->setOutputPort(vIfIndex);
    tunnel = true;
    return tunnel;
  }

// }}}

  return true;
}

void MIPv6MStateMobileNode::mnSrcAddrDetermination(IPv6Datagram* datagram)
{
  ipv6_addr::SCOPE destScope = ipv6_addr_scope(datagram->destAddress());

  //We don't care which outgoing iface dest is on because it is determined by
  //default router ifIndex anyway (preferably iface on link with Internet
  //connection after translation to care of addr)
  if (datagram->srcAddress() == IPv6_ADDR_UNSPECIFIED &&
      mipv6cdsMN->primaryHA().get() != 0 &&
      destScope == ipv6_addr::Scope_Global)
  {
    RoutingTable6* rt = mob->rt;
    Dout(dc::mipv6, rt->nodeName()<<" using homeAddress "
	 <<rt->mipv6cds->mipv6cdsMN
	 ->homeAddr()<<" for destination "<<datagram->destAddress());
    datagram->setSrcAddress(mipv6cdsMN->homeAddr());
  }
}

void MIPv6MStateMobileNode::parseXMLAttributes()
{
  XMLConfiguration::XMLOmnetParser* p = OPP_Global::getParser();
  cXMLElement* ne = p->getNetNode(mob->nodeName());
  if (!ne)
    ne = mob->rt->par("baseSettings");
  if (ne)
  {
    mipv6cdsMN->setEagerHandover(p->getNodePropBool(ne, "eagerHandover"));

    mipv6cdsMN->setSendBUAckFlag(p->getNodePropBool(ne, "mnSendBUAckFlag"));
  }
}
} //namespace MobileIPv6
