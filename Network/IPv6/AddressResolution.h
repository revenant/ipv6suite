// -*- C++ -*-
//
// Copyright (C) 2001 Johnny Lai
// Monash University, Melbourne, Australia

/**
   @file AddressResolution.h

   @brief Perform Address resolution Store datagrams pending addr res
*/

#if !defined __ADDRESSRESOLUTION_H
#define __ADDRESSRESOLUTION_H

#ifndef MAP
#define MAP
#include <map>
#endif //MAP
#ifndef LIST
#define LIST
#include <list>
#endif //LIST

#ifndef IPv6_ADDR_H
#include "ipv6_addr.h"
#endif //IPv6_ADDR_H
#ifndef ROUTINGTABLE6ACCESS_H
#include "RoutingTable6Access.h"
#endif //ROUTINGTABLE6ACCESS_H
#ifndef NDSTATES_H
#include "NDStates.h"
#endif //NDSTATES_H


class IPv6Datagram;
class IPv6ForwardCore;

namespace IPv6NeighbourDiscovery
{
  class ICMPv6NDMNgbrAd;
  class ICMPv6NDMNgbrSol;
  class NDARTimer;
}

typedef std::multimap<ipv6_addr, IPv6Datagram* > PendingPacketQ;
typedef std::list<IPv6NeighbourDiscovery::NDARTimer*> NDARTimers;
typedef NDARTimers::iterator NDARTI;

/**
   @class AddressResolution
 */
class AddressResolution:public RoutingTable6Access
{

 public:
  Module_Class_Members(AddressResolution, RoutingTable6Access, 0);
  virtual void initialize();
  virtual void handleMessage(cMessage* msg);
  virtual void finish();

 private:
  ///Sec 7.2.5
  void processNgbrAd(IPv6NeighbourDiscovery::ICMPv6NDMNgbrAd* ngbrAdv);
  ///Sec. 7.2.3
  void processNgbrSol(IPv6NeighbourDiscovery::ICMPv6NDMNgbrSol* ngbrSol);
  ///Sec. 7.2.2
  void initiateNgbrSol(IPv6NeighbourDiscovery::NDARTimer* tmr);
  void sendNgbrSol(IPv6NeighbourDiscovery::NDARTimer* tmr);
  void failedAddrRes(IPv6NeighbourDiscovery::NDARTimer* tmr);
  ///Sec. 7.2.4 Perhaps do that here?
  void sendNgbrAd();



  void resendNS();
  void timedOutNS();

  ///Pending Packet Queue for packets awaiting addr res
  PendingPacketQ ppq;
  NDARTimers tmrs;
  IPv6ForwardCore* fc;

  unsigned int ctrIcmp6OutNgbrAdv;
  unsigned int ctrIcmp6OutNgbrSol;

  cModule** outputMod;
  static unsigned int outputUnicastGate;
};

#endif //__ADDRESSRESOLUTION_H
