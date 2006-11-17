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
 * @file IPv6Mobility.h
 * @author Johnny Lai
 * @date 14 Apr 2002
 * @brief SimpleModule to handle most MIPv6 related operations
 *
 * Contains the Home Agents List (HAL), Binding Cache (BC), Binding Updates List
 * (BUL) and maintain the lifetime of those entries according to Draft 16.
 * Provides an interface for MIPv6NDStateHomeAgent/Host to update HAL, BC.
 * Provides interface to update BUL for MIPv6StateMobileNode.
 *
 */

#ifndef IPV6MOBILITY_H
#define IPV6MOBILITY_H

#include <iostream>
#include <omnetpp.h>
#include <string>

#include "opp_utils.h" //abort_ipv6suite

#ifdef USE_MOBILITY
struct ipv6_addr;

#ifndef LIST
#define LIST
#include <list>
#endif //LIST

#endif


namespace MobileIPv6
{
  // OMNeT++ self messages
  extern const int Tmr_BURetransmission;
  extern const int Tmr_MIPv6Lifetime;
  extern const int Tmr_L2Trigger;
  extern const int Tmr_L2DelayRecorder;
  extern const int Tmr_L2LinkDownRecorder;

  extern const int Sched_SendBU;
  extern const int Sched_SendHoTI;
  extern const int Sched_SendCoTI;

  enum SignalingEnhance
  {
    None = 0,
    Direct = 1,
    CellResidency = 2
  };

  class MIPv6CDS;
  class MIPv6MStateCorrespondentNode;
  class MIPv6MStateMobileNode;
  class MIPv6MStateHomeAgent;
  class MIPv6MobilityState;
}

class cTimerMessage;
class RoutingTable6;
class InterfaceTable;

/**
 * @class IPv6Mobility
 *
 * @brief Processes MIPv6 protocol messages and manages lifetime of MIPv6
 * Conceptual data structures
 */

class IPv6Mobility : public cSimpleModule
{
public:

  friend class MobileIPv6::MIPv6MStateCorrespondentNode;
  friend class MobileIPv6::MIPv6MStateMobileNode;
  friend class MobileIPv6::MIPv6MStateHomeAgent;
  friend class MobileIPv6::MIPv6MobilityState;
  friend class XMLConfiguration::XMLOmnetParser;

  //@name constructors, destructors and operators
  //@{
  //Module_Class_Members(IPv6Mobility, cSimpleModule, 0);
  IPv6Mobility(const char *name=NULL, cModule *parent=NULL);
  ~IPv6Mobility();
  //@}

  //@name reimplemented cSimpleModule functions
  //@{
  virtual void finish();
  virtual void initialize(int stage);
  virtual int numInitStages() const  {return 3;}
  virtual void handleMessage(cMessage* msg);

  //@}

  bool isMobileNode();

  bool isHomeAgent();

  bool isCorrespondentNode();

  bool routeOptimise() { return _routeOptimise; }
  void setRouteOptimise(bool ro) { _routeOptimise = ro; }

  bool returnRoutability() { return _returnRoutability; }
  void setReturnRoutability(bool rr)
    {
      assert(rr);
      _returnRoutability = rr;
    }

  bool earlyBindingUpdate() { return _isEBU; }
  void setEarlyBindingUpdate(bool isEBU)
    {
      if (!_returnRoutability && isEBU)
      {
        std::cerr<<"Error: "<<fullPath()<<" Early BU is true while Route Optimisation is off"<<endl;
        abort_ipv6suite();
      }
      _isEBU = isEBU;
    }

  void setLinkUpTime(simtime_t t) { linkUpTime = t; }
  simtime_t getLinkUpTime(void) { return linkUpTime; }

  bool isEwuOutVectorHODelays() const { return ewuOutVectorHODelays; }

  // handover in this case is time when obtaining new CoA - link up time
  void recordHODelay(simtime_t t)
  {
    handoverLatency->record( t - linkUpTime );
  }

  MobileIPv6::SignalingEnhance signalingEnhance();
  void setSignalingEnhance(MobileIPv6::SignalingEnhance s);

  void recordHODelay(simtime_t buRecvTime, const ipv6_addr& addr);

  void parseXMLAttributes();


#ifdef USE_HMIP
  bool isMAP() const;

  bool hmipSupport() const;

#if EDGEHANDOVER
  bool edgeHandover() const
  {
    return ehType != "";
  }

  const std::string& edgeHandoverType() const
  {
    return ehType;
  }

  void setEdgeHandoverType(const std::string& type)
  {
    ehType = type;
  }

  ///Returns the callback pointer
  cTimerMessage* edgeHandoverCallback(){ return ehCallback; }

  ///The old callback is returned if one exists and is cancelled.
  cTimerMessage* setEdgeHandoverCallback(cTimerMessage* ehcb);

#endif //EDGEHANDOVER
#endif // USE_HMIP

  const char* nodeName() const;

  InterfaceTable *ift;
  RoutingTable6 *rt;

  ///should renmae to MobilityRole
  MobileIPv6::MIPv6MobilityState* role;
//  MobileIPv6::MIPv6CDS* mipv6cds;

  /**
   @brief RFC 3775 9.3.1	
   @param hoa home address from the home address destination option inside dgram
   @param dgram received datagram
   @return true if dgram for further processing, false when dgram should be dropped
  */
  bool processReceivedHoADestOpt(ipv6_addr hoa, IPv6Datagram* dgram);

private:

  IPv6Mobility(const IPv6Mobility& src);
  IPv6Mobility& operator=(IPv6Mobility& src);

  void processLinkLayerTrigger(cMessage* msg);

  ///Enable route optimisation?
  bool _routeOptimise;

  bool _returnRoutability;

  bool _isEBU;

  bool ewuOutVectorHODelays;
  simtime_t linkUpTime; // time when establishing with new link
  MobileIPv6::SignalingEnhance _signalingEnhance;

  simtime_t prevLinkUpTime; // Internet link up
  simtime_t avgCellResidenceTime;
  int handoverCount;

public:

  simtime_t linkDownTime;

  // parameters for cell resdiency signaling
  simtime_t handoverDelay;

  // handoverLatency is time when obtaining new CoA - link up time
  cOutVector* handoverLatency;
  cOutVector* linkUpVector;
  cOutVector* linkDownVector;
  cOutVector* backVector;
  cOutVector* buVector;
  //local BU to MAP
  cOutVector* lbuVector;
  //local BA to MAP  
  cOutVector* lbackVector;
  //BU to Bound Map in Edge Handover
  cOutVector* bbuVector;
  //BA from Bound Map Edge Handover
  cOutVector* bbackVector;

private:
#if EDGEHANDOVER
  ///Algorithm used for edge handover. Controls which subclass of HMIPv6NDStateHost gets created
  std::string ehType;

  /**
     @brief Callback function invoked by processBA when EH algorithm is specified in
      XML.

      While the pointer exists here the actual owner module is the one that
      created the callback usually ND. If this was placed in ND we'd have to
      have a reference to ND which is against the grain of how things work since
      ND is the conductor.
  */
  cTimerMessage* ehCallback;
#endif
};


#endif /* IPV6MOBILITY_H */

