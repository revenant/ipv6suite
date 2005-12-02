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
 * @file   IPv6Mobility.cc
 * @author Johnny Lai
 * @date   14 Apr 2002
 *
 * @brief Implementation of IPv6Mobility simple module to handle most MIPv6
 * related operations
 *
 */

#include <boost/cast.hpp>
#include "sys.h"
#include "debug.h"

#include "IPv6Mobility.h"
#include "IPv6Datagram.h"
#include "HdrExtDestProc.h"
#include "InterfaceTableAccess.h"
#include "RoutingTable6Access.h"

#ifdef USE_MOBILITY
#include "MIPv6MobilityState.h"
#include "MIPv6MessageBase.h"
#include "MIPv6CDS.h"
#include "MIPv6CDSHomeAgent.h"
#include "MIPv6CDSMobileNode.h"
#include "MIPv6Timers.h"
#include "MIPv6MobilityState.h"
#include "MIPv6MStateMobileNode.h"

#include "WirelessEtherModule.h" // for linklayer trigger enum values

#ifndef __MIPv6ENTRY_H__
#include "MIPv6Entry.h"
#endif // __MIPv6ENTRY_H__

#if (defined OPP_VERSION && OPP_VERSION >= 3) || OMNETPP_VERSION >= 0x300
#include "opp_utils.h" //getParser()
#include "XMLOmnetParser.h"
#include "XMLCommon.h"
#endif

#include "IPvXAddress.h"
#include "IPAddressResolver.h"
#include "MIPv6MNEntry.h"
#include "IPv6CDS.h"
#include "MIPv6CDSMobileNode.h"
#endif //USE_MOBILITY

#ifdef USE_HMIP
#include "HMIPv6CDSMobileNode.h"
#if EDGEHANDOVER
#include "EHCDSMobileNode.h"
#endif
#endif

Define_Module(IPv6Mobility);

using namespace MobileIPv6;

namespace MobileIPv6
{
  // OMNeT++ self messages
  const int Tmr_BURetransmission = 8007;
  const int Tmr_MIPv6Lifetime = 8008;
  const int Tmr_L2Trigger = 8009;
  const int Tmr_L2DelayRecorder = 8010;
  const int Tmr_L2LinkDownRecorder = 8011;

  const int Sched_SendBU = 9001;
  const int Sched_SendHoTI = 9002;
  const int Sched_SendCoTI = 9003;
}

namespace
{
  const unsigned int MIPv6_PERIOD = 1 ; //seconds
};


/**
 * @todo simulation parameter for periodic interval to clean up expired entries
 *
 */

void IPv6Mobility::initialize(int stage)
{
  if (stage == 0)
  {
    ift = InterfaceTableAccess().get();
    rt = RoutingTable6Access().get();

    mipv6cds = 0;
    _MobilityState = 0;
    periodTmr = 0;
#ifdef USE_MOBILITY
    schedSendBU = 0;
    linkDownTime = 0;
    handoverDelay = 0;
    prevLinkUpTime = 0; 
    avgCellResidenceTime = 0;
    handoverCount = 0;
    ewuOutVectorHODelays = false;
    linkUpTime = 0;

#if EDGEHANDOVER
    ehCallback = 0;
    //This overwrites the value assigned by XML and I thought this was run before parsing !
    //ehType = "";
#endif // EDGEHANDOVER
#endif // USE_MOBILITY
  }
  else if (stage == 1)
  {
#ifdef USE_MOBILITY
    if (rt->mobilitySupport())
    {
      if (isHomeAgent())
        mipv6cds = new MIPv6CDSHomeAgent(ift->numInterfaceGates());
      else if (isMobileNode())
      {
        backVector = new cOutVector("BAck recv");
        buVector = new cOutVector("BU sent");
        lbuVector = new cOutVector("LBU sent");
        lbackVector = new cOutVector("LBAck recv");
        lbbuVector = new cOutVector("LBBU sent");
        lbbackVector = new cOutVector("LBBAck sent");
#ifdef USE_HMIP
        if (!rt->hmipSupport())
#endif //USE_HMIP
          mipv6cds = new MIPv6CDSMobileNode(ift->numInterfaceGates());
#ifdef USE_HMIP
        else
        {
#if EDGEHANDOVER
          if (edgeHandover())
            {
              Dout(dc::eh, nodeName()<<" Edgehandover instance created");
              mipv6cds = new EdgeHandover::EHCDSMobileNode(ift->numInterfaceGates());
            }
          else
#endif //EDGEHANDOVER
          mipv6cds = new HierarchicalMIPv6::HMIPv6CDSMobileNode(ift->numInterfaceGates());
        }
#endif //USE_HMIP
      }
      else
        mipv6cds = new MIPv6CDS;

      rt->mipv6cds = mipv6cds;

      periodTmr = new MIPv6PeriodicCB(this, mipv6cds->setupLifetimeManagement(),
                                      MIPv6_PERIOD);

      parseXMLAttributes();
    }
#endif // USE_MOBILITY
  }
  else if (stage == 2)
  {
#ifdef USE_MOBILITY
    if ( ewuOutVectorHODelays )
      handoverLatency = new cOutVector("L3 handover delay");
    linkUpVector = new cOutVector("L2 Up");
    linkDownVector = new cOutVector("L2 Down");
    
    if (rt->mobilitySupport() && isMobileNode() && par("homeAgent").stringValue()[0])
    { 
      ipv6_addr haAddr = IPv6_ADDR_UNSPECIFIED;
      IPvXAddress destAddr = IPAddressResolver().resolve(par("homeAgent"));
      if (destAddr.isNull())
        haAddr = c_ipv6_addr(par("homeAgent"));
      else
        haAddr = c_ipv6_addr(destAddr.get6().str().c_str());

      if (haAddr == IPv6_ADDR_UNSPECIFIED)
        return;
      Dout(dc::mipv6, nodeName()<<" using assigned homeAgent "<<haAddr
           <<" from par(homeAgent) "<<par("homeAgent"));
      IPv6NeighbourDiscovery::RouterEntry* re = new IPv6NeighbourDiscovery::RouterEntry(haAddr);
      re->setState(NeighbourEntry::INCOMPLETE);
      rt->cds->insertRouterEntry(re, false);
      boost::weak_ptr<IPv6NeighbourDiscovery::RouterEntry> bre = rt->cds->router(haAddr);
      MIPv6RouterEntry* ha = new MIPv6RouterEntry(bre, true, ipv6_prefix(haAddr, 64), VALID_LIFETIME_INFINITY);
      MIPv6CDSMobileNode* mipv6cdsMN = boost::polymorphic_downcast<MIPv6CDSMobileNode*>(mipv6cds);
      mipv6cdsMN->insertHomeAgent(ha);
      boost::shared_ptr<MIPv6RouterEntry> bha = mipv6cdsMN->findHomeAgent(haAddr);
      mipv6cdsMN->primaryHA() = bha;
      //Todo assuming HA is on ifIndex of 0
      bool primaryHoa = true;
      ipv6_prefix hoa = mipv6cdsMN->formHomeAddress(
        mipv6cdsMN->primaryHA(), ift->interfaceByPortNo(0), primaryHoa);
      rt->assignAddress(IPv6Address(hoa), 0);
      mipv6cdsMN->setAwayFromHome(true);
      mipv6cdsMN->currentRouter() = boost::shared_ptr<MIPv6RouterEntry>();
      rt->cds->removeDestEntryByNeighbour(haAddr);
    }
#endif // USE_MOBILITY

  }
}

void IPv6Mobility::finish()
{
  // XXX cleanup stuff must be moved to dtor!
  // finish() is NOT for cleanup -- that must be done in destructor, and
  // additionally, ptrs must be NULL'ed in constructor so that it won't crash
  // if initialize() hasn't run because of an error during startup! --AV
#ifdef USE_MOBILITY
  if (periodTmr && !periodTmr->isScheduled())
    delete periodTmr;
  periodTmr = 0;
  delete mipv6cds;
  mipv6cds = 0;
  delete schedSendBU;
  schedSendBU = 0;
#endif
}

void IPv6Mobility::handleMessage(cMessage* msg)
{

#ifdef USE_MOBILITY
  if (!msg->isSelfMessage())
  {
    if ( msg->arrivedOn("l2TriggerIn") )
    {
      processLinkLayerTrigger(msg);
      delete msg;
      return;
    }

    IPv6Datagram* dgram = static_cast<IPv6Datagram*>(msg);
    //assert(_MobilityState);
    //bool success = _MobilityState->nextState(dgram, this);
    bool success = MIPv6MobilityState::nextState(dgram, this);
    assert(_MobilityState);
    if (!success)
    {
      delete dgram;
      return;
    }

    MIPv6MobilityHeaderBase* mhb = 0;

    _MobilityState->processMobilityMsg(dgram, mhb, this);

    if (mhb)
    delete mhb;

    delete msg;
  }
  else
  {
    (check_and_cast<cTimerMessage *> (msg) )->callFunc();
  }

#endif


}

#ifdef USE_MOBILITY // calling the MOBILITY define module in RoutingTable6
  bool IPv6Mobility::isMobileNode()
    {
      return rt->isMobileNode();
    }

  bool IPv6Mobility::isHomeAgent()
    {
      return rt->isHomeAgent();
    }

  bool IPv6Mobility::isCorrespondentNode()
    {
      return rt->mobilitySupport();
    }

void IPv6Mobility::recordHODelay(simtime_t buRecvTime, const ipv6_addr& addr)
{
  if ( isMobileNode() && isEwuOutVectorHODelays() )
  {
    boost::polymorphic_downcast<MobileIPv6::MIPv6MStateMobileNode*>(_MobilityState)->
      recordHODelay(buRecvTime, addr, this);
  }
}

MobileIPv6::SignalingEnhance IPv6Mobility::signalingEnhance()
{
  return _signalingEnhance;
}

void IPv6Mobility::setSignalingEnhance(MobileIPv6::SignalingEnhance s)
{
  _signalingEnhance = s;
}

void IPv6Mobility::parseXMLAttributes()
{
#if (defined OPP_VERSION && OPP_VERSION >= 3) || OMNETPP_VERSION >= 0x300
  if (isMobileNode())
  {
    XMLConfiguration::XMLOmnetParser* p = OPP_Global::getParser();
    MIPv6CDSMobileNode* mipv6cdsMN = boost::polymorphic_downcast<MIPv6CDSMobileNode*>(mipv6cds);
    mipv6cdsMN->setEagerHandover(p->getNodePropBool(p->getNetNode(nodeName()), "eagerHandover"));
  }
#endif

}

void IPv6Mobility::processLinkLayerTrigger(cMessage* msg)
{
  if ( msg->kind() == LinkDOWN)
  {
    linkDownVector->record(msg->timestamp());
    // record MN's own handover delay
    if ( !prevLinkUpTime )
      return;    

    double totalLinkUpTime = avgCellResidenceTime * handoverCount;
    double prevCellResidenceTime = msg->timestamp() - prevLinkUpTime;
    avgCellResidenceTime = ( totalLinkUpTime + prevCellResidenceTime ) / 
      ( handoverCount + 1);
    
    prevLinkUpTime = 0;

    // record link down time
    linkDownTime = simTime();
  }
  else if ( msg->kind() == LinkUP)
  {
    linkUpVector->record(msg->timestamp());
    if (ewuOutVectorHODelays)
    {
      Dout(dc::mipv6, nodeName()<<":"<< simTime() << " sec, link layer linkup trigger received.");
      linkUpTime = msg->timestamp();
    }
  }
}

#endif // USE_MOBILITY

#ifdef USE_HMIP
  bool IPv6Mobility::isMAP() const
    {
      return rt->isMAP();
    }

bool IPv6Mobility::hmipSupport() const
{
  return rt->hmipSupport();
}
#endif // USE_HMIP

const char* IPv6Mobility::nodeName() const
{
  return rt->nodeName();
}

#if EDGEHANDOVER

cTimerMessage* IPv6Mobility::setEdgeHandoverCallback(cTimerMessage* ehcb)
{
  Dout(dc::eh, nodeName()<<" "<<simTime()<<" callback set to "<<ehcb->name()<<" and old one was "
       <<(ehCallback?ehCallback->name():"none"));
  cTimerMessage* oldcb = ehCallback;
  if (oldcb && oldcb->isScheduled())
    oldcb->cancel();
  ehCallback = ehcb;
  return oldcb;
}

#endif //EDGEHANDOVER
