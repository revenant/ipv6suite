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

#include "MIPv6MStateMobileNode.h"
#include "cTTimerMessageCB.h"
#include "IPv6Datagram.h"
#include "IPv6Mobility.h"
#include "MIPv6MobilityHeaders.h"
#include "MIPv6CDSMobileNode.h"
#include "MIPv6Entry.h"
#include "MIPv6MNEntry.h"
#include "MIPv6DestOptMessages.h" //MIPv6OptHomeAddress
#include "opp_utils.h" //ContextSwitcher
#include "IPv6Encapsulation.h" // for sendHoTI
#include "RoutingTable6.h" // for sendBU
#include "InterfaceTable.h"
#include "IPv6InterfaceData.h"
#include "MIPv6MobilityOptions_m.h"
#include "MIPv6Timers.h" //MIPv6PeriodicCB
#include "MIPv6MessageBase.h" //MIPv6MobilityHeaderBase

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

class BURetranTmr;
typedef std::list<BURetranTmr*> BURetranTmrs;
typedef BURetranTmrs::iterator BURTI;

/**
   @class BURetranTmr
   @brief Implement Binding Update retransmissions
*/

class BURetranTmr: public cTTimerMessageCBA<void, void>
{
  friend class MIPv6MStateMobileNode;

public:

  // Constructor/destructor.
  BURetranTmr(unsigned int timeout, IPv6Datagram* dgram,
              MIPv6MStateMobileNode* mn, IPv6Mobility* mob, MIPv6CDS* mipv6cds)
    :cTTimerMessageCBA<void, void>
  (Tmr_BURetransmission, mob, makeCallback(this, &BURetranTmr::retransmitBU),
   "BURetransmissionTmr"), timeout(timeout), dgram(dgram), stateMN(mn),
    mipv6cds(mipv6cds)
    {}
  virtual ~BURetranTmr(void)
    {
//stop it from dumping core at end run of sim
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
      if (dgram->destAddress() == mipv6cds->mipv6cdsMN->primaryHA()->addr())
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
          if (mipv6cds->hmipv6cdsMN->isMAPValid() && mipv6cds->hmipv6cdsMN->currentMap().addr() == dgram->destAddress())
          {
            mob->lbuVector->record(now);
          }
      }
#endif //USE_HMIP

      assert(dgram->destAddress() != IPv6_ADDR_UNSPECIFIED);
      Dout(dc::mipv6|flush_cf, nodeName<<" "<<now<<" Resending BU for the "
           <<dgram->destAddress());

    }

private:
  ///now-msg->sendingTime() = timeout
  unsigned int timeout;
public:
  IPv6Datagram* dgram;
  MIPv6MStateMobileNode* stateMN;
  MIPv6CDS* mipv6cds;

};


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
        mstateMN->sendInits(bule->addr(), coa);
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
  mipv6cdsMN(0), hmipv6cds(0), ehcds(0)
{
  InterfaceTable *ift = mod->ift;
  mipv6cdsMN = new MIPv6CDSMobileNode(ift->numInterfaceGates());
  mipv6cds->mipv6cdsMN = mipv6cdsMN;
  if (mod->rt->hmipSupport())
  {
    hmipv6cds = new HierarchicalMIPv6::HMIPv6CDSMobileNode(mipv6cds, ift->numInterfaceGates());
    mipv6cds->hmipv6cdsMN = hmipv6cds;
  }
  if (mob->edgeHandover())
  {
    ehcds = new EdgeHandover::EHCDSMobileNode(mipv6cds, ift->numInterfaceGates());
    mipv6cds->ehcds = ehcds;
  }
  if (!periodTmr)
    periodTmr = new MIPv6PeriodicCB(mob, mipv6cdsMN->setupLifetimeManagement(),
                                    MIPv6_PERIOD);

	mob->backVector = new cOutVector("BAck recv");
	mob->buVector = new cOutVector("BU sent");
	mob->lbuVector = new cOutVector("LBU sent");
	mob->lbackVector = new cOutVector("LBAck recv");
	//BU to Bound map
	mob->bbuVector = new cOutVector("BBU sent");
    mob->bbackVector = new cOutVector("BBAck recv");
}

MIPv6MStateMobileNode::~MIPv6MStateMobileNode(void)
{
  delete mipv6cdsMN;
  delete mipv6cdsMN;
  delete hmipv6cds;
  delete ehcds;

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
  }
}

bool MIPv6MStateMobileNode::processMobilityMsg(IPv6Datagram* dgram)
{

  MIPv6MobilityHeaderBase* mhb = mobilityHeaderExists(dgram);

  if (!mhb)
    return true;

  switch ( mhb->header_type() )
  {
    case MIPv6MHT_BA:
    {
      BA* ba = check_and_cast<BA*>(mhb);
      processBA(ba, dgram);
    }
    break;

    case MIPv6MHT_BM:
    {
      BM* bm = check_and_cast<BM*>(mhb);
      processBM(bm, dgram);
    }
    break;

    case MIPv6MHT_BR:
    {
      BR* br =  check_and_cast<BR*> (mhb);
      processBR(br, dgram);
    }
    break;
    case MIPv6MHT_HoT: case MIPv6MHT_CoT:
    {
      TMsg* t =  check_and_cast<TMsg*> (mhb);
      processTestMsg(t, dgram);
    }
    break;
    default:
      return MIPv6MStateCorrespondentNode::processMobilityMsg(dgram);
    break;
  }
  return true;
}

/**
 * 10.14
 *
 * ignoring authentication req. from 4.5 well there doesn't appear to be any
 * besides 4.5.5 point 6
 *
 * @todo fix ba->physicalLenInOctet() < ba->length()
 */

void MIPv6MStateMobileNode::processBA(BA* ba, IPv6Datagram* dgram)
{
  // check if the length in the option is actually greater or equal to
  // how long it SHOULD be according to the draft standard
//   if ( ba->physicalLenInOctet() < ba->length() )
//   {
// #if defined TESTMIPv6 || defined BA_DEBUG
//     cerr << mob->nodeName()<<" BA received with incorrect length "<<ba->length()
//          <<" instead of >"<<ba->physicalLenInOctet()<<"\n";
// #endif //defined TESTMIPv6 || defined BA_DEBUG
//     return;
//   }

  // check if the sequence number field matches the sequence number
  // sent by the mobile node to this destination address in an
  // outstanding binding update
  bu_entry* bue = mipv6cdsMN->findBU(dgram->srcAddress());
  if (bue == 0)
  {
#if defined TESTMIPv6 || defined BUL_DEBUG || defined BA_DEBUG
    cerr << mob->nodeName()<<" BA received from "<<dgram->srcAddress()
         <<" with no corresponding BUL entry\n";
#endif //defined TESTMIPv6 || defined BUL_DEBUG || defined BA_DEBUG
    Dout(dc::mipv6|dc::notice|flush_cf, mob->nodeName()<<" BA received from "
         <<dgram->srcAddress()<<" with no corresponding BUL entry");
    return;
  }

  if (ba->sequence() != bue->sequence())
  {
    Dout(dc::mipv6|dc::notice|flush_cf, mob->nodeName()<<" BA received from "
         <<dgram->srcAddress()<<" IGNORED. With sequence "<< ba->sequence()
         <<" instead of "<< bue->sequence());
    //11.7.3 
    return;
  }

  simtime_t now = mob->simTime();

  //cancel and delete BU retransmission timer (as received)

  //Warning: Assuming that we are sending only binding update for one care of
  //addr not more(when multiple interfaceID's exist), if we do then will need to
  //check exactly for which care of addr we are removing timer for
  bool found = false;
  for (BURTI it = buRetranTmrs.begin(); it != buRetranTmrs.end(); it++)
  {
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

  if (dgram->srcAddress() == mipv6cdsMN->primaryHA()->addr())
    mob->backVector->record(now);
#ifdef USE_HMIP
  else if (mob->hmipSupport())
  {
#if EDGEHANDOVER
    if (mob->edgeHandover() && dgram->srcAddress() == ehcds->boundMapAddr())
    {
      mob->bbackVector->record(now);
    } 
    else
#endif //EDGEHANDOVER
      if (hmipv6cds->isMAPValid() && hmipv6cds->currentMap().addr() == dgram->srcAddress())
      {
	mob->lbackVector->record(now);
      }
  }
#endif //USE_HMIP

  if (!found)
    Dout(dc::warning|dc::mipv6|flush_cf, mob->nodeName()<<" "<<now
         <<" unable to find BURetranTmr for deletion as received Back from "
         <<dgram->srcAddress());
  if (BA::BAS_ACCEPTED == ba->status())
  {
    Dout(dc::mipv6| flush_cf, mob->nodeName()<<" "<<now<<" BA received from "
         <<dgram->srcAddress()<<" seq="<<ba->sequence());

    bue->state = 0;

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

#ifdef USE_HMIP
    //mob->sendDirect(new ICMPv6Message(0,0, dgram->dup()), 0, nd, NDiscOut);
    if (mob->hmipSupport())
    {

      if (hmipv6cds->isMAPValid() && hmipv6cds->currentMap().addr() == dgram->srcAddress())
      {
        Dout(dc::hmip, mob->nodeName()<<" "<<now<<" BA from MAP "
             <<dgram->srcAddress());

        //Assuming single interface for now if assumption not true revise code
        //accordingly
        assert(dgram->inputPort() == 0);
        if (!mob->rt->addrAssigned(hmipv6cds->remoteCareOfAddr(), dgram->inputPort()))
        {
          IPv6Address addrObj(hmipv6cds->remoteCareOfAddr());
          addrObj.setPrefixLength(EUI64_LENGTH);
          addrObj.setStoredLifetimeAndUpdate(ba->lifetime());
          addrObj.setPreferredLifetime(ba->lifetime());
          mob->rt->assignAddress(addrObj, dgram->inputPort());
        }

        if (mipv6cdsMN->careOfAddr() != hmipv6cds->remoteCareOfAddr())
        {
#if EDGEHANDOVER
          if (!mob->edgeHandover() ||
              ///can't use EH as MAP is not on edge anyway
              hmipv6cds->currentMap().distance() > 1)
          {
#endif //EDGEHANDOVER
          Dout(dc::hmip, " sending BU to all coa="
               <<hmipv6cds->remoteCareOfAddr()<<" hoa="<<mipv6cdsMN->homeAddr());

          //Map is skipped
          sendBUToAll(hmipv6cds->remoteCareOfAddr(), mipv6cdsMN->homeAddr(),
                      bue->lifetime());
#if EDGEHANDOVER
          }
          else
          {
            if (!mob->edgeHandoverCallback())
              //Exit code of 254
              DoutFatal(dc::fatal, "You forgot to set the callback for edge handover cannot proceed");
            else
            {
              //if rcoa does not prefix match the dgram->srcAddress we ignore
              //since we do not want to bind with HA using a coa from a previous MAP.

              Dout(dc::eh, mob->nodeName()<<" invoking eh callback based on BA from bue "<<*bue
                   <<" coa="<<mipv6cdsMN->careOfAddr() <<" rcoa="<<hmipv6cds->remoteCareOfAddr()
                   <<" bcoa "<<ehcds->boundCoa());
              Loki::Field<0>((boost::polymorphic_downcast<EdgeHandover::EHCallback*>
                              (mob->edgeHandoverCallback()))->args) = dgram->dup();
              assert(dgram->inputPort() == 0);
              mob->edgeHandoverCallback()->callFunc();

              //Not something we should do as routerstate is for routers only
//               EdgeHandover::EHNDStateHost* ehstate =
//                 boost::polymorphic_downcast<EdgeHandover::EHNDStateHost*>
//                 (boost::polymorphic_downcast<NeighbourDiscovery*>(
//                   OPP_Global::findModuleByType(mob->rt, "NeighbourDiscovery"))->getRouterState());

//               ehstate->invokeMapAlgorithmCallback(dgram);

            }
          }
#endif //EDGEHANDOVER
        }
      }
#if EDGEHANDOVER
      else if (hmipv6cds->isMAPValid() && mob->edgeHandover() &&
               dgram->srcAddress() == mipv6cdsMN->primaryHA()->addr() &&
               ///Since we've restricted boundMaps to be only ARs (existing
               ///MIPv6RouterEntry when calling setBoundMap) they have to have a
               ///distance of 1.
               //Not true as we may want to bind with a previous map along the
               //edge so it will have distance > 1
               hmipv6cds->currentMap().distance() >= 1)
      {
        ///If the HA's BA is acknowledging binding with another MAP besides the
        ///currentMap's then this code block needs to be revised accordingly. It
        ///is possible for currentMap to have changed whilst updating HA.
        if (bue->careOfAddr() != hmipv6cds->remoteCareOfAddr())
        {
          Dout(dc::warning|flush_cf, "Bmap at HA is not the same as what we thought it was coa(HA)"
               <<bue->careOfAddr()<<" our record of rcoa "<<hmipv6cds->remoteCareOfAddr());
          assert(bue->careOfAddr() == hmipv6cds->remoteCareOfAddr());
        }

        Dout(dc::eh|flush_cf, mob->nodeName()<<" bmap is now "<<hmipv6cds->currentMap().addr()
             <<" inport="<<dgram->outputPort());
        ///outputPort should contain outer header's inputPort so we know which
        ///iface packet arrived on (see IPv6Encapsulation::handleMessage decapsulateMsgIn branch)
        ///original inport needed so we can configure the proper bcoa for multi homed MNs
        assert(dgram->outputPort() >= 0);
        ehcds->setBoundMap(hmipv6cds->currentMap(), dgram->outputPort());
      }
#endif //EDGEHANDOVER
    }
#endif //USE_HMIP

  }
  else
  {
    //Remove entry from BUL if BU failed.
    mipv6cdsMN->removeBU(bue->addr());
    Dout(dc::mipv6|dc::notice|flush_cf, mob->nodeName()<<" BU rejected by "
         <<dgram->srcAddress()<<" status="<<ba->status());

    //Don't know how to do optional recover from these errors for now sending
    //new BU.
  }
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
    )
  {
    assert(mipv6cdsMN->homeAddr() != IPv6_ADDR_UNSPECIFIED);

    //Send BU to pHA
    //Create pHA bul entry and update it
    sendBU(mipv6cdsMN->primaryHA()->addr(), coa, hoa,
           lifetime, true,
           mipv6cdsMN->primaryHA()->re.lock()->ifIndex());

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
         <<coa<<" oldcoa "<<oldcoa);
    if (oldcoa != coa)
    {

      //Iterate through every BU_entry and send updates to nodes with same hoa
      //(currently it just skips map indiscriminately)
      for_each(mipv6cdsMN->bul.begin(), mipv6cdsMN->bul.end(),
               sendBUs(coa, oldcoa, hoa, lifetime, mob, mipv6cdsMN));

#ifdef USE_HMIP
      //Forwarding from previous MAP to new MAP is already done in HMIP
      if (!mob->hmipSupport() ||
          (mob->hmipSupport() && !hmipv6cds->isMAPValid()) )
      {
#endif //USE_HMIP
        previousCoaForward(coa, oldcoa);
#ifdef USE_HMIP
      }
#endif //USE_HMIP
      //Don't need forwarding from previous MAP as that's done in
      //HMIPNDStateHost::processRtrAd
    }

  }
}

void MIPv6MStateMobileNode::processTestMsg(TMsg* testMsg, IPv6Datagram* dgram)
{
  Dout(dc::rrprocedure|flush_cf, "RR Procedure At " << mob->simTime() << " sec, " << mob->nodeName()<<" receives " << testMsg->className() << " src=" << dgram->srcAddress() << " dest= " << dgram->destAddress());

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

  if ( testMsg->header_type() == MIPv6MHT_CoT && mipv6cdsMN->careOfAddr() != dgram->destAddress())
  {
    Dout(dc::warning|dc::rrprocedure|flush_cf, "RR procedure ERROR: "
         << mob->nodeName()<<" dest="<<dgram->destAddress()
         <<"in "<< testMsg->className() << " does not match coa ="
         <<mipv6cdsMN->careOfAddr());
    return;
  }

  if ( testMsg->header_type() == MIPv6MHT_HoT && mipv6cdsMN->homeAddr() != dgram->destAddress())
  {
    Dout(dc::warning|dc::rrprocedure|flush_cf, "RR procedure ERROR: "
         << mob->nodeName()<<" dest="<<dgram->destAddress()
         <<"in "<< testMsg->className() << " does not match home address ="
         <<mipv6cdsMN->homeAddr());
    return;
  }

  // check if the core-of init cookie filed in the message matches the
  // value stored in the BUL
  if ( bule->cookie(testMsg->header_type()) != testMsg->cookie)
  {
    Dout(dc::warning|dc::rrprocedure|flush_cf, "RR procedure ERROR: " << mob->nodeName()<<" the init cookie filed in the " << testMsg->className() << " does not match the value stored in the BUL");
    return;
  }

  // check if the bule indicates that no keygen token has been
  // received yet
  if ( bule->token(testMsg->header_type()) == testMsg->token)
  {
    Dout(dc::warning|dc::rrprocedure|flush_cf, "RR procedure ERROR: " << mob->nodeName()<<" the BUL indicates that the token " << testMsg->className() << " has been received");
    return;
  }

  // all of the above tests have been passed

  // CELLTODO - dealt with this later
  if ( testMsg->header_type() == MIPv6MHT_CoT )
  {
    assert(dgram->timestamp());
    bule->_careOfTestRTT = mob->simTime() - dgram->timestamp();
  }

  bule->setToken(testMsg->header_type(), testMsg->token);

  if ( mob->signalingEnhance() == CellResidency )
  {
    // successful transmission when the signaling packets via direct
    // route reach to the destination first time
    if ( isDirectRoute &&
         bule->testInitTimeout(testMsg->header_type()) == INITIAL_BINDACK_TIMEOUT)
      bule->setTestSuccess(testMsg->header_type());

    // signaling loss happens even if signaling packets via indirect
    // route, so we need to increment the delay for the next signaling
    // tranmission
    else if ( !isDirectRoute && bule->testInitTimeout(testMsg->header_type()) != INITIAL_BINDACK_TIMEOUT )
    {
      bule->increaseSendDelayTimer(testMsg->header_type());
    }
  }

  bule->resetTITimeout(testMsg->header_type());

  Dout(dc::rrprocedure|flush_cf, "RR Procedure At " << mob->simTime() << " sec, " << mob->nodeName()<<" has verified that " << testMsg->className() << " src=" << srcAddr << " dest= " << dgram->destAddress() << " is valid");

  MIPv6MobilityHeaderType theOtherTestMsgType;
  if ( testMsg->header_type() == MIPv6MHT_HoT )
  {
    bule->hotiRetransTmr->cancel();
    theOtherTestMsgType = MIPv6MHT_CoT;

    if (mob->earlyBindingUpdate())
    {
      bule->hotiRetransTmr->reschedule(mob->simTime() + MAX_NONCE_LIFETIME);

      Dout(dc::rrprocedure|flush_cf, " RR procedure:(EARLY BU)  At " <<  mob->simTime()<< " sec, " << mob->nodeName() << " next home test will be at " << mob->simTime() + MAX_NONCE_LIFETIME<<" sec");
    }
  }
  else if ( testMsg->header_type() == MIPv6MHT_CoT )
  {
    bule->cotiRetransTmr->cancel();
    theOtherTestMsgType = MIPv6MHT_HoT;
  }

  if (bule->token(theOtherTestMsgType) != UNSPECIFIED_BIT_64)
  {
    if ( mob->signalingEnhance() == CellResidency && bule->testSuccess() )
      bule->incSuccessDirSignalCount();

    bule->resetTestSuccess();

    assert(dgram->timestamp());

    // send BU
    sendBU(dgram->srcAddress(), mipv6cdsMN->careOfAddr(),
           mipv6cdsMN->homeAddr(), mob->rt->minValidLifetime(),
           false, dgram->inputPort(), false, dgram->timestamp());
    Dout(dc::rrprocedure|flush_cf, "RR Procedure At " << mob->simTime()<< " sec, "
         << mob->rt->nodeName()
         <<" Correspondent Registration: sending BU to CN (Route Optimisation) dest= "
         << dgram->srcAddress());

    bule->setToken(MIPv6MHT_HoT, UNSPECIFIED_BIT_64);
    bule->setToken(MIPv6MHT_CoT, UNSPECIFIED_BIT_64);
    bule->isPerformingRR = false;
  }
}

void MIPv6MStateMobileNode::sendInits(const ipv6_addr& dest,
                                      const ipv6_addr& coa)
{
  OPP_Global::ContextSwitcher switchContext(mob);

  std::vector<ipv6_addr> addrs(2);
  addrs[0] = dest;
  addrs[1] = coa;

  // for signaling enhancement schemes, we need one more address just
  // to obtain binding update list entry for the cn with its home
  // address
  if ( mob->signalingEnhance() != None )
  {
    addrs.resize(3);
    addrs[2] = dest; // dest is currently cn's hoa
  }

  bu_entry* bule = mipv6cdsMN->findBU(dest);

  bool isNewBU = false;
  if(!bule)
  {
    isNewBU = true;

    bule = new bu_entry(dest, mipv6cdsMN->homeAddr(), coa, mob->rt->minValidLifetime(), 0, 0, false);

    if ( mob->isEwuOutVectorHODelays() )
    	bule->regDelay = new cOutVector("CN reg (including RR)");

    mipv6cdsMN->addBU(bule);

    TIRetransTmr* hotiTmr;
    hotiTmr = new TIRetransTmr(Sched_SendHoTI, mob, this,
                                 &MobileIPv6::MIPv6MStateMobileNode::sendHoTI, "Sched_SendHoTI");
    bule->hotiRetransTmr = hotiTmr;

    TIRetransTmr* cotiTmr;
    cotiTmr = new TIRetransTmr(Sched_SendCoTI, mob, this,
       &MobileIPv6::MIPv6MStateMobileNode::sendCoTI, "Sched_SendCoTI");
    bule->cotiRetransTmr = cotiTmr;
  }
  else
  {
    if (bule->isPerformingRR || mob->simTime() < bule->last_time_sent + (simtime_t) 1/3) /*MAX_UPDATE_RATE*/  // for some reason, MAX_UPDATE_RATE keeps returning zero.. I can't be stuffed fixing it.. just leave it for now
      return;
  }

  Loki::Field<1> (bule->hotiRetransTmr->args) = mob->simTime();
  Loki::Field<1> (bule->cotiRetransTmr->args) = mob->simTime();

  bule->isPerformingRR = true;

  simtime_t hotiScheduleTime = mob->simTime() + SELF_SCHEDULE_DELAY;
  simtime_t cotiScheduleTime = mob->simTime() + SELF_SCHEDULE_DELAY;

  /*** SIGNALING ENHANCEMENT SCHEMES ***/

  if ( mob->signalingEnhance() == Direct )
  {
    boost::weak_ptr<bc_entry> bce = mipv6cds->findBinding(dest);
    if(bce.lock())
      addrs[0] = bce.lock()->care_of_addr; // dest being the CN's coa
  }
  else if ( mob->signalingEnhance() == CellResidency )
  {
    boost::weak_ptr<bc_entry> bce = mipv6cds->findBinding(dest);

    if(bce.lock()) // when CN is also mobile
    {
      simtime_t elapsedTime = mob->simTime() - bce.lock()->buArrivalTime;
      simtime_t cnThreshold = bce.lock()->avgHandoverDelay + CELL_RESI_THRESHOLD;
/*
      if ( mob->avgHandoverDelay && mob->avgHandoverDelay + INITIAL_BINDACK_TIMEOUT < MN_THRESHOLD )
        mnThreshold = mob->avgHandoverDelay + INITIAL_BINDACK_TIMEOUT;
      else
        mnThreshold = MN_THRESHOLD;
*/
      if ( bce.lock()->avgCellResidenceTime - elapsedTime > cnThreshold )
        addrs[0] = bce.lock()->care_of_addr; // direct signaling
    }
    /*

    if ( bule->dirSignalCount() <= INITIAL_SIGNALING_COUNT )
    {
      if(bce.lock())
      {
        addrs[0] = bce.lock()->care_of_addr; // dest being the CN's coa
        bule->incDirSignalCount();
      }
    }
    else
    {
      assert(bce.lock());

      double probSuccess = (double)bule->successDirSignalCount() / bule->dirSignalCount();

      bool dirSignalDecider = ( probSuccess >= uniform(0,1) );

      if ( dirSignalDecider )
      {
        addrs[0] = bce.lock()->care_of_addr; // dest being the CN's coa
        bule->incDirSignalCount();
      }
      // we are facing high rate of signaling loss, fall back to
      // indirect signaling
      else
      {
        hotiScheduleTime += bule->hotiSendDelayTimer();
        cotiScheduleTime += bule->cotiSendDelayTimer();
      }
      }      */
  }

  Loki::Field<0> (bule->hotiRetransTmr->args) = addrs;
  Loki::Field<0> (bule->cotiRetransTmr->args) = addrs;

  if ( !mob->earlyBindingUpdate() && bule->hotiRetransTmr->isScheduled() )
  {
    Dout(dc::rrprocedure|flush_cf, " RR procedure ERROR: At " <<  mob->simTime()<< " sec, " << mob->nodeName() << " moves to a new foreign network but still retransmits the HoTI for the previous CoA. The timer message is canceled");
    bule->hotiRetransTmr->cancel();
  }

  if ( bule->cotiRetransTmr->isScheduled() )
  {
    Dout(dc::rrprocedure|flush_cf, " RR procedure ERROR: At " <<  mob->simTime()<< " sec, " << mob->nodeName() << " moves to a new foreign network but still retransmits the CoTI for the previous CoA. The timer message is canceled");
    bule->cotiRetransTmr->cancel();
  }

  if (!mob->earlyBindingUpdate() || (mob->earlyBindingUpdate() && isNewBU))
    bule->hotiRetransTmr->reschedule(hotiScheduleTime);

  bule->cotiRetransTmr->reschedule(cotiScheduleTime);
}

void MIPv6MStateMobileNode::sendHoTI(const std::vector<ipv6_addr> addrs, simtime_t timestamp)
{

  ipv6_addr dest = addrs[0];
  const ipv6_addr& coa = addrs[1];
  ipv6_addr cnhoa;
  if ( mob->signalingEnhance() != None )
    cnhoa= addrs[2];

  Dout(dc::rrprocedure|flush_cf, " RR procedure: At " <<  mob->simTime()<< " sec, " << mob->nodeName() << " is about to send a HoTI, dest= " << IPv6Address(dest));

  bu_entry* bule;
  if ( mob->signalingEnhance() == None )
    bule = mipv6cdsMN->findBU(dest);
  else
    bule = mipv6cdsMN->findBU(cnhoa);

  assert(bule);

  bule->increaseHotiTimeout();

  if (mob->signalingEnhance() != None &&
      bule->testInitTimeout(MIPv6MHT_HoTI) != INITIAL_BINDACK_TIMEOUT)
  {
    // only if the dest is CN's CoA, we then revert this address back
    // to CN's HoA
    dest = cnhoa;
  }

  cModule* outputMod = OPP_Global::findModuleByType(mob, "IPv6Output");
  assert(outputMod);

  TIMsg* hoti = new TIMsg(MIPv6MHT_HoTI, bule->cookie(MIPv6MHT_HoTI));
  hoti->cookie.high = rand();
  hoti->cookie.low = rand();
  bule->setCookie(MIPv6MHT_HoTI, hoti->cookie);

  IPv6Datagram* dgram_hoti = new IPv6Datagram(mipv6cdsMN->homeAddr(), dest, hoti);
  dgram_hoti->setTransportProtocol(IP_PROT_IPv6_MOBILITY);
  dgram_hoti->setTimestamp(timestamp);

  // TODO: return home; Since the return home handover isn't fully
  // robust, we will send the hoti straight to the outputcore instead
  // of forwardcore for now. coa can be removed when the return home
  // handover is fixed

  if (coa != mipv6cdsMN->homeAddr())
  {
    IPv6Encapsulation* tunMod = check_and_cast<IPv6Encapsulation*>(OPP_Global::findModuleByType(mob, "IPv6Encapsulation"));
    assert(tunMod);

    size_t vIfIndex = tunMod->findTunnel(coa,
                                         mipv6cdsMN->primaryHA()->prefix().prefix);
    if(!vIfIndex)
      vIfIndex = tunMod->createTunnel(coa, mipv6cdsMN->primaryHA()->prefix().prefix, 0);

    dgram_hoti->setOutputPort(vIfIndex);
    dgram_hoti->setHopLimit(mob->ift->interfaceByPortNo(0)->ipv6()->curHopLimit);
    mob->sendDirect(dgram_hoti, 0, tunMod, "mobilityIn");
  }
  else
  {
    dgram_hoti->setHopLimit(mob->ift->interfaceByPortNo(0)->ipv6()->curHopLimit);
    mob->sendDirect(dgram_hoti, 0, outputMod, "mobilityIn");
  }

  bule->hotiRetransTmr->reschedule(mob->simTime() + bule->testInitTimeout(MIPv6MHT_HoTI));

  Dout(dc::rrprocedure|flush_cf, " RR procedure: At " <<  mob->simTime()<< " sec, " << mob->nodeName() << " sending HoTI src= " << dgram_hoti->srcAddress() << " to " << dgram_hoti->destAddress() << "| next HoTI retransmission time will be at " << bule->testInitTimeout(MIPv6MHT_HoTI) + mob->simTime());
}

void MIPv6MStateMobileNode::sendCoTI(const std::vector<ipv6_addr> addrs, simtime_t timestamp)
{
  ipv6_addr dest = addrs[0];
  const ipv6_addr& coa = addrs[1];
  ipv6_addr cnhoa;
  if ( mob->signalingEnhance() != None )
    cnhoa= addrs[2];

  Dout(dc::rrprocedure|flush_cf, " RR procedure: At " <<  mob->simTime()<< " sec, " << mob->nodeName() << " is about to send a CoTI, dest= " << IPv6Address(dest));

  bu_entry* bule;
  if ( mob->signalingEnhance() == None )
    bule = mipv6cdsMN->findBU(dest);
  else
    bule = mipv6cdsMN->findBU(cnhoa);

  assert(bule);

  bule->increaseCotiTimeout();

  if (mob->signalingEnhance() != None &&
      bule->testInitTimeout(MIPv6MHT_CoTI) != INITIAL_BINDACK_TIMEOUT)
  {
    // only if the dest is CN's CoA, we then revert this address back
    // to CN's HoA
    dest = cnhoa;
  }

  cModule* outputMod = OPP_Global::findModuleByType(mob, "IPv6Output");
  assert(outputMod);

  TIMsg* coti = new TIMsg(MIPv6MHT_CoTI, bule->cookie(MIPv6MHT_CoTI));
  coti->cookie.high = rand();
  coti->cookie.low = rand();
  bule->setCookie(MIPv6MHT_CoTI, coti->cookie);

  // Once the tunnel is established, the packet destinated for the
  // particular tunnel will be sent in tunnel. We want the CoTI to be
  // sent directly to the correspondent node. Therefore it is sent
  // directly to the output core via "mobilityIn")

  IPv6Datagram* dgram_coti = new IPv6Datagram(coa, dest, coti);
  dgram_coti->setTransportProtocol(IP_PROT_IPv6_MOBILITY);
  dgram_coti->setTimestamp(timestamp);

  dgram_coti->setHopLimit(mob->ift->interfaceByPortNo(0)->ipv6()->curHopLimit);
  mob->sendDirect(dgram_coti, 0, outputMod, "mobilityIn");

  bule->cotiRetransTmr->reschedule(mob->simTime() + bule->testInitTimeout(MIPv6MHT_CoTI));

  Dout(dc::rrprocedure|flush_cf," RR procedure: At " << mob->simTime()<< " sec, "<< mob->nodeName() << " sending CoTI src= " << dgram_coti->srcAddress() << " to " << dgram_coti->destAddress()<< "| next CoTI retransmission time will be at " << bule->testInitTimeout(MIPv6MHT_CoTI) + mob->simTime());

  if (mob->earlyBindingUpdate())
  {
    // send early BU
    sendBU(dest, coa,
           mipv6cdsMN->homeAddr(), mob->rt->minValidLifetime(),
           false, 0);
    Dout(dc::rrprocedure|flush_cf, "RR Procedure (Early BU) At" << mob->simTime()<< " sec, " << mob->rt->nodeName()
         <<" Correspondent Registration: sending BU to CN (Route Optimisation) dest= "
         << IPv6Address(dest));
  }
}

/**
 * Binding Error in RFC 3775
 *
 */

void MIPv6MStateMobileNode::processBM(BM* bm, IPv6Datagram* dgram)
{
  bu_entry* bule = 0;

  if ((bule = mipv6cdsMN->findBU(dgram->srcAddress())) != 0)
  {
    Dout(dc::warning, " implement processBM for MN to send BU to CN");
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
  }
  else
    Dout(dc::warning, mob->nodeName()<<" "<<mob->simTime()
         <<" Unable to process BM from "<<dgram->srcAddress());
  // if the mobile node does not have a binding update list entry for
  // the source of the binding missing message, it MUST ignore the
  // message

}

/**
 * @brief Respond to known CN i.e. a node in BUL
 *
 */

void  MIPv6MStateMobileNode::processBR(BR* br, IPv6Datagram* dgram)
{
  //IPv6Datagram* dgram = check_and_cast<IPv6Datagram*> (br->encapsulatedMsg());
  size_t ifIndex = dgram->inputPort();
  bu_entry* bule = 0;

  //Check that CN is in BUL then send BU and update BUL details
  if ((bule = mipv6cdsMN->findBU(dgram->srcAddress())) != 0)
  {

    //if unique identified exists in br then send it back in the BU too

    //Latest lifetime
    unsigned int lifetime = 0;

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

void MIPv6MStateMobileNode::removeBURetranTmr(BURetranTmr* buTmr)
{
  buRetranTmrs.remove(buTmr);
  delete buTmr;
}

#ifdef USE_HMIP
bool MIPv6MStateMobileNode::sendMapBU(const ipv6_addr& dest, const ipv6_addr& coa,
                                      const ipv6_addr& hoa, size_t lifetime,
                                      size_t ifIndex)
{
  return sendBU(dest, coa, hoa, lifetime, false, ifIndex, true);
}
#endif //USE_HMIP

/*
  @brief Forward from PAR to current AR (when previous AR is an HA)

  @param coa is the new care of address
  @param hoa is the previous care of address

  @return true if forwarding BU to PAR was sent false otherwise
 */
bool MIPv6MStateMobileNode::previousCoaForward(const ipv6_addr& coa,
                                               const ipv6_addr& hoa)
{
  assert(coa != hoa);

  boost::shared_ptr<MIPv6RouterEntry> oldRtr = mipv6cdsMN->currentRouter()?
    mipv6cdsMN->currentRouter():mipv6cdsMN->previousDefaultRouter();
  if (oldRtr && oldRtr->isHomeAgent())
  {

#ifdef USE_HMIP
    //Required to prevent warnings like the following in debug log as b/rcoa is
    //not a valid home address for pure HA. Anyway HMIP/EH should bind with HAs as
    //hmip MAP and have only one HA

    //WARNING  :  hoa=30f4:0:0:3:c274:82ff:fea6:958b is not on link w.r.t. HA prefix list
    bool hmipFlag = false;
    if (mob->hmipSupport())
    {
      hmipFlag = hmipv6cds->mapEntries().count(oldRtr->addr()) == 1;
    }
#endif //USE_HMIP

    Dout(dc::mipv6, mob->nodeName()<<" pcoaf forwarding from PAR="
         <<*oldRtr.get()<<" pcoa="<<hoa<<" ncoa="<<coa);

    sendBU(oldRtr->addr(), coa, hoa,
           static_cast<unsigned int>(mipv6cdsMN->pcoaLifetime()), true,
           //don't care about ifIndex as DAD only done on
           //primaryHA and only the very first BU to it
           0, mob
#ifdef USE_HMIP
           , hmipFlag
#endif //USE_HMIP
           );
    return true;
  }
  return false;
}


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
 * remote startups this may be difficult to determine.  It is used to calculate
 * the retransmission of the BU in case we want a BA from HA.  In this impln
 * only home registrations will have Ack set in the BU. For non homeReg this can
 * will be ignored.
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
  //Can be called from NDStateHost(IPv6NeighbourDiscovery) or
  //itself(IPv6Mobility)
  // 2 lines equiv to Enter_Method except that uses this pointer implicitly
  OPP_Global::ContextSwitcher switchContext(mob);
  //switchContext.methodCall("sendBU(%s)", ipv6_addr_toString(dest).c_str());

  bool dad = false;
  bool ack = homeReg;

  ///Won't worry about multiple global on-link prefixes on home subnet.  We will only respond to one of them for now.
  const bool saonly = false;

  /////////////////

  if (homeReg)
  {
    dad = mipv6cdsMN->bulEmpty();
  }

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

  if (mob->returnRoutability() && !homeReg)
    assert(bule);

#ifdef USE_HMIP

  if (mapReg)
    ack = mapReg;

  if (!ack && mipv6cdsMN->sendBUAckFlag())
    ack = true;

  //In case we do forwarding from previous MAP we don't need to redo DAD on MAP
  //that has already been registered with old RCOA.
  if (mapReg && bule == 0)
  {
    dad = true;
  }
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

  bool useCellSignaling = (mob->signalingEnhance() == CellResidency ? true : false);

  BU* bu = new BU(ack, homeReg, saonly, dad, seq, lifetime, hoa
#ifdef USE_HMIP
                  , mapReg
#endif //USE_HMIP
                  , useCellSignaling
                  ,mob);

  // When cell residency signaling is enabled, send handover duration
  // information to the peer

  if ( useCellSignaling )
  {
    // add handover delay option
    HandoverDelay* delayInfo = new HandoverDelay;
    delayInfo->setOptType(MOPT_HandoverDelay);
// Don't remove this line; option length to be decided
// delayInfo->setOptLength();
    delayInfo->setDelay(mob->handoverDelay);
    bu->addMobilityOption(delayInfo);
  }

  IPv6Datagram* dgram = new IPv6Datagram(coa, dest, bu);
  dgram->setHopLimit(mob->ift->interfaceByPortNo(0)->ipv6()->curHopLimit);
  dgram->setTransportProtocol(IP_PROT_IPv6_MOBILITY);

  if (homeReg
#ifdef USE_HMIP
      || mapReg
#endif //USE_HMIP
      ) // BU sent to HA should not have any timestamp set
  {
    assert( !timestamp );
    dgram->setTimestamp( mob->simTime() );
  }
  else // BU sent to CN should already have timestamp from previous state
  {
#ifndef USE_HMIP
    assert( timestamp );
#else
//Removed assertion as SaitEHCal -r 1 will cause this to trigger. Don't know where to set timestamp
    if (!timestamp)
      timestamp = mob->simTime();
#endif
    dgram->setTimestamp( timestamp );
  }

  //Draft 17 6.1.7 BU must have haddr dest opt
  HdrExtDestProc* destProc = dgram->acquireDestInterface();

  destProc->addOption(new MIPv6TLVOptHomeAddress(hoa));

  if (mipv6cdsMN->primaryHA().get() && dgram->destAddress() == mipv6cdsMN->primaryHA()->addr())
    mob->buVector->record(mob->simTime());
  //scheduleSendBU(dgram);
  //mob->send(static_cast<cMessage*>(msg->dup()), gate);
  mob->send(dgram, "routingOut");

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
           <<" outstanding BU transmission already");
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
      bule->regDelay = new cOutVector("home reg");
    else if (mob->isEwuOutVectorHODelays())
      bule->regDelay = new cOutVector("CN reg (without RR)");
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

  IPv6Encapsulation* tunMod = check_and_cast<IPv6Encapsulation*>
    (OPP_Global::findModuleByType(mob, "IPv6Encapsulation"));
  assert(tunMod != 0);

  if (homeReg || mapReg)
  {
    //delete reverse tunnels with exit point of ha/map
    size_t delIndex = tunMod->findTunnel(IPv6_ADDR_UNSPECIFIED,
					 dgram->destAddress());
    size_t vIfIndex = tunMod->findTunnel(coa, dgram->destAddress());

    if (vIfIndex != delIndex)
    {
      mob->bubble("Destroy tunnel delIndex");
      tunMod->destroyTunnel(delIndex);
    }

  ///Returns the vIfIndex of tunnel if found
//  size_t findTunnelWithExitPoint(const ipv6_addr& exit);

    //Create reverse tunnel to HA/Map
    assert(!vIfIndex);
    if (!vIfIndex)
    {
      vIfIndex = tunMod->createTunnel(coa, dgram->destAddress(), ifIndex);
      Dout(dc::hmip|dc::encapsulation|dc::debug|flush_cf, mob->nodeName()
	   <<" reverse tunnel created entry coa="<<coa<<" exit "
	   <<(homeReg?"ha=":"map=")
	   <<dgram->destAddress()<<" vIfIndex="<<hex<<vIfIndex<<dec);
    }
    else
    {
      Dout(dc::warning|flush_cf, mob->nodeName()
	   <<" reverse tunnel exists already coa="<<coa<<" exit "
	   <<(homeReg?"ha=":"map=")
	   <<dgram->destAddress()<<" vIfIndex="<<hex<<vIfIndex<<dec);
    }

    if (!mob->hmipSupport())
      return true;

    HMIPv6CDSMobileNode* hmipv6cds = mipv6cds->hmipv6cdsMN;

    if (homeReg)
    {
      //trigger on HA to lcoa tunnel if coa was formed from map prefix

      //list of maps in here may be smaller or bigger than in bul depends on
      //whether the map list tracks the actual maps in current domain @see 
      //hmipv6ndstatehost::discoverMap
      ipv6_addr map = hmipv6cds->findMapOwnsCoa(coa);
      if (map != IPv6_ADDR_UNSPECIFIED)
      {
	bu_entry* bue = mipv6cdsMN->findBU(map);
	if (bue)
	{
	  vIfIndex = tunMod->findTunnel(bue->careOfAddr(), map);
	  assert(vIfIndex);
	  //trigger on ha only after BA for maximum correctness but nonoptimal
	  //operation?
	  tunMod->tunnelDestination(mipv6cdsMN->primaryHA()->addr(), vIfIndex);
	}
      }
    }
    else
    {
      //trigger on HA to different lcoa tunnel as we moved to different AR only
      //if this map is registered at HA
      if (mipv6cdsMN->primaryHA().get())
      {
	bu_entry* bue = mipv6cdsMN->findBU(mipv6cdsMN->primaryHA()->addr());
	if (bue && ipv6_prefix(dgram->destAddress(), EUI64_LENGTH).matchPrefix(
	      bue->careOfAddr()))
	{
	  mob->bubble("trigger on HA for lcoa -> MAP during map reg");
	  // vIfIndex points to tunnel lcoa -> map
	  tunMod->tunnelDestination(mipv6cdsMN->primaryHA()->addr(), vIfIndex);
	}
      }
    }
  }
  else //not (home or map reg)
  {
    //trigger on cn address to lcoa tunnel if coa formed from map prefix

    if (!mob->hmipSupport())
      return true;

    HMIPv6CDSMobileNode* hmipv6cds = mipv6cds->hmipv6cdsMN;

    //list of maps in here may be smaller or bigger than in bul depends on
    //whether the map list tracks the actual maps in current domain @see 
    //hmipv6ndstatehost::discoverMap
    ipv6_addr map = hmipv6cds->findMapOwnsCoa(coa);
    if (map != IPv6_ADDR_UNSPECIFIED)
    {
      bu_entry* bue = mipv6cdsMN->findBU(map);
      if (bue)
      {
	size_t vIfIndex = tunMod->findTunnel(bue->careOfAddr(), map);
	assert(vIfIndex);
	tunMod->tunnelDestination(dgram->destAddress(), vIfIndex);
      }
    }
  }
  return true;
}

/**
 * Sec. 10.9
 *
 */

bool MIPv6MStateMobileNode::updateBU(const BU* bu)
{
  return false;
}

void MIPv6MStateMobileNode::l2MovementDetectionCB(cMessage* msg)
{}

void MIPv6MStateMobileNode::parseXMLAttributes()
{
  XMLConfiguration::XMLOmnetParser* p = OPP_Global::getParser();
  mipv6cdsMN->setEagerHandover(p->getNodePropBool(p->getNetNode(mob->nodeName()), "eagerHandover"));

  mipv6cdsMN->setSendBUAckFlag(p->getNodePropBool(p->getNetNode(mob->nodeName()), "mnSendBUAckFlag"));
}
} //namespace MobileIPv6
