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

#include <map>
#include <list>

#include "ipv6_addr.h"
#include "NDStates.h"


class IPv6Datagram;
class IPv6Forward;
class RoutingTable6;
class InterfaceTable;


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
class AddressResolution : public cSimpleModule
{

 public:
  Module_Class_Members(AddressResolution, cSimpleModule, 0);
  virtual int numInitStages() const {return 2;}
  virtual void initialize(int stage);
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

  InterfaceTable *ift;
  RoutingTable6 *rt;

  ///Pending Packet Queue for packets awaiting addr res
  PendingPacketQ ppq;
  NDARTimers tmrs;
  IPv6Forward* fc;

  unsigned int ctrIcmp6OutNgbrAdv;
  unsigned int ctrIcmp6OutNgbrSol;

  cModule** outputMod;
  static unsigned int outputUnicastGate;
};

#endif //__ADDRESSRESOLUTION_H
