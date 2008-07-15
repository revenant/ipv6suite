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
 *  @file IPv6InterfaceData.h
 *
 *  @brief Generic network Interface similar to ifconfig information.
 *  Interfaces with the real network interface.
 *
 *  @author  Johnny Lai
 *
 *  @date 2/03/2002
 *
 */

#ifndef __IPv6INTERFACEDATA_H
#define __IPv6INTERFACEDATA_H


#include <string>
#include "ipv6_addr.h"
#include "NDEntry.h"
#include "IPv6Headers.h"


using std::string;

///incomplete type in gdb due to definition in class scope (gdb bug)
typedef std::vector<IPv6Address> IPv6Addresses;

/**
 * @defgroup NDConstants Neighbour Discovery Protocol constants
 *
 */
//@{
extern const double DEFAULT_MAX_RTR_ADV_INT;


#ifdef USE_MOBILITY
extern const double MIN_MIPV6_MAX_RTR_ADV_INT;
extern const double MIN_MIPV6_MIN_RTR_ADV_INT;
#endif

///Diameter of the Internet at time of implementation
extern const int DEFAULT_ADVCURHOPLIMIT;
extern const double MAX_RTR_SOLICITATION_DELAY;
extern const int REACHABLE_TIME;
extern const int RETRANS_TIMER;

extern const double MIN_RANDOM_FACTOR;
extern const double MAX_RANDOM_FACTOR;

//@}

class LinkLayerModule;

/**
 * IPv6-specific data for InterfaceEntry
 */
class IPv6InterfaceData : public cPolymorphic
{
public:
  IPv6InterfaceData();
  virtual ~IPv6InterfaceData() {}

  std::string info() const;  //displayed in Tkenv
  std::string detailedInfo() const;  //displayed in Tkenv

  ////Let the compiler generate these we want memberwise copy of members.
  ///There are no pointers so this is safe.
  //IPv6InterfaceData(const IPv6InterfaceData& obj);
  //IPv6InterfaceData& operator=(const IPv6InterfaceData& obj);

  void print(bool);

private:
  static void removeAddrFromArray(IPv6Addresses& addrs, const IPv6Address& addr);

public:
  void removeAddress(const IPv6Address& addr) {
    assert(addrAssigned(addr));
    removeAddrFromArray(inetAddrs,addr);
  }
  void removeTentativeAddress(const IPv6Address& addr) {
    removeAddrFromArray(tentativeAddrs,addr);
  }

  bool addrAssigned(const ipv6_addr& addr) const;
  bool tentativeAddrAssigned(const ipv6_addr& addr) const;

  ///Elapse all valid/preferredLifetimes of assigned addresses
  void elapseLifetimes(unsigned int seconds);

  ///Return the shortest validLifetime assigned to an address on this iface
  unsigned int minValidLifetime();

  ///Return an assigned or tentative address that matches the specified prefix
  IPv6Address matchPrefix(const ipv6_addr& prefix, unsigned int prefixLength,
                          bool tentative = false);

public:
  // XXX naked public data members here MUST BE WRAPPED INTO GET/SET FUNCTIONS!!!!!! --AV

  // Type of network interface (Ethernet, Point to Point)  XXX OUT!!!!! --AV
  const char* encap(void);

  /**
   * @todo Change to use shared_ptrs of ipv6_prefix (once that has an interface
   * resembling IPv6Address) and thus STL list container.  This enables other
   * protocols that need to know when an addr assigned to the node is still
   * valid by maintaining a weak_ptr to it.  For now when an addr lifetime
   * expires we will have to set all other pointers to null manually which is
   * tedioius and error prone.
   *
   */

  IPv6Addresses inetAddrs;

  /// tentative address for duplicate address detection
  /// This data structure serves 4 purposes 1. Store manually configured address
  /// parsed from XML network configuration. 2. ND uses it to determine which
  /// addresses are tentative during Neighbour Discovery and DAD procedure
  /// 3. DAD started on addresses inside here right after link local addr is
  /// assigned ( detectDupOther) only once 4. Subsequent tentative addr derived
  /// from router prefixes are stored here but DAD is initiated for them from
  /// elsewhere.  (This is to prevent DAD from recurring on addresses that are
  /// currently undergoing DAD).
  IPv6Addresses tentativeAddrs;

  unsigned int baseReachableTime() const
  {
    return _baseReachableTime;
  }

  ///@name Node Configuration Variables
  //@{
  short curHopLimit;
  unsigned int retransTimer;
  
#if FASTRS
  double maxRtrSolDelay;
#endif // FASTRS
  void setBaseReachableTime(unsigned int base);

  double reachableTime();

  // number of consecutive NS messages to be sent
  int dupAddrDetectTrans;

  ///Convenience handle to the link layer module
#ifdef USE_MOBILITY
  struct mipv6Variables
  {
    mipv6Variables()
      :minRtrSolInterval(1), maxInterval(2), maxConsecutiveMissedRtrAdv(1),
       L2ConnectionStatus(false), routerLinkAddrMonitor(false)
      {}
    ///in seconds
    int minRtrSolInterval;
    /// in seconds
    int maxInterval;
    ///Trigger RS when this condition met (max means disabled)
    unsigned int maxConsecutiveMissedRtrAdv;
    ///Enable layer two hooks of when interface connects
    bool L2ConnectionStatus;
    ///Monitor at layer two for packets forwarded/sent from router
    bool routerLinkAddrMonitor;
  } mipv6Var;
#endif //USE_MOBILITY

  //IPv6 router constants
  struct RouterVariables
    {
      RouterVariables():advSendAds(false),
                        maxRtrAdvInt(DEFAULT_MAX_RTR_ADV_INT),
                        minRtrAdvInt(0.33*maxRtrAdvInt),
                        advManaged(false), advOther(false),
                        advLinkMTU(IPv6_MIN_MTU), advReachableTime(0),
                        advRetransTmr(0),
                        advCurHopLimit(DEFAULT_ADVCURHOPLIMIT),
                        advDefaultLifetime(3*maxRtrAdvInt
#if USE_MOBILITY
                                       <1?1:3*maxRtrAdvInt
#endif //USE_MOBILITY
                                       )
#ifdef USE_MOBILITY
                        ,advHomeAgent(false)
#endif
#if FASTRA
                        ,fastRA(false),
                        maxFastRAS(10),
                        fastRACounter(0)
#endif //FASTRA
        {}

      void setReachableTime(unsigned int t)
        {
          advReachableTime = t > 3600000?advReachableTime:t;
        }


      bool advSendAds;
      simtime_t maxRtrAdvInt; //seconds
      simtime_t minRtrAdvInt;  //seconds
      bool advManaged; //False as we are not doing stateful autoconfiguration
      bool advOther; //Also false as not diseminating other config info from routers
      int advLinkMTU; //Host LinkMTU
      int advReachableTime; //Host BaseReachableTime?
      int advRetransTmr; //Host RetransTimer
      int advCurHopLimit; //Host CurHopLimit
      ///Integer seconds in ND rfc but for MIPv6 really need better granularity
      simtime_t advDefaultLifetime;

      ///These are either learned from routers or configured manually(router)
      AdvPrefixList advPrefixList;

#ifdef USE_MOBILITY
      bool advHomeAgent; // true if an interface supports home agent functionality
#endif
#if FASTRA
      bool fastRA;
      unsigned int maxFastRAS;
      mutable unsigned int fastRACounter;
#endif //FASTRA
    } rtrVar;
  //@}
private:
  unsigned int _baseReachableTime;

  double _reachableTime;
  string llAddr; //--> XXX to InterfaceEntry!
  unsigned int _interfaceID[2];
};

#endif //INTERFACE6ENTRY_H

