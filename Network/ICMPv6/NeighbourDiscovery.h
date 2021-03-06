// -*- C++ -*-
// Copyright (C) 2001, 2004 Johnny Lai
// Monash University, Melbourne, Australia

/**
   @file NeighbourDiscovery.h
   @brief Definition of simple module NeighbourDiscovery.

   By default all nodes start up with default values as specified in
   RFC2461 for host/router parameters. Only when a node's IPForward
   property is set to true does the node become a router.  When the
   advSendAds flag for an interface is true then that interface
   becomes an advertising interface i.e.  one that sends unsolicited
   Router Ads.
*/
#if !defined __NEIGHBOURDISCOVERY_H
#define __NEIGHBOURDISCOVERY_H

#ifndef __OMNETPP_H
#include <omnetpp.h>
#endif //__OMNETPP_H


class ICMPv6Message;
class RoutingTable6;
class InterfaceTable;
class IPv6InterfaceData;
class NotificationBoard;

//Forward declarations
namespace IPv6NeighbourDiscovery
{
  // OMNeT++ self messages
  extern const int Ctrl_NodeInitialise;
  extern const int Tmr_DupAddrSolTimeout;
  extern const int Tmr_NextUnsolRtrAd;
  extern const int Tmr_RtrSolTimeout;
  extern const int Tmr_AddrConfLifetime;
  extern const int Tmr_RouterLifetime;
  extern const int Tmr_PrefixLifetime;

  class NDState;
  class NDStateHost;
  class NDStateRouter;
};

namespace MobileIPv6
{
  class MIPv6NDStateHost;
  class MIPv6NDStateRouter;

  extern const int Tmr_RtrAdvMissed;
  extern const int Sched_SendRtrSol;
  extern const int Sched_SendUnsolNgbrAd;
};

namespace XMLConfiguration
{
  class IPv6XMLParser;
  class IPv6XMLWrapManager;
  class XMLOmnetParser;
}

class cTimerMessage;

/**
   @class NeighbourDiscovery
   @brief module housing the Neighbour Discovery state machine.

   Provide a client interface to Neighbour Discovery state machine for
   ICMP Layer to use.  Each interface has its own configurable state
   i.e.  some interfaces act as routers by advertising while others
   are by default hosts.
 */
class NeighbourDiscovery: public cSimpleModule
{
 public:
  friend class IPv6NeighbourDiscovery::NDState;
  friend class IPv6NeighbourDiscovery::NDStateHost;
  friend class IPv6NeighbourDiscovery::NDStateRouter;
  friend class MobileIPv6::MIPv6NDStateHost;
  friend class MobileIPv6::MIPv6NDStateRouter;

  friend class XMLConfiguration::IPv6XMLParser;
  friend class XMLConfiguration::IPv6XMLWrapManager;
  friend class XMLConfiguration::XMLOmnetParser;

  Module_Class_Members(NeighbourDiscovery, cSimpleModule, 0);

  virtual void initialize(int stageNo);
  virtual void finish();
  virtual void handleMessage(cMessage* msg);
  virtual int numInitStages() const;
  IPv6NeighbourDiscovery::NDState* getRouterState();
  NotificationBoard* nb;

 private:
  InterfaceTable *ift;
  RoutingTable6 *rt;
  IPv6NeighbourDiscovery::NDState* nd;
  unsigned int ctrIcmp6OutRtrSol;
  unsigned int ctrIcmp6OutRtrAdv;
  unsigned int ctrIcmp6OutRedirect;
  unsigned int ctrIcmp6InRtrSol, ctrIcmp6InRtrAdv;
  unsigned int ctrIcmp6InNgbrSol, ctrIcmp6InNgbrAdv;
  unsigned int ctrIcmp6InRedirect;
};

#endif //__NEIGHBOURDISCOVERY_H
