// -*- C++ -*-
// Copyright (C) 2001, 2003, 2004, 2005 CTIE, Monash University 
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
   @file NDEntry.h
   @brief Neighbour Discovery Entries - Prefix, Neighbour and Router
   @see RFC2461 Sec 5
   @author Johnny Lai
   @date 19.9.01

*/

#ifndef NDENTRY_H
#define NDENTRY_H

#ifndef STRING
#define STRING
#include <string>
#endif //STRING

#ifndef IOSFWD
#define IOSFWD
#include <iosfwd>
#endif //IOSFWD

#ifndef BOOST_WEAK_PTR_HPP_INCLUDED
#include <boost/weak_ptr.hpp>
#endif //BOOST_WEAK_PTR_HPP_INCLUDED

#ifndef IPv6ADDRESS_H
#include "IPv6Address.h"
#endif //IPv6ADDRESS_H

extern const unsigned int VALID_LIFETIME_DEFAULT ;  
extern const unsigned int VALID_PREFLIFETIME_DEFAULT;

namespace XMLConfiguration
{  
  class IPv6XMLParser;
  class IPv6XMLWrapManager;
  class XMLOmnetParser;
}

namespace IPv6NeighbourDiscovery
{

  class ICMPv6NDOptPrefix;
  class PrefixEntry;
  class PrefixExpiryTmr;

/**
 * @defgroup IPv6NDCDS Neighbour Discovery Conceptual Data Structures  
 * @{
*/


/**
   @class PrefixEntry

   @brief Represents an entry in the Prefix List as described in RFC2461 Sec
   5.1 
 */

class PrefixEntry
{
public:
  friend class XMLConfiguration::IPv6XMLParser;
  friend class XMLConfiguration::IPv6XMLWrapManager;
  friend class XMLConfiguration::XMLOmnetParser;

  friend bool operator<(const PrefixEntry& lhs, const PrefixEntry& rhs);

  explicit PrefixEntry(const IPv6Address& addr = IPv6Address(IPv6_ADDR_UNSPECIFIED),
              unsigned int lifetime = VALID_LIFETIME_DEFAULT);
  
  PrefixEntry(const ipv6_addr& oprefix, unsigned int prefixLength, 
              unsigned int lifetime = VALID_LIFETIME_DEFAULT);

  PrefixEntry(const ICMPv6NDOptPrefix& prefOpt);

  ~PrefixEntry();
  
  ///Expiry func called to destroy the prefix entry when router lifetime expires
  //void prefixTimeout(PrefixExpiryTmr* tmr);

  ///Equality is determined by the prefix alone
  bool operator==(const PrefixEntry& rhs)
    {
      return (_prefix.prefixLength() == rhs._prefix.prefixLength() &&
              static_cast<ipv6_addr> (_prefix) == 
              static_cast<ipv6_addr> (rhs._prefix));
    }

  ///@name Attributes
  //@{
  const IPv6Address& prefix() const { return _prefix; }

  void setPrefix(const IPv6Address& pref, bool truncate = true) 
    { 
      _prefix = pref;
      if (truncate)
        _prefix.truncate();
    }

  void setPrefix(const char* pref, bool truncate = true) 
    { 
      _prefix.setAddress(pref);
      if (truncate)
        _prefix.truncate();
    }
  
  unsigned int advValidLifetime() const;
  void setAdvValidLifetime(unsigned int validLifetime);

#ifdef USE_MOBILITY
  bool advRtrAddr() const { return _advRtrAddr; }
  void setAdvRtrAddr(bool advRtrAddr) { _advRtrAddr = advRtrAddr; }
#endif
  
  bool advOnLink() const { return _advOnLink; }
  void setAdvOnLink(bool onLink) { _advOnLink = onLink; }
  
  
  bool realtime() const { return _realtime; }
  void setRealtime(bool real = false) { _realtime = real; }

  unsigned int advPrefLifetime() const { return _advPrefLifetime; }
  void setAdvPrefLifetime(unsigned int preflt) { _advPrefLifetime = preflt; }
  
  bool advAutoFlag() const { return _advAutoFlag; }
  void setAdvAutoFlag(bool autoFlag) { _advAutoFlag = autoFlag; }
  
//  void setTimer(PrefixExpiryTmr* otmr) { tmr = otmr; }
//  PrefixExpiryTmr* timer() { return tmr; }
  
  unsigned int expiryTime() const { return advValidLifetime(); }
  //@}

private:
  
  IPv6Address _prefix;

  unsigned int _advValidLifetime;

  ///For now just do constant. Refer to 6.2.1 AdvValidLifetime
  bool _realtime;

  ///For now assume all conforming addresses are onlink  
  ///"L-bit" field in Prefix Information of Router Adv.
  bool _advOnLink;      
  
  ///@name Autoconf variables
  //@{
  unsigned int _advPrefLifetime; //default is 605800 (7days)
  bool _advAutoFlag;  //default true refer to autoconf

#ifdef USE_MOBILITY
  bool _advRtrAddr;
#endif

  //@}

//  PrefixExpiryTmr* tmr;
};

  bool operator<(const PrefixEntry& lhs, const PrefixEntry& rhs);
  std::ostream& operator<<(std::ostream& os, const PrefixEntry& dat);



  class ICMPv6NDMNgbrAd;
  class ICMPv6NDMNgbrSol;
  class ICMPv6NDMRedirect;


/**
   @class  NeighbourEntry
   @brief An entry in the Neighbour Cache

   May contain reachability state, no. of unanswered probes and 
   time the next Neighbour Unreachability event is scheduled.
 */
class NeighbourEntry
{
public:
  
  enum ReachabilityState
  {
    INCOMPLETE, REACHABLE, STALE, DELAY, PROBE
  };
    
  NeighbourEntry(const ipv6_addr& addr = IPv6_ADDR_UNSPECIFIED, 
                 size_t ifaceNo = 0, const char* destLinkAddr = 0,
                 ReachabilityState state = INCOMPLETE);  
  
  NeighbourEntry(const IPv6NeighbourDiscovery::ICMPv6NDMNgbrSol* ns);
  
  NeighbourEntry(const IPv6NeighbourDiscovery::ICMPv6NDMRedirect* redirect);
  
  bool update(const IPv6NeighbourDiscovery::ICMPv6NDMNgbrAd* na);

  void update(const IPv6NeighbourDiscovery::ICMPv6NDMNgbrSol* ns);

  void update(const IPv6NeighbourDiscovery::ICMPv6NDMRedirect* redirect);
  
  ///@name Attributes
  //@{
  bool isRouter() const { return _isRouter; }

  void setIsRouter(bool is) { _isRouter = is; }
    
  string linkLayerAddr() const { return _LLaddress; }
  void setLLAddr(const string& addr) { _LLaddress = addr; }

  size_t ifIndex() const { return interfaceNo; }
  void setIfIndex(size_t idx) { interfaceNo = idx; }

  const ipv6_addr& addr() const { return unicastAddr;  }

  ReachabilityState state() const { return _state; }
  void setState( ReachabilityState state) { _state = state; }
  
  //@}
 
protected:

  ipv6_addr unicastAddr;
  string _LLaddress;  
  size_t interfaceNo;
  bool _isRouter;
  //Unreachability Detection Algorithm
  ReachabilityState _state;
  
};

  std::ostream& operator<<(std::ostream& os, const NeighbourEntry& ne);

//  class RouterExpiryTmr;

/**
   @class RouterEntry

   @brief An entry in the Default routers list or Neighbour Cache that
   represents a router.  

   @note Not all routers are in default Routers list.
 */

class RouterEntry: public NeighbourEntry
{
 public:
  RouterEntry(const ipv6_addr& addr = IPv6_ADDR_UNSPECIFIED,
              size_t ifaceNo = 0, const char* linkAddr = "",
              unsigned int lifetime =  VALID_LIFETIME_INFINITY);
  
  RouterEntry(const ICMPv6NDMRedirect* redirect);

  ~RouterEntry();
  
  ///Returns remaining lifetime of entry
  unsigned int invalidTime() const;
  ///Required by ExpiryEntryList<Entry,Timer>
  unsigned int expiryTime() const { return invalidTime(); }

  ///Assigns a new lifetime to entry
  void setInvalidTmr(unsigned int newLifetime);

public:
  simtime_t lastRAReceived;

  //Unused
  //void update(IPv6NeighbourDiscovery::ICMPv6NDMRtrAd* ra);

  ///Expiry func called to rmove the router entry when router lifetime expires
//  void routerExpired(RouterExpiryTmr* re);

//  void setTimer(RouterExpiryTmr* otmr) { tmr = otmr; }
//  RouterExpiryTmr* timer() { return tmr; }
 private:

  ///can't be removed yet as the ctor requires a place to store the initial
  ///invalidlifetime before tmr is created by insertRouterEntry.
  unsigned int _invalidTime;  
//  RouterExpiryTmr* tmr;
};

  std::ostream& operator<<(std::ostream& os, const RouterEntry& re);

/**
 * @struct DestinationEntry
 * @brief An entry in the Destination Cache
 *
 * Follows the conceptual data structures proposed in RFC2461.
 * 
 * @warning Shares NeighbourEntry pointers i.e. there is a one to many
 * relationship between DestinationEntry and NeighbourEntry.  To ensure seg
 * faults don't occur and correct memory leaks reimplement neigbhour as
 * boost::shared_pointer.
 */
struct DestinationEntry
{
  DestinationEntry(boost::weak_ptr<NeighbourEntry> ngbr = boost::weak_ptr<NeighbourEntry>(), size_t mtu = 0)
    :neighbour(ngbr), pathMTU(mtu) {}  
  boost::weak_ptr<NeighbourEntry> neighbour;
  size_t pathMTU; 
  //int roundTrip timers
};

std::ostream& operator<<(std::ostream& os, const DestinationEntry& re);

//@}

} //namespace IPv6NeighbourDiscovery

#include <vector> 
///Needed by both RoutingTable6, Interface6Entry and ICMPv6NDMessage
typedef std::vector<IPv6NeighbourDiscovery::PrefixEntry> AdvPrefixList;
typedef std::vector<IPv6NeighbourDiscovery::PrefixEntry*> LinkPrefixes;
#endif //NDENTRY_H
