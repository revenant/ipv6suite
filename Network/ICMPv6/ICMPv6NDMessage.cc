//
// Copyright (C) 2001 CTIE, Monash University
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
    @file ICMPv6NDMessage.cc
    @brief Implementations of Neighbour Discovery
             Message classes and their options.

    @author Johnny Lai
    @date 14.9.01
*/

#include "ICMPv6Message_m.h"
#include "ICMPv6NDMessage.h"
#include "IPv6Datagram.h"
#include "NDEntry.h"
#include "ipv6_addr.h"
#include <algorithm> //for_each

#if !defined __ICMPv6NDMESSAGE_CC
#define __ICMPv6NDMESSAGE_CC

using namespace IPv6NeighbourDiscovery;

const short IPv6NeighbourDiscovery::NDHOPLIMIT = 255;
const int IPv6NeighbourDiscovery::ISROUTER_MASK = 0x80000000;
const int IPv6NeighbourDiscovery::SOLICITED_MASK = 0x40000000;
const int IPv6NeighbourDiscovery::OVERRIDE_MASK = 0x20000000;
const int IPv6NeighbourDiscovery::MANAGED_MASK = 0x800000;
const int IPv6NeighbourDiscovery::OTHER_MASK = 0x400000;

#ifdef USE_MOBILITY
const int IPv6NeighbourDiscovery::HOMEAGENT_MASK = 0x100000;
#endif


ICMPv6NDMRtrAd::ICMPv6NDMRtrAd(int lifetime, int hopLimit, unsigned int reach,
                               unsigned int retrans,
                               const AdvPrefixList& prefixList,
                               bool managed, bool other)
  :ICMPv6NDMsgBaseRtrAd(ICMPv6_ROUTER_AD)
{
  setName("RA");

  setCurHopLimit(hopLimit);
  setManaged(managed);
  setOther(other);
  setRouterLifetime(lifetime);
  setReachableTime(reach);
  setRetransTimer(retrans);
  setPrefixesInfo(prefixList);

  size_t len = length();
  for (size_t i = 0; i < prefixes.size(); i++)
    len += prefixes[i].lengthInUnits()*IPv6_EXT_UNIT_OCTETS*BITS;
  setLength(len);
}

ICMPv6NDMRtrAd::ICMPv6NDMRtrAd(int lifetime, int hopLimit, unsigned int reach,
                               unsigned int retrans, bool managed, bool other)
  :ICMPv6NDMsgBaseRtrAd(ICMPv6_ROUTER_AD)
{
  setName("RA");

  setCurHopLimit(hopLimit);
  setManaged(managed);
  setOther(other);
  setRouterLifetime(lifetime);
  setReachableTime(reach);
  setRetransTimer(retrans);
}

ICMPv6NDMRtrAd::ICMPv6NDMRtrAd(const ICMPv6NDMRtrAd& src)
  :ICMPv6NDMsgBaseRtrAd(ICMPv6_ROUTER_AD)
{
  setName(src.name());
  operator=(src);
}

const ICMPv6NDMRtrAd& ICMPv6NDMRtrAd::operator=(const ICMPv6NDMRtrAd& src)
{
  if (this != &src)
  {
    ICMPv6NDMsgBaseRtrAd::operator=(src);
    prefixes = src.prefixes;

#ifdef USE_HMIP
    mapOpts = src.mapOpts;
#endif //USE_HMIP
  }

  return *this;
}

bool ICMPv6NDMRtrAd::operator==(const ICMPv6NDMRtrAd& rhs) const
{
  if (this == &rhs)
    return true;

  if (!ICMPv6NDMsgBaseRtrAd::operator==(rhs))
    return false;
  if (curHopLimit() == rhs.curHopLimit() && managed() == rhs.managed() &&
      other() == rhs.other() && routerLifetime() == rhs.routerLifetime() &&
      reachableTime() == rhs.reachableTime() && retransTimer() == rhs.retransTimer())
  {
    if (prefixes.size() != rhs.prefixes.size())
      return false;

    for (size_t i = 0; i < prefixes.size(); i++)
      if (prefixes[i] != rhs.prefixes[i])
        return false;
  }
  else
    return false;

  return true;

}

inline void ICMPv6NDMRtrAd::setPrefixesInfo(const AdvPrefixList& pe)
{
  prefixes.assign(pe.begin(), pe.end());

#if defined VERBOSE
  copy(prefixes.begin(), prefixes.end(),
       ostream_iterator<PrefixEntry> (cout,"\n"));
#endif //VERBOSE
}

std::ostream&  ICMPv6NDMRtrAd::operator<<(std::ostream& os) const
{
  os<<" curHopLimit="<<curHopLimit()<<" managed="<<managed()<<" other="
    <<other()<<" lifetime="<<routerLifetime()
#ifdef USE_MOBILITY
    <<" ha="<<isHomeAgent()
#endif
    <<"  reachableTime="<<reachableTime()<<" retransTime="<<retransTimer()
    <<" mtu="<<MTU()<<" advInt="<<advInterval()
    <<" L2 addr="<<(hasSrcLLAddr()?srcLLAddr():string(""))
    <<" prefixes: ";
  std::copy(prefixesInfo().begin(), prefixesInfo().end(),
            ostream_iterator<ICMPv6NDOptPrefix, char>(os, "\t"));

#ifdef USE_HMIP
  if (hasMapOptions())
  {
    os<<"\nmapOptions:";
    std::copy(mapOptions().begin(), mapOptions().end(),
              ostream_iterator<HierarchicalMIPv6::HMIPv6ICMPv6NDOptMAP>(os, "\n"));
  }
#endif 
  return os;
}

ICMPv6NDMRedirect::ICMPv6NDMRedirect(const ipv6_addr& dest,
                                     const ipv6_addr& target,
                                     IPv6Datagram* dgram,
                                     string targetLinkAddr)
  :ICMPv6NDMsgBaseRedirect(ICMPv6_REDIRECT)
{
  setName("Redirect");

  setDestAddr(dest);
  setTargetAddr(target);
  attachHeader(dgram);
  if (targetLinkAddr != "")
    setTargetLLAddr(targetLinkAddr);
}

ICMPv6NDMRedirect::ICMPv6NDMRedirect(const ICMPv6NDMRedirect& src)
  :ICMPv6NDMsgBaseRedirect(ICMPv6_REDIRECT)
{
  setName(src.name());
  operator=(src);
}

const ICMPv6NDMRedirect& ICMPv6NDMRedirect::operator=(const ICMPv6NDMRedirect& src)
{
  if (this != &src)
    ICMPv6NDMsgBaseRedirect::operator=(src);

  return *this;
}

void ICMPv6NDMRedirect::attachHeader(IPv6Datagram* dgram)
{
  ICMPv6NDOptRedirect* redirHeader = static_cast<ICMPv6NDOptRedirect*> (opts[1]);

  if (opts[1])
    delete opts[1];

  redirHeader = new ICMPv6NDOptRedirect(dgram, dgram->length()/IPv6_EXT_UNIT_OCTETS/BITS);
  opts[1] = redirHeader;

  //TODO suppose to truncate to 1280 octets
  setLength(length()+dgram->length());
}

const IPv6Datagram* ICMPv6NDMRedirect::header() const
{
  return (static_cast<ICMPv6NDOptRedirect*> (opts[1]))->header();
}

IPv6Datagram* ICMPv6NDMRedirect::detachHeader()
{
  ICMPv6NDOptRedirect* redirHeader = static_cast<ICMPv6NDOptRedirect*> (opts[1]);
  if (!redirHeader)
    return 0;

  IPv6Datagram* dgram = redirHeader->removeHeader();
  setLength(length()-dgram->length());
  delete opts[1];
  opts[1] = 0;
  return dgram;
}

ICMPv6NDOptPrefix::ICMPv6NDOptPrefix(size_t prefix_len,
                                     const ipv6_addr& oprefix,
                                     bool onLink, bool autoConf,
                                     size_t validTime, size_t preferredTime
#ifdef USE_MOBILITY
                                     , bool rtr_addr
#endif
)
  :ICMPv6_NDOptionBase(NDO_PREFIX_INFO, 4) ,prefixLen(prefix_len),
   onLink(onLink), autoConf(autoConf), validLifetime(validTime),
   preferredLifetime(preferredTime), prefix(oprefix)
#ifdef USE_MOBILITY
   , rtrAddr(rtr_addr)
#endif
{}

ICMPv6NDOptPrefix::ICMPv6NDOptPrefix(const PrefixEntry& pe)
  :ICMPv6_NDOptionBase(NDO_PREFIX_INFO, 4), prefixLen(pe.prefix().prefixLength()),
   onLink(pe.advOnLink()), autoConf(pe.advAutoFlag()),
   validLifetime(pe.advValidLifetime()), preferredLifetime(pe.advPrefLifetime()),
   prefix(pe.prefix())
#ifdef USE_MOBILITY
   ,rtrAddr(pe.advRtrAddr())
#endif
{}

void ICMPv6NDOptRedirect::setHeader(IPv6Datagram* header)
{
  setLengthInUnits(lengthInUnits() - _header->length()/IPv6_EXT_UNIT_OCTETS/BITS);
  delete _header;
  _header = header;
  setLengthInUnits(lengthInUnits() + _header->length()/IPv6_EXT_UNIT_OCTETS/BITS);
}

IPv6Datagram* ICMPv6NDOptRedirect::removeHeader()
{
  ::IPv6Datagram* dgram = _header;
  _header = 0;
  setLengthInUnits(lengthInUnits() - dgram->length()/IPv6_EXT_UNIT_OCTETS/BITS);
  return dgram;
}

ICMPv6NDOptRedirect::~ICMPv6NDOptRedirect()
{
  delete removeHeader();
}

ICMPv6NDMRtrSol::ICMPv6NDMRtrSol()
  :ICMPv6NDMsgBseRtrSol(ICMPv6_ROUTER_SOL)
{
  setName("RS");
}

ICMPv6NDMRtrSol::ICMPv6NDMRtrSol(const ICMPv6NDMRtrSol& src)
  :ICMPv6NDMsgBseRtrSol(ICMPv6_ROUTER_SOL)
{
  setName(src.name());
  operator=(src);

}

const ICMPv6NDMRtrSol& ICMPv6NDMRtrSol::operator=(const ICMPv6NDMRtrSol& src)
{
  if (this != &src)
    ICMPv6NDMsgBseRtrSol::operator=(src);
  return *this;

}

bool ICMPv6NDMRtrSol::operator==(const ICMPv6NDMRtrSol& rhs) const
{
  if (this == &rhs)
    return true;

  if (!ICMPv6NDMsgBseRtrSol::operator==(rhs))
    return false;
  return srcLLAddr() == rhs.srcLLAddr()?true:false;

}
ICMPv6NDMNgbrSol::ICMPv6NDMNgbrSol(const ipv6_addr& targetAddr, const string&
                                   srcLLAddr)
  :ICMPv6NDMsgBaseNgbrSol(ICMPv6_NEIGHBOUR_SOL)
{
  setName("NS");

  setTargetAddr(targetAddr);
  if (!srcLLAddr.empty())
    setSrcLLAddr(srcLLAddr);
}

ICMPv6NDMNgbrSol::ICMPv6NDMNgbrSol(const ICMPv6NDMNgbrSol& src)
  :ICMPv6NDMsgBaseNgbrSol(ICMPv6_NEIGHBOUR_SOL)
{
  setName(src.name());
  operator=(src);

}

const ICMPv6NDMNgbrSol& ICMPv6NDMNgbrSol::operator=(const ICMPv6NDMNgbrSol& src)
{
  if (this != &src)
    ICMPv6NDMsgBaseNgbrSol::operator=(src);

  return *this;

}

bool ICMPv6NDMNgbrSol::operator==(const ICMPv6NDMNgbrSol& rhs) const
{
  if (this == &rhs)
    return true;

  if (!ICMPv6NDMsgBaseNgbrSol::operator==(rhs))
    return false;

  return hasSrcLLAddr() == rhs.hasSrcLLAddr()?srcLLAddr() == rhs.srcLLAddr():false;

}


ICMPv6NDMNgbrAd::ICMPv6NDMNgbrAd(const ipv6_addr& targetAddr, const string& addr, bool isRouter, bool solicited, bool override)
  :ICMPv6NDMsgBaseNgbrAd(ICMPv6_NEIGHBOUR_AD)
{
  setName("NA");

  setTargetAddr(targetAddr);
  setFlags(isRouter, solicited, override);
  setTargetLLAddr(addr);
}

ICMPv6NDMNgbrAd::ICMPv6NDMNgbrAd(const ICMPv6NDMNgbrAd& src)
  :ICMPv6NDMsgBaseNgbrAd(ICMPv6_NEIGHBOUR_AD)
{
  setName(src.name());
  operator=(src);
}

const ICMPv6NDMNgbrAd& ICMPv6NDMNgbrAd::operator=(const ICMPv6NDMNgbrAd& src)
{
  if (this != &src)
    ICMPv6NDMsgBaseNgbrAd::operator=(src);
  return *this;

}

bool ICMPv6NDMNgbrAd::operator==(const ICMPv6NDMNgbrAd& rhs) const
{
  if (this == &rhs)
    return true;

  if (!ICMPv6NDMsgBaseNgbrAd::operator==(rhs))
    return false;

  return targetLLAddr() == rhs.targetLLAddr() && isRouter() == rhs.isRouter() &&
    solicited() == rhs.solicited() && override() == rhs.override()?true:false;

}

std::ostream& ICMPv6NDMNgbrAd::operator<<(std::ostream& os) const
{
  return os<<" isRouter="<<isRouter()<<" sol="<<solicited()<<" ovr="<<override()
           <<(hasTargetLLAddr()?targetLLAddr():string(""));
}

#ifdef USE_MOBILITY
// class ICMPv6NDOptAdvInt

ICMPv6NDOptAdvInt::ICMPv6NDOptAdvInt(unsigned long advInt)
  : ICMPv6_NDOptionBase(NDO_ADV_INTERVAL, 1), advInterval(advInt)
{}
#endif

#ifdef USE_HMIP

void ICMPv6NDMRtrAd::addOption(const HierarchicalMIPv6::HMIPv6ICMPv6NDOptMAP& mapOpt)
{
  mapOpts.push_back(mapOpt);
}



void ICMPv6NDMRtrAd::setOptions(const HierarchicalMIPv6::MAPOptions& opts)
{
  mapOpts = opts;
}

#endif //USE_HMIP

#endif //__ICMPv6NDMESSAGE_CC
