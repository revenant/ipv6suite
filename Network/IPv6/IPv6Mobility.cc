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
#include "RoutingTable6.h"

#ifdef USE_MOBILITY
#include "MIPv6MobilityState.h"
#include "MIPv6MessageBase.h"
#include "MIPv6CDS.h"
#include "MIPv6CDSHomeAgent.h"
#include "MIPv6CDSMobileNode.h"
#include "MIPv6Timers.h"
#include "MIPv6MobilityState.h"
#include "MIPv6MStateMobileNode.h"
#ifndef __MIPv6ENTRY_H__
#include "MIPv6Entry.h"
#endif // __MIPv6ENTRY_H__

#if defined OPP_VERSION && OPP_VERSION >= 3
#include "opp_utils.h" //getParser()
#include "XMLOmnetParser.h"
#include "XMLCommon.h"
#endif

#endif

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
    mipv6cds = 0;
    _MobilityState = 0;
    periodTmr = 0;
#ifdef USE_MOBILITY
    l2LinkDownTime = 0;
    handoverLatency = new cOutVector("handoverLatency");
#if EDGEHANDOVER
    ehCallback = 0;
    //This overwrites the value assigned by XML and I thought this was run before parsing !
    //ehType = "";
#endif // EDGEHANDOVER
#endif // USE_MOBILITY

    RoutingTable6Access::initialize();
  }
  else if (stage == 1)
  {
#ifdef USE_MOBILITY
    if (rt->mobilitySupport())
    {
      if (isHomeAgent())
        mipv6cds = new MIPv6CDSHomeAgent(rt->interfaceCount());
      else if (isMobileNode())
      {
#ifdef USE_HMIP
        if (!rt->hmipSupport())
#endif //USE_HMIP
          mipv6cds = new MIPv6CDSMobileNode(rt->interfaceCount());
#ifdef USE_HMIP
        else
        {
#if EDGEHANDOVER
          if (edgeHandover())
            {
              Dout(dc::eh, nodeName()<<" Edgehandover instance created");
              mipv6cds = new EdgeHandover::EHCDSMobileNode(rt->interfaceCount());
            }
          else
#endif //EDGEHANDOVER
          mipv6cds = new HierarchicalMIPv6::HMIPv6CDSMobileNode(rt->interfaceCount());
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

}

void IPv6Mobility::finish()
{
#ifdef USE_MOBILITY
  if (periodTmr && !periodTmr->isScheduled())
    delete periodTmr;
  periodTmr = 0;
  delete mipv6cds;
  mipv6cds = 0;
#endif
}

void IPv6Mobility::handleMessage(cMessage* msg)
{

#ifdef USE_MOBILITY
  if (!msg->isSelfMessage())
  {
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
  if ( isMobileNode() )
  {
    boost::polymorphic_downcast<MobileIPv6::MIPv6MStateMobileNode*>(_MobilityState)->
      recordHODelay(buRecvTime, addr, this);
  }
}

void IPv6Mobility::parseXMLAttributes()
{
#if defined OPP_VERSION && OPP_VERSION >= 3
  if (isMobileNode())
  {
    XMLConfiguration::XMLOmnetParser* p = OPP_Global::getParser();
    MIPv6CDSMobileNode* mipv6cdsMN = check_and_cast<MIPv6CDSMobileNode*>(mipv6cds);
    mipv6cdsMN->setEagerHandover(p->getNodePropBool(p->getNetNode(nodeName()), "eagerHandover"));
  }
#endif

}

#endif // USE_MOBILITY

#ifdef USE_HMIP
  bool IPv6Mobility::isMAP()
    {
      return rt->isMAP();
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
