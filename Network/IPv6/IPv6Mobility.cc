//
// Copyright (C) 2006 by Johnny Lai
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
#include "InterfaceTableAccess.h"
#include "RoutingTable6Access.h"

#ifdef USE_MOBILITY

#include "MIPv6MobilityState.h"
#include "MIPv6MStateMobileNode.h"
#include "MIPv6MStateHomeAgent.h"

#include "WirelessEtherModule.h" // for linklayer trigger enum values

#include "opp_utils.h" //getParser()
#include "XMLOmnetParser.h"
#include "XMLCommon.h"

#include "HMIPv6MStateMobileNode.h"

#endif //USE_MOBILITY


#include "cTimerMessage.h"

#include "NotificationBoard.h" 
#include "NotifierConsts.h"
#include <iostream>
#include "opp_utils.h" //abort_ipv6suite

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

IPv6Mobility::IPv6Mobility(const char *name, cModule *parent)
    :cSimpleModule(name, parent, 0),
     ift(InterfaceTableAccess().get()),
     rt(RoutingTable6Access().get()),
     role(0), _routeOptimise(false),
     _returnRoutability(false), _isEBU(false),
     ewuOutVectorHODelays(false),
     linkUpTime(0),
     _signalingEnhance(MobileIPv6::SignalingEnhance()),
     prevLinkUpTime(0),
     avgCellResidenceTime(0),
     handoverCount(0),
     linkDownTime(0),
     handoverDelay(0),
     linkUpVector(0),
     linkDownVector(0),
     backVector(0),
     buVector(0),
     lbuVector(0),
     lbackVector(0),
     bbuVector(0),
     bbackVector(0),
     rsVector("RS sent"),
     raVector("RA recv"),
     nsVector("NS sent"),
     naVector("NA sent"),
     globalAddrAssignedVector("global addr assigned"),
     hotiVector("HOTI sent"),
     hotVector("HOT recv"),
     cotiVector("COTI sent"),
     cotVector("COT recv"),
     nb(0),
     ehType(""), 
     ehCallback(0)
{
}

IPv6Mobility::~IPv6Mobility()
{
  delete role;
  delete linkDownVector;
  delete linkUpVector;
}


/**
 * @todo simulation parameter for periodic interval to clean up expired entries
 *
 */

void IPv6Mobility::initialize(int stage)
{

  if (stage == 2)
  {
    nb = NotificationBoardAccess().get();
    nb->subscribe(this, NF_L2_BEACON_LOST);
    nb->subscribe(this, NF_L2_ASSOCIATED);
    nb->subscribe(this, NF_IPv6_NS_SENT);
    nb->subscribe(this, NF_IPv6_NA_SENT);
    nb->subscribe(this, NF_IPv6_RA_RECVD);
    nb->subscribe(this, NF_IPv6_RS_SENT);
    nb->subscribe(this, NF_IPv6_ADDR_ASSIGNED);
  }

  if (!rt->mobilitySupport())
    return;

#ifdef USE_MOBILITY

  if (stage == 1)
  {
      if (isHomeAgent())
        role = new MIPv6MStateHomeAgent(this);
      else if (isMobileNode())
      {
	if (rt->hmipSupport())
	  role = new HierarchicalMIPv6::HMIPv6MStateMobileNode(this);
	else
	  role = new MIPv6MStateMobileNode(this);
      }
      else
        role = new MIPv6MStateCorrespondentNode(this);

      role->initialize(stage);
      //does nothing currently
      parseXMLAttributes();
  }
  else if (stage == 2)
  {
    role->initialize(stage);
    linkUpVector = new cOutVector("L2 Up");
    linkDownVector = new cOutVector("L2 Down");
    assert(returnRoutability());    
  }
#endif // USE_MOBILITY

}

void IPv6Mobility::finish()
{
}

void IPv6Mobility::handleMessage(cMessage* msg)
{

  if (!rt->mobilitySupport())
  {
    //Todo send ICMP Parameter Problem Code 0 as do not understand header 
    delete msg;
    return;
  }

#ifdef USE_MOBILITY
  if (!msg->isSelfMessage())
  {
    IPv6Datagram* dgram = check_and_cast<IPv6Datagram*>(msg);
    assert(dgram);
    if (!dgram)
      return;

    if (role->processMobilityMsg(dgram))
      delete dgram;

  }
  else
  {
    (check_and_cast< cTimerMessage *> (msg) )->callFunc();
  }

#endif


}

void IPv6Mobility::setEarlyBindingUpdate(bool isEBU)
{
  if (!_returnRoutability && isEBU)
  {
    std::cerr<<"Error: "<<fullPath()<<" Early BU is true while Route Optimisation is off"<<endl;
    abort_ipv6suite();
  }
  _isEBU = isEBU;
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
    boost::polymorphic_downcast<MobileIPv6::MIPv6MStateMobileNode*>(role)->
      recordHODelay(buRecvTime, addr);
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
{}

/*
//seperate from l2 trigger to force movement detection callback
void IPv6Mobility::processLinkLayerTrigger(cMessage* msg)
{
  //we never use this now instead NotificationBoard for lu/ld messages
  assert(false);
  if ( msg->kind() == LinkDOWN)
  {
    //linkDownVector->record(msg->timestamp());
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
    //linkUpVector->record(msg->timestamp());
    if (ewuOutVectorHODelays)
    {
      Dout(dc::mipv6, nodeName()<<":"<< simTime() << " sec, link layer linkup trigger received.");
      linkUpTime = msg->timestamp();
    }
  }
}
*/

void IPv6Mobility::receiveChangeNotification(int category, cPolymorphic *details)
{
  Enter_Method_Silent();
  printNotificationBanner(category, details);

  if (category == NF_L2_BEACON_LOST)
  {
    linkDownTime = simTime();
    linkDownVector->record(linkDownTime);
  }
  else if (category == NF_L2_ASSOCIATED)
  {
    linkUpTime = simTime();
    linkUpVector->record(linkUpTime);
  }
  else if (category == NF_IPv6_RA_RECVD)
    raVector.record(simTime());
  else if (category == NF_IPv6_RS_SENT)
    rsVector.record(simTime());
  else if (category == NF_IPv6_NS_SENT)
    nsVector.record(simTime());
  else if (category == NF_IPv6_NA_SENT)
    naVector.record(simTime());
  else if (category == NF_IPv6_ADDR_ASSIGNED)
  {
    //actual value is coa acquisition time
    globalAddrAssignedVector.record(simTime()-linkUpTime);
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
  ::cTimerMessage* oldcb = ehCallback;
  if (oldcb && oldcb->isScheduled())
    oldcb->cancel();
  ehCallback = ehcb;
  return oldcb;
}

bool IPv6Mobility::processReceivedHoADestOpt(ipv6_addr hoa, IPv6Datagram* dgram)
{
  return role->processReceivedHoADestOpt(hoa, dgram);
}
#endif //EDGEHANDOVER
