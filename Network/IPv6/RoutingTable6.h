// -*- C++ -*-
// Copyright (C) 2001, 2003, 2004 CTIE, Monash University
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
 *  @file RoutingTable6.h
 *  @brief Read in the interfaces and routing table from file; Manage requests
 *  to the routing table and the interface table
 *
 *  @author  Eric Wu, Johnny Lai
 *
 *  @date   29/10/2001
 *  @test   See RoutingTableTest
 *  Based on RoutingTable module by Jochen Reber
 *
 */

#ifndef ROUTINGTABLE6_H
#define ROUTINGTABLE6_H

#include <cassert>
#include <string>
#include <vector>
#include <list>
#include <map>

#include <omnetpp.h>
#include "ipv6_addr.h"
#include "IPv6InterfaceData.h"


using IPv6NeighbourDiscovery::RouterEntry;
using IPv6NeighbourDiscovery::PrefixEntry;
using IPv6NeighbourDiscovery::NeighbourEntry;

class InterfaceTable;
class InterfaceEntry;
class cTimerMessage;

namespace IPv6NeighbourDiscovery
{
  class PrefixExpiryTmr;
  class RouterExpiryTmr;
  class IPv6CDS;
  class NDStateHost;
}

namespace XMLConfiguration
{
  class IPv6XMLManager;
  class IPv6XMLParser;
  class IPv6XMLWrapManager;
  class XMLOmnetParser;
}

#ifndef IPV6_ADDR_H
#include "ipv6_addr.h"
#endif //IPV6_ADDR_H

#ifdef USE_MOBILITY
namespace MobileIPv6
{
  class MIPv6CDS;
}
#endif //USE_MOBILITY

#ifndef EXPIRYENTRYLIST_H
#include "ExpiryEntryList.h"
#endif //EXPIRYENTRYLIST_H

/**
 * @class RoutingTable6
 * @brief contains the conceptual data structures mentioned in RFC2461
 *
 * Container class managing most of the data structures involved with routes and
 * interfaces.
 *
 */

class RoutingTable6: public cSimpleModule
{
#ifdef USE_CPPUNIT
  friend class RoutingTableTest;
#endif

  friend class XMLConfiguration::XMLOmnetParser;
  //@todo friend added to allow access to pel and rel.
  friend class IPv6NeighbourDiscovery::NDStateHost;

public:
  Module_Class_Members(RoutingTable6, cSimpleModule, 0);

  ///@name Redefined cSimpleModule funcs
  //@{
  virtual void initialize(int stage);
  virtual void finish();
  virtual void handleMessage(cMessage *);
  virtual int  numInitStages() const  {return 3;}
  //@}

  ///@name node attributes
  //@{
  /// returns the node id at the network level
  /// Helps to seed random number generators?
  int nodeId() const;

  /// Makes it easier to see which node is what during debug
  const char* nodeName() const;

  bool isEwuOutVectorHODelays() const { return ewuOutVectorHODelays; }

  bool isRouter() const { return IPForward; }

  ///Determines when packets with Site scope should be forwarded
  bool routeSitePackets() const { return forwardSitePacket; }

#ifdef USE_MOBILITY

  /// Does this node support Mobile IPv6 i.e. for Routers, HA, MNs Sec. 7
  bool mobilitySupport() const { return mipv6Support; }

  void setMobilitySupport(bool mipv6);

  /// Called by nodes with moblitySupport only
  bool isHomeAgent() const
    {
      assert(mobilitySupport());
      assert(!(role == HOME_AGENT && !isRouter()));
      return isRouter() && role == HOME_AGENT;
    }

  bool isMobileNode() const
    {
      assert(mobilitySupport());
      return role == MOBILE_NODE;
    }

  bool awayFromHome() const;

  void setLinkUpTime(simtime_t t) { linkUpTime = t; }
  simtime_t getLinkUpTime(void) { return linkUpTime; }

  void recordHODelay(simtime_t t) 
  { 
    handoverLatency->record( t - linkUpTime ); 
  }

#endif //USE_MOBILITY

  bool odad() const { return odadSupport; }
  void setODAD(bool o) { odadSupport = o; }

#ifdef USE_HMIP
  /// Support for HMIPv6 for Routers and Hosts
  bool hmipSupport() const { return hmipv6Support; }

  void setHmipSupport(bool hmip) { hmipv6Support = hmip; }

  void setMapSupport(bool map) { mapSupport = map; }

  bool isMAP()
    {
      return mapSupport;
    }
#endif //USE_HMIP

  //@}

  ///@name Routing functions
  //@{

  void addRoute(unsigned int ifIndex, IPv6Address& nextHop,
                const IPv6Address& dest, bool destIsHost=true);
  ///Return the src address for a packet going out on ifIndex to dest
  //ipv6_addr determineSrcAddress(const ipv6_addr& dest, size_t ifIndex);


  ////Determine if the address is a local one, ie for delivery to localhost
  bool localDeliver(const ipv6_addr& dest);

  //@}

  ///@name CDS functions
  //@{

  ///Associate the prefix with interface link on which it belongs
  void insertPrefixEntry(const PrefixEntry& pe, size_t ifIndex);
  ///Try not to use the cds->removePrefixEntry because entry needs to be remvoed
  ///from pel too perhaps
  void removePrefixEntry(PrefixEntry* ppe);
  /// insert router entry into the list
  void insertRouterEntry(RouterEntry* re, bool setDefault = false);
  ///Try not to use cds->removeRouterEntry because entry needs to be remvoed
  ///from rel too perhaps
  void removeRouterEntry(RouterEntry* re);
  IPv6NeighbourDiscovery::IPv6CDS* cds;

#ifdef USE_MOBILITY
  //convenience handle Don't delete
  MobileIPv6::MIPv6CDS* mipv6cds;
#endif //USE_MOBILITY

  //@}

  ///@name IGMPv6 functions
  //@{
  void joinMulticastGroup(const ipv6_addr& addr);

  void leaveMulticastGroup(const ipv6_addr& addr);
  //@}

  ///@name InterfaceEntry functions  XXX THESE SHOULD GO!!!!!!!!!!!! --AV
  //@{

  /// Assigns a tentative Address to interface at if_idx
  bool assignAddress(const IPv6Address& addr, unsigned int if_idx);
  /// Removes an assigned addr when lifetime has expired
  void removeAddress(const IPv6Address& addr, unsigned int ifIndex);
  /// Remove an assigned address
  void removeAddress(const ipv6_addr& addr, unsigned int ifIndex);
  /// Convenience function for checking if addr assigned
  bool addrAssigned(const ipv6_addr& addr, unsigned int ifIndex) const;
  //@}

  ///@name RoutingAlgorithm interface
  //@{

  void setForwardPackets(bool forward);
#ifdef USE_MOBILITY

  enum ROLE
  {
    CORRESPONDENT_NODE,
    MOBILE_NODE,
    HOME_AGENT
  };

  void setRole(ROLE _role) { role = _role; }

  ROLE getRole() const { return role; }

#endif //USE_MOBILITY
  //@}

  ///@name Address Configuration Lifetime Expiry
  //@{

  ///Update and elapse lifetimes when Router Advertisement updates prefix
  ///lifetimes
  void rescheduleAddrConfTimer(unsigned int minUpdatedLifetime);

  //Don't need this all the time as getting the shortest update time and the
  //countDown to current one will give the shortest lifetime anyway.  Only
  //needed for the first time trial i.e. when the addrExpiryTmr is first
  //scheduled.

  ///Return the shortest validLifetime of an assigned address for this node
  unsigned int minValidLifetime();

  unsigned int ctrIcmp6OutMsgs;

  bool displayIfconfig;

private:
  ///Add IPv6InterfaceData to given InterfaceEntry
  void configureInterfaceForIPv6(InterfaceEntry *ie);

  /// configure local loopback for IPv6
  void configureLoopbackForIPv6();

  ///Elapse all valid/preferredLifetimes of assigned addresses
  void elapseLifetimes(unsigned int seconds);

  ///Remove addresses that have -> 0 lifetime
  void invalidateAddresses();

  ///callback to remove addresses that have expired
  void lifetimeExpired();

  cTimerMessage* addrExpiryTmr;
  //@}

  ///IfConfig formatted output
  void print();

  ///Set of mulicast groups to which this node belongs to
  typedef std::vector<ipv6_addr> MulticastAddresses;
  MulticastAddresses multicastGroup;

  bool IPForward;

  bool forwardSitePacket;

private:

#ifdef USE_MOBILITY
  bool mipv6Support;

  bool ewuOutVectorHODelays;
  // handoverLatency is the L3 layer handover delay = time when
  // obtaining new CoA - link up time
  cOutVector* handoverLatency; 
  simtime_t linkUpTime; // time when establishing with new link

  /**
   * XML configured role for the current node.  It will eventually be a set of
   * flags as these are not mutually exclusive.  But for now just implement as
   * exclusive roles.
   */

  ROLE role;

  bool mapSupport;

  InterfaceTable *ift;

#endif //USE_MOBILITY

#ifdef USE_HMIP
  bool hmipv6Support;
#endif //USE_HMIP

  ///Optimistic DAD flag for node
  bool odadSupport;

  ///@name Prefix and Router Lifetime Management
  //@{

  ///Callback function for PrefixEntry lifetime expired
  void prefixTimeout();
  ///Callback function for RouterEntry lifetime expired
  void routerTimeout();

  typedef ExpiryEntryList<RouterEntry*, Loki::cTimerMessageCB<void> > REL;
  typedef ExpiryEntryList<PrefixEntry*, Loki::cTimerMessageCB<void> > PEL;

  REL* rel;
  PEL* pel;
  //@}
};


#endif //ROUTINGTABLE6_H

