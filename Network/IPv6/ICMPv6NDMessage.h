// -*- C++ -*-
// $Header: /home/cvs/IPv6Suite/IPv6SuiteWithINET/Network/IPv6/Attic/ICMPv6NDMessage.h,v 1.2 2005/02/10 05:59:32 andras Exp $
//
// Copyright (C) 2001 CTIE, Monash University
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
    @file ICMPv6NDMessage.h

    @brief Description of the Neighbour Discovery (ND) ICMP messages and their
    options.

    @author Johnny Lai
    @date 14.9.01

*/

#if !defined ICMPV6NDMESSAGE_H
#define ICMPV6NDMESSAGE_H

#ifndef ICMPV6NDMESSAGEBASE_H
#include "ICMPv6NDMessageBase.h"
#endif //ICMPV6NDMESSAGEBASE_H

#ifndef STRING
#define STRING
#include <string>
#endif //STRING

#ifndef IPV6_ADDR_H
#include "ipv6_addr.h"
#endif //IPV6_ADDR_H
#ifndef NDENTRY_H
#include "NDEntry.h" //class AdvPrefixList;
#endif //NDENTRY_H

#ifdef USE_HMIP
#ifndef HMIPV6ICMPV6NDMESSAGE_H
#include "HMIPv6ICMPv6NDMessage.h"
#endif //HMIPV6ICMPV6NDMESSAGE_H
#endif //USE_HMIP


class IPv6Datagram;


namespace IPv6NeighbourDiscovery
{

extern const short NDHOPLIMIT;
extern const int ISROUTER_MASK;
extern const int SOLICITED_MASK;
extern const int OVERRIDE_MASK;
extern const int MANAGED_MASK;
extern const int OTHER_MASK;

#ifdef USE_MOBILITY
extern const int HOMEAGENT_MASK;
#endif

/**
 * @defgroup ICMPv6NDMsgs ICMPv6 Neighbour Discovery Messages
 * @{
 */

/**
   @class ICMPv6NDMRtrSol
   @brief Router Solicitation message
 */
//The n_addrs template arg should be 0 the 1 is to allow compilation
class ICMPv6NDMRtrSol: public ICMPv6NDMsgBseRtrSol
{
public:

  ICMPv6NDMRtrSol();
  ICMPv6NDMRtrSol(const ICMPv6NDMRtrSol& src);
  const ICMPv6NDMRtrSol& operator=(const ICMPv6NDMRtrSol& src);
  bool operator==(const ICMPv6NDMRtrSol& rhs) const;

  virtual ICMPv6NDMRtrSol* dup() const { return new ICMPv6NDMRtrSol(*this); }

  ///@name Neighbour options
  //@{
  bool hasSrcLLAddr() const { return hasLLAddr(0); }

  string srcLLAddr() const { return LLAddress(); }
  void setSrcLLAddr(const string& addr, int len = 1)
    {
      setLLAddress(true, addr, len);
    }
  //@}
private:

};

/**
 * @typedef PrefixesInfo
 * @ingroup ICMPv6NDOptions
 *
 */
typedef std::vector<ICMPv6NDOptPrefix> PrefixesInfo;

/**
 *  @class ICMPv6NDMRtrAd
 *  @brief Base class for Router Advertisement message
 */

class ICMPv6NDMRtrAd: public ICMPv6NDMsgBaseRtrAd
{
public:

  ///Not doing stateful autoconf so default for managed and other are false
  ICMPv6NDMRtrAd(int lifetime, int hopLimit, unsigned int reach,
                       unsigned int retrans,
                       const AdvPrefixList& prefixList,
                       bool managed = false, bool other = false);

  ICMPv6NDMRtrAd(int lifetime, int hopLimit = 0, unsigned int reach = 0,
                 unsigned int retrans = 0,
                 bool managed = false, bool other = false);
  ICMPv6NDMRtrAd(const ICMPv6NDMRtrAd& src);
  const ICMPv6NDMRtrAd& operator=(const ICMPv6NDMRtrAd& src);
  bool operator==(const ICMPv6NDMRtrAd& rhs) const;

  virtual ICMPv6NDMRtrAd* dup() const { return new ICMPv6NDMRtrAd(*this); }

  ///@name ICMP fields
  //@{
  int curHopLimit() const { return optInfo()>>24; }
  void setCurHopLimit(int hopLimit) { setOptInfo(optInfo() | (hopLimit<<24));}

  bool managed() const { return optInfo() & MANAGED_MASK; }
  void setManaged(bool managed)
    {
      if (managed)
        setOptInfo(optInfo() | MANAGED_MASK);
      else
        setOptInfo(optInfo() & ~MANAGED_MASK);
    }

  bool other() const { return optInfo() & OTHER_MASK; }
  void setOther(bool other)
    {
      if (other)
        setOptInfo(optInfo() | OTHER_MASK);
      else
        setOptInfo(optInfo() & ~OTHER_MASK);
    }

  int routerLifetime() const { return optInfo() & 0xFFFF; }
  void setRouterLifetime(int routerLife) { setOptInfo(optInfo() | routerLife); }

#ifdef USE_MOBILITY
  bool isHomeAgent(void) const { return optInfo() & HOMEAGENT_MASK; }
#endif

  ///Reuses the storage of the first ipv6Addr
  unsigned int reachableTime() const { return address(0).low; }
  ///Reuses the storage of the first ipv6Addr
  unsigned int retransTimer() const { return address(0).normal; }

  ///If these functions affect performance then perhaps have address() return a referencde
  ///for direct modification
  void setReachableTime(unsigned int reachTime)
    {
      ipv6_addr addr = address(0);
      addr.low = reachTime;
      setAddress(addr, 0);
    }
  void setRetransTimer(unsigned int retrans)
    {
      ipv6_addr addr = address(0);
      addr.normal = retrans;
      setAddress(addr, 0);
    }
  //@}

  ///@name Neighbour Options
  //@{
  ///Prefix SHOULD be included
  const ICMPv6NDOptPrefix& prefixInfo(int i) const
    {
      return prefixes[i];
    }

  void setPrefixInfo(const ICMPv6NDOptPrefix& pref, size_t i)
    {
      prefixes[i] = pref;
    }

  void setPrefixesInfo(const AdvPrefixList& pe);
  const PrefixesInfo& prefixesInfo() const { return prefixes; }

  size_t prefixCount() const { return prefixes.size(); }

  void setMTU(unsigned int mtu)
    {
      bool changeLength = true;
      if (opts[1])
        changeLength = false;

      delete opts[1];

      opts[1] = new ICMPv6NDOptMTU(mtu);
      if (changeLength)
        setLength(length() + opts[1]->length());
    }
  unsigned int MTU() const
    {
      if (opts[1])
        return (static_cast<ICMPv6NDOptMTU*> (opts[1]))->MTU();
      return 0;
    }

#ifdef USE_MOBILITY
  void setAdvInterval(unsigned long advInt)
    {
      bool changeLength = true;
      if (opts[2])
      {
        changeLength = false;
        static_cast<ICMPv6NDOptAdvInt*>(opts[2])->advInterval = advInt;
      }
      else
        opts[2] = new ICMPv6NDOptAdvInt(advInt);

      if (changeLength)
        setLength(length() + opts[2]->length());
    }

  unsigned long advInterval(void)
    {
      if (opts[2])
        return (static_cast<ICMPv6NDOptAdvInt*>(opts[2]))->advInterval;
      return 0;
    }
#endif

#ifdef USE_HMIP

  bool hasMapOptions() const { return !mapOpts.empty(); };

  void addOption(const HierarchicalMIPv6::HMIPv6ICMPv6NDOptMAP& mapOpt);

  void setOptions(const HierarchicalMIPv6::MAPOptions& opts);

  const HierarchicalMIPv6::MAPOptions& mapOptions() const { return mapOpts; }

#endif //USE_HMIP

  bool hasSrcLLAddr() const { return hasLLAddr(0); }

  ///Option MAY be ommitted to allow loadsharing RFC2461 Sec 4.2
  string srcLLAddr() const
    {
      return LLAddress(0);
    }
  void setSrcLLAddr(const string& addr, int len = 1)
    {
      setLLAddress(true, addr, len, 0);
    }
  //@}
protected:
  PrefixesInfo prefixes;

#ifdef USE_HMIP
  //std::auto_ptr<HierarchicalMIPv6::MAPOptions> mapOpts;
  HierarchicalMIPv6::MAPOptions mapOpts;
#endif //USE_HMIP
};

/**
   @class ICMPv6NDMNgbrSol
   @brief Neighbour Solicitation message
 */
class ICMPv6NDMNgbrSol: public ICMPv6NDMsgBaseNgbrSol
{
public:
  ICMPv6NDMNgbrSol(const ipv6_addr& targetAddr = IPv6_ADDR_UNSPECIFIED, const string& srcLLAddr = "");
  ICMPv6NDMNgbrSol(const ICMPv6NDMNgbrSol& src);
  const ICMPv6NDMNgbrSol& operator=(const ICMPv6NDMNgbrSol& src);
  bool operator==(const ICMPv6NDMNgbrSol& rhs) const;

  virtual ICMPv6NDMNgbrSol* dup() const { return new ICMPv6NDMNgbrSol(*this); }

  virtual size_t optionCount() const { return srcLLAddr() != ""?0:1; }

  ///@name Neighbour options
  //@{
  bool hasSrcLLAddr() const { return hasLLAddr(0); }

  string srcLLAddr() const { return LLAddress(0); }
  void setSrcLLAddr(const string& addr, int len = 1)
    {
      setLLAddress(true, addr, len);
    }
  //@}

  //@name ICMP fields
  //@{
  void setTargetAddr(const ipv6_addr target) { setAddress(target, 0); }
  ipv6_addr targetAddr() { return address(0); }
  //@}
};


/**
   @class ICMPv6NDMNgbrAd
   @brief Neighbour Advertisement message
 */
class ICMPv6NDMNgbrAd: public ICMPv6NDMsgBaseNgbrAd
{
public:
  ICMPv6NDMNgbrAd(const ipv6_addr& targetAddr = IPv6_ADDR_UNSPECIFIED, const string& addr = "", bool isRouter = false, bool solicited = false, bool override = true );
  ICMPv6NDMNgbrAd(const ICMPv6NDMNgbrAd& src);
  const ICMPv6NDMNgbrAd& operator=(const ICMPv6NDMNgbrAd& src);
  bool operator==(const ICMPv6NDMNgbrAd& rhs) const;

  std::ostream& operator<<(std::ostream& os) const;

  virtual ICMPv6NDMNgbrAd* dup() const { return new ICMPv6NDMNgbrAd(*this); }

  ///@name ICMP fields
  //@{
  void setTargetAddr(const ipv6_addr target) { setAddress(target, 0); }
  ipv6_addr targetAddr() { return address(0); }

  bool isRouter() const
    {
      return (optInfo() & ISROUTER_MASK)?true:false;
    }
  bool solicited() const
    {
      return (optInfo() & SOLICITED_MASK)?true:false;
    }
  bool override() const
    {
      return (optInfo() & OVERRIDE_MASK)?true:false;
    }
  void setIsRouter(bool isRouter)
    {
      if (isRouter)
        setOptInfo(optInfo() | ISROUTER_MASK);
      else
        setOptInfo(optInfo() & ~ISROUTER_MASK);
    }
  void setSolicited(bool solicited)
    {
      if (solicited)
        setOptInfo(optInfo() | SOLICITED_MASK);
      else
        setOptInfo(optInfo() & ~SOLICITED_MASK);

    }
  void setOverride(bool override)
    {
       if (override)
        setOptInfo(optInfo() | OVERRIDE_MASK);
      else
        setOptInfo(optInfo() & ~OVERRIDE_MASK);
    }

  void setFlags(bool isRouter, bool solicited, bool override)
    {
      setIsRouter(isRouter);
      setSolicited(solicited);
      setOverride(override);
    }
  //@}

  ///@name Neighbour options
  //@{
  bool hasTargetLLAddr() const { return hasLLAddr(0); }

  ///Option MUST exist for mulicast solicitation or SHOULD if unicast
  string targetLLAddr() const
    {
      return LLAddress(0);
    }
  void setTargetLLAddr(const string& addr, int len = 1)
    {
      setLLAddress(false, addr, len, 0);
    }
  //@}
};

inline std::ostream& operator<<(std::ostream& os, const ICMPv6NDMNgbrAd& na)
{
  return na.operator<<(os);
}

/**
   @class ICMPv6NDMRedirect
   @brief Redirect message
 */
class ICMPv6NDMRedirect: public ICMPv6NDMsgBaseRedirect
{
public:
  ICMPv6NDMRedirect(const ipv6_addr& dest, const ipv6_addr& target,
                    IPv6Datagram* dgram, string targetLinkAddr = "");

  ICMPv6NDMRedirect(const ICMPv6NDMRedirect& src);
  const ICMPv6NDMRedirect& operator=(const ICMPv6NDMRedirect& src);
  bool operator==(const ICMPv6NDMRedirect& rhs) const;

  virtual ICMPv6NDMRedirect* dup() const { return new ICMPv6NDMRedirect(*this); }

  ///@name ICMP fields
  //@{
  void setTargetAddr(const ipv6_addr& target) { setAddress(target, 0); }
  ipv6_addr targetAddr() const { return address(0); }

  void setDestAddr(const ipv6_addr dest) { setAddress(dest, 1); }
  ipv6_addr destAddr() const { return address(1); }
  //@}

  ///@name Redirect Options
  //@{
  void attachHeader(IPv6Datagram* dgram);
  const IPv6Datagram* header() const;
  IPv6Datagram* detachHeader();

  bool hasTargetLLAddr() const { return hasLLAddr(1); }
  string targetLLAddr() const { return LLAddress(1); }
  void setTargetLLAddr(const string& addr, int len = 1)
    {
      setLLAddress(false, addr, len, 1);
    }
  //@}
};
//@}

} // IPv6NeighbourDiscovery

#endif //ICMPV6NDMESSAGE_H
